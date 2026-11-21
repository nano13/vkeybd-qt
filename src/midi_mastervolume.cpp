#include "midi_mastervolume.h"

#include <QSignalBlocker>

MIDIMasterVolume::MIDIMasterVolume(QObject *parent)
    : QObject{parent}
{
    this->label_volume = new QLabel("Master Volume (DCA): 100%");
    this->slider_volume = new QSlider(Qt::Horizontal);
    
    this->label_tether = new QLabel("Volume Tether Strength");
    this->slider_tether = new QSlider(Qt::Horizontal);
    
    this->slider_tether->setRange(0, 150);
    this->slider_tether->setTickInterval(20);
    this->slider_tether->setTickPosition(QSlider::TicksBelow);
    this->slider_tether->setValue(20);
    
    this->slider_volume->setRange(0, 120);
    this->slider_volume->setTickInterval(20);
    this->slider_volume->setTickPosition(QSlider::TicksBelow);
    this->slider_volume->setValue(NEUTRAL);
    
    connect(this->slider_volume, &QSlider::valueChanged, this, &MIDIMasterVolume::volumeSliderMoved);
    
    this->timer = new QTimer(this);
    this->timer->setInterval(3);
    this->timer->setTimerType(Qt::PreciseTimer);
    connect(this->timer, &QTimer::timeout, this, &MIDIMasterVolume::tick);
}

void MIDIMasterVolume::moveVolumeSlider(int value)
{
    {
        const QSignalBlocker blocker(this->slider_volume);
        this->slider_volume->setValue(value);
    }
    
    updateSliderLabel(value);
    
    emit sliderMoved(value);
}

void MIDIMasterVolume::volumeSliderMoved(int value)
{
    updateSliderLabel(value);
    
    emit sliderMoved(value);
}
void MIDIMasterVolume::updateSliderLabel(int value)
{
    QString label = this->label_volume->text().split(":").at(0);
    label += ": " + QString::number(value) + "%";
    
    this->label_volume->setText(label);
}

void MIDIMasterVolume::volumeKeyPressed(int key)
{
    this->tether = static_cast<qreal>(this->slider_tether->value()) / 200;
    
    // continue from the exact (fractional) position while a change is still
    // running, otherwise from what the slider shows (it may have been dragged)
    if (!this->timer->isActive())
        this->volume = this->slider_volume->value();
    
    int direction = 0;
    if (key == KeysRaw::Down || key == KeysRaw::Up)
    {
        direction = (key == KeysRaw::Down) ? -1 : 1;
        
        // a new temporary change: it returns to the value set before it
        if (!this->temporary)
            this->rest = this->slider_volume->value();
        this->temporary = true;
        this->reset_slider = true;
    }
    else if (key == KeysRaw::PageDown || key == KeysRaw::PageUp)
    {
        direction = (key == KeysRaw::PageDown) ? -1 : 1;
        
        // a permanent change: where it ends is the new value to return to
        this->temporary = false;
        this->reset_slider = false;
    }
    
    this->sign_positive = (this->volume >= this->rest);
    
    this->direction = direction;
    if (this->direction < 0)
        this->sign_positive = false;
    else if (this->direction > 0)
        this->sign_positive = true;
    
    this->timer->start();
}
void MIDIMasterVolume::volumeKeyReleased()
{
    // the timer keeps running to let the slider spring back (if reset_slider)
    this->direction = 0;
    // the side of the rest value we spring back from (the overshoot check in tick())
    this->sign_positive = (this->volume >= this->rest);
}

void MIDIMasterVolume::tick()
{
    const int previous = qRound(this->volume);
    
    if (this->direction == 0)
    {
        if (this->volume == this->rest || !this->reset_slider)
        {
            this->temporary = false;
            this->timer->stop();
            return;
        }
        
        // spring back towards the value before the temporary change
        if (this->volume < this->rest)
            this->volume += this->tether;
        else
            this->volume -= this->tether;
        
        // if we shoot over it we stop exactly there
        if ((this->sign_positive && this->volume < this->rest) || (!this->sign_positive && this->volume > this->rest))
        {
            this->volume = this->rest;
            this->temporary = false;
            this->timer->stop();
        }
    }
    else
    {
        // moved as long as the key is pressed, within the slider range
        this->volume = qBound<qreal>(this->slider_volume->minimum(), this->volume + this->direction * this->tether, this->slider_volume->maximum());
    }
    
    // only whole percent steps reach the slider (and the MIDI output)
    const int current = qRound(this->volume);
    if (current != previous || current != this->slider_volume->value())
    {
        moveVolumeSlider(current);
    }
}

int MIDIMasterVolume::getValue()
{
    return this->slider_volume->value();
}
void MIDIMasterVolume::setValue(int value)
{
    this->slider_volume->setValue(value);
}
void MIDIMasterVolume::setValueSilently(int value)
{
    this->timer->stop();
    this->direction = 0;
    this->volume = value;
    // e.g. another tab or preset: its value is the one to return to
    this->rest = value;
    this->temporary = false;
    
    {
        const QSignalBlocker blocker(this->slider_volume);
        this->slider_volume->setValue(value);
    }
    updateSliderLabel(value);
}
