#include "midi_pitchwheel.h"

MIDIPitchWheel::MIDIPitchWheel(QWidget *parent) : QWidget(parent)
{
    QGridLayout *grid = new QGridLayout;
    setLayout(grid);
    
    QLabel *label_tether = new QLabel("tether strength");
    QLabel *label_pitch = new QLabel("pitch");
    
    this->slider_tether = new QSlider(Qt::Horizontal, this);
    this->slider_pitch = new QSlider(Qt::Horizontal, this);
    
    this->slider_tether->setRange(0, 127);
    this->slider_tether->setValue(10);
    
    // see the midi-protocol, e.g:
    // https://sites.uci.edu/camp2014/2014/04/30/managing-midi-pitchbend-messages/
    this->slider_pitch->setRange(0, 16383);
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
    this->slider_tether->setStyleSheet(style);
    this->slider_pitch->setStyleSheet(style);
    
    this->slider_pitch->setTracking(false);
    connect(this->slider_pitch, &QSlider::valueChanged, this, &MIDIPitchWheel::startPitchThread);
    
    this->thread = new QThread(this);
    
}

void MIDIPitchWheel::startPitchThread()
{
    int tether = this->slider_tether->value();
    int pitch = this->slider_pitch->value();
    
    //if (tether != 0 && !this->thread->isRunning())
    if (tether != 0)
    {
        MIDIPitchWheelWorker *worker = new MIDIPitchWheelWorker(tether, pitch);
        //worker->setInterval(30);
        //worker->setValues(tether, pitch);
        connect(worker, &MIDIPitchWheelWorker::movePitchSlider, this, &MIDIPitchWheel::movePitchSlider);
        
        worker->moveToThread(this->thread);
        //connect(this->thread, &QThread::started, worker, &MIDIPitchWheelWorker::process);
        
        if (!this->thread->isRunning())
        {
            
            
            qDebug() << "starting thread";
            this->thread->start();
            
            
        }
    }
}

void MIDIPitchWheel::movePitchSlider(int position)
{
    //qDebug() << "moving slider: " + QString::number(position);
    
    this->slider_pitch->setValue(position);
}



MIDIPitchWheelWorker::MIDIPitchWheelWorker(int tether, int pitch, QObject *parent) : QObject(parent)
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
    
    this->timer = new QTimer(this);
    this->timer->setInterval(30);
    this->timer->setTimerType(Qt::PreciseTimer);
    connect(this->timer, &QTimer::timeout, this, &MIDIPitchWheelWorker::tick, Qt::DirectConnection);
    this->timer->start();
}

void MIDIPitchWheelWorker::tick()
{
    //while (this->pitch != 8192)
    if (this->pitch == 8192)
    {
        this->timer->stop();
    }
    else
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
