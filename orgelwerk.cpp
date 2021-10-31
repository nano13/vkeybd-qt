#include "orgelwerk.h"

Orgelwerk::Orgelwerk(QString label, QWidget *parent) : QWidget(parent)
{
    this->interface_audio = new InterfaceAlsa(label);
    //this->interface_audio = new InterfaceJack;
    
    drawGUI();
    initInputThread();
}

void Orgelwerk::drawGUI()
{
    this->channels = new MIDIChannelSelector;
    this->keys = new MIDIKeySelector;
    this->pitch = new MIDIPitchWheel(this->interface_audio);
    this->piano = new KeyboardPiano;
    this->pc = new KeyboardPC;
    
    connect(this->pc, &KeyboardPC::MIDIPress, this, &Orgelwerk::keyMIDIDown);
    connect(this->pc, &KeyboardPC::MIDIRelease, this, &Orgelwerk::keyMIDIUp);
    
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    
    QGroupBox *group_channels = new QGroupBox("Channels");
    QGroupBox *group_keys = new QGroupBox("Keys");
    QGroupBox *group_pitch = new QGroupBox("Pitch");
    QGroupBox *group_keyboards = new QGroupBox("Keyboards");
    
    QVBoxLayout *layout_channels = new QVBoxLayout;
    QVBoxLayout *layout_keys = new QVBoxLayout;
    QHBoxLayout *layout_pitch = new QHBoxLayout;
    QVBoxLayout *layout_keyboards = new QVBoxLayout;
    
    layout_channels->setContentsMargins(0, 0, 0, 0);
    
    group_channels->setLayout(layout_channels);
    group_keys->setLayout(layout_keys);
    group_pitch->setLayout(layout_pitch);
    group_keyboards->setLayout(layout_keyboards);
    
    QScrollArea *scroll_channels = new QScrollArea;
    scroll_channels->setWidget(this->channels);
    
    layout_channels->addWidget(scroll_channels);
    layout_keys->addWidget(this->keys);
    //layout_program->addWidget();
    layout_pitch->addWidget(this->pitch);
    layout_keyboards->addWidget(this->piano);
    layout_keyboards->addWidget(this->pc);
    
    this->button_panic->setText("Panic!");
    connect(this->button_panic, &QPushButton::clicked, this, &Orgelwerk::panicKeyPressed);
    QString stylesheet = "QPushButton {"
                         "  color: white;"
                         "  background-color: red;"
                         "}"
                         "QPushButton:pressed {"
                         "  color: white;"
                         "  background-color: yellow;"
                         "}";
    this->button_panic->setStyleSheet(stylesheet);
    
    layout->addWidget(group_channels);
    layout->addWidget(group_keys);
    layout->addWidget(group_pitch);
    layout->addWidget(group_keyboards);
    layout->addWidget(this->button_panic);
}

void Orgelwerk::initInputThread()
{
    this->thread_input = new QThread(this);
    this->worker_input = new InputKeyboardRawThread();
    this->worker_input->moveToThread(this->thread_input);
    
    this->thread_input->start();
    
    //this->worker_input->start();
}

Orgelwerk::~Orgelwerk()
{
    this->thread_input->exit();
}

void Orgelwerk::keyDown(int keycode)
{
    qDebug() << "keyDown: " + QString::number(keycode);
    this->pc->keyDown(keycode);
}
void Orgelwerk::keyUp(int keycode)
{
    qDebug() << "keyUp: " + QString::number(keycode);
    this->pc->keyUp(keycode);
}

void Orgelwerk::panicKeyPressed()
{
    for (int i=0; i <= 127; i++)
    {
        this->piano->keyReleased(i);
    }
    
    this->pc->allKeysUp();
    
    this->interface_audio->keyPanicEvent();
}

void Orgelwerk::keyMIDIDown(int midicode)
{
    this->piano->keyPressed(midicode);
    
    QList<int> list_of_keys = this->keys->getListOfSelectedKeys();
    for (int i=0; i < list_of_keys.length(); i++)
    {
        this->interface_audio->keyPressEvent(midicode + list_of_keys.at(i) - 12);
    }
}
void Orgelwerk::keyMIDIUp(int midicode)
{
    this->piano->keyReleased(midicode);
    
    QList<int> list_of_keys = this->keys->getListOfSelectedKeys();
    for (int i=0; i < list_of_keys.length(); i++)
    {
        this->interface_audio->keyReleaseEvent(midicode + list_of_keys.at(i) - 12);
    }
}

void Orgelwerk::movePitchWheel(int key)
{
    this->pitch->movePitchWheel(key);
}
