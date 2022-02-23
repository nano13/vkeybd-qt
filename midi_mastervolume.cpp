#include "midi_mastervolume.h"

MIDIMasterVolume::MIDIMasterVolume(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setMargin(0);
    
    this->label_volume = new QLabel("Master Volume (DCA): 100%");
    this->slider_volume = new QSlider(Qt::Horizontal, this);
    this->slider_volume->setRange(0, 120);
    this->slider_volume->setTickInterval(20);
    this->slider_volume->setTickPosition(QSlider::TicksBelow);
    this->slider_volume->setValue(100);
    
    connect(this->slider_volume, &QSlider::valueChanged, this, &MIDIMasterVolume::volumeSliderMoved);
    
    layout->addWidget(this->label_volume);
    layout->addWidget(this->slider_volume);
    
    this->worker = new MIDIMasterVolumeWorker();
    connect(this->worker, &MIDIMasterVolumeWorker::moveVolumeSlider, this, &MIDIMasterVolume::moveVolumeSlider);
    
    this->thread = new QThread(this);
    this->worker->moveToThread(this->thread);
    this->thread->start();
}

MIDIMasterVolume::~MIDIMasterVolume()
{
    this->thread->exit();
}

void MIDIMasterVolume::moveVolumeSlider(int position)
{
    this->slider_volume->setValue(position);
}

void MIDIMasterVolume::volumeSliderMoved(int value)
{
    emit sliderMoved(value);
    
    QString label = this->label_volume->text().split(":").at(0);
    label += ": " + QString::number(value) + "%";
    
    this->label_volume->setText(label);
}

void MIDIMasterVolume::volumeKeyPressed(int key)
{
    int volume = this->slider_volume->value();
    this->worker->setVolume(volume);
    
    int direction = 0;
    if (key == Qt::Key_Down)
    {
        direction = -1;
    }
    else if (key == Qt::Key_Up)
    {
        direction = 1;
    }
    
    this->slider_volume->blockSignals(true);
    this->worker->keyDown(direction);
}
void MIDIMasterVolume::volumeKeyReleased()
{
    this->slider_volume->blockSignals(false);
    this->worker->keyUp();
}





MIDIMasterVolumeWorker::MIDIMasterVolumeWorker(QObject *parent)
    : QObject{parent}
{
    this->timer = new QTimer(this);
    this->timer->setInterval(30);
    this->timer->setTimerType(Qt::PreciseTimer);
    connect(this->timer, &QTimer::timeout, this, &MIDIMasterVolumeWorker::tick, Qt::DirectConnection);
    
    this->timer->start();
}

void MIDIMasterVolumeWorker::setVolume(int value)
{
    this->volume = value;
    
    if (this->volume < 100)
    {
        this->sign_positive = false;
    }
    else
    {
        this->sign_positive = true;
    }
}

void MIDIMasterVolumeWorker::keyDown(int direction)
{
    this->direction = direction;
    
    if (this->direction < 0)
    {
        this->sign_positive = false;
    }
    else if (this->direction > 0)
    {
        this->sign_positive = true;
    }
}
void MIDIMasterVolumeWorker::keyUp()
{
    this->direction = 0;
}

void MIDIMasterVolumeWorker::tick()
{
    if (this->direction == 0)
    {
        // reset volume slider
        if (this->volume != 100)
        {
            if (this->volume < 100)
            {
                this->volume = this->volume + this->tether;
            }
            else if (this->volume > 100)
            {
                this->volume = this->volume - this->tether;
            }
            
            // if we shoot over 100 we set volume to 100 and quit
            if ((this->sign_positive && this->volume < 100) || (!this->sign_positive && this->volume > 100))
            {
                this->volume = 100;
                
                //this->timer->setInterval(100);
            }
            
            emit moveVolumeSlider(this->volume);
        }
    }
    else
    {
        // pitch wheel moved as long as key pressed
        this->volume = this->volume + this->direction * this->volume;
        
        emit moveVolumeSlider(this->volume);
    }
}
