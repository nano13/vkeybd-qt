#include "midi_pitchwheel.h"

MIDIPitchWheel::MIDIPitchWheel(InterfaceAudio *interface_audio, QWidget *parent) : QWidget(parent)
{
    this->interface_audio = interface_audio;
    
    QGridLayout *grid = new QGridLayout;
    setLayout(grid);
    grid->setContentsMargins(0, 0, 0, 0);
    
    QLabel *label_tether = new QLabel("tether strength");
    QLabel *label_pitch = new QLabel("pitch");
    
    this->slider_tether = new QSlider(Qt::Horizontal, this);
    this->slider_pitch = new QSlider(Qt::Horizontal, this);
    
    this->slider_tether->setRange(0, 127);
    this->slider_tether->setTickInterval(64);
    this->slider_tether->setTickPosition(QSlider::TicksBelow);
    this->slider_tether->setValue(64);
    
    // see the midi-protocol, e.g:
    // https://sites.uci.edu/camp2014/2014/04/30/managing-midi-pitchbend-messages/
    this->slider_pitch->setRange(0, 16383);
    this->slider_pitch->setTickInterval(8192);
    this->slider_pitch->setTickPosition(QSlider::TicksBelow);
    this->slider_pitch->setValue(8192);
    
    grid->addWidget(label_tether, 0, 0);
    grid->addWidget(label_pitch, 0, 1);
    grid->addWidget(this->slider_tether, 1, 0);
    grid->addWidget(this->slider_pitch, 1, 1);
    
    QString style = "QSlider::groove:horizontal {"
            "   border: 1px solid;"
            "   height: 10px;"
            "   margin: 0px;"
            "}"
            "QSlider::handle:horizontal {"
            "   background-color: black;"
            "   border: 1px solid;"
            "   height: 40px;"
            "   width: 20px;"
            "   margin: -15px 0px;"
            "}";
    // https://stackoverflow.com/questions/27531542/tick-marks-disappear-on-styled-qslider
    this->slider_tether->setStyleSheet(style);
    this->slider_pitch->setStyleSheet(style);
    
    this->slider_tether->setTracking(false);
    connect(this->slider_tether, &QSlider::valueChanged, this, &MIDIPitchWheel::startPitchThread);
    this->slider_pitch->setTracking(false);
    connect(this->slider_pitch, &QSlider::valueChanged, this, &MIDIPitchWheel::startPitchThread);
    
    this->worker = new MIDIPitchWheelWorker();
    connect(this->worker, &MIDIPitchWheelWorker::movePitchSlider, this, &MIDIPitchWheel::movePitchSlider);
    
    this->thread = new QThread(this);
    this->worker->moveToThread(this->thread);
    this->thread->start();
}

MIDIPitchWheel::~MIDIPitchWheel()
{
    this->thread->exit();
}

void MIDIPitchWheel::startPitchThread()
{
    int tether = this->slider_tether->value();
    int pitch = this->slider_pitch->value();
    
    this->worker->setValues(tether, pitch);
}

void MIDIPitchWheel::movePitchSlider(int position)
{
    this->slider_pitch->setValue(position);
    
    //this->interface_audio->keyPitchbendEvent(position);
}

void MIDIPitchWheel::movePitchWheel(int key)
{
    int value = 0;
    
    if (key == Qt::Key_Left)
    {
        value = 4096;
    }
    else if (key == Qt::Key_Right)
    {
        value = 12288;
    }
    else if (key == Qt::Key_Down)
    {
        value = 0;
    }
    else if (key == Qt::Key_Up)
    {
        value = 16384;
    }
    
    this->slider_pitch->setValue(value);
    //startPitchThread();
}



MIDIPitchWheelWorker::MIDIPitchWheelWorker(QObject *parent) : QObject(parent)
{
    this->timer = new QTimer(this);
    this->timer->setInterval(30);
    this->timer->setTimerType(Qt::PreciseTimer);
    connect(this->timer, &QTimer::timeout, this, &MIDIPitchWheelWorker::tick, Qt::DirectConnection);
    
    this->timer->start();
}

void MIDIPitchWheelWorker::setValues(int tether, int pitch)
{
    this->tether = tether;
    this->pitch = pitch;
    
    if (this->pitch < 8192)
    {
        this->sign_positive = false;
    }
    else
    {
        this->sign_positive = true;
    }
}

void MIDIPitchWheelWorker::tick()
{
    if (this->pitch != 8192)
    {
        if (this->pitch < 8192)
        {
            this->pitch = this->pitch + this->tether*25;
        }
        else if (this->pitch > 8192)
        {
            this->pitch = this->pitch - this->tether*25;
        }
        
        // if we shoot over 8192 we set pitch to 8192 and quit
        if ((this->sign_positive && this->pitch < 8192) || (!this->sign_positive && this->pitch > 8192))
        {
            this->pitch = 8192;
        }
        
        emit movePitchSlider(this->pitch);
    }
}
