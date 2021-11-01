#ifndef MIDIPITCHWHEEL_H
#define MIDIPITCHWHEEL_H

#include <QThread>
#include <QTimer>

#include <interface_audio.h>

class MIDIPitchWheelWorker : public QObject
{
    Q_OBJECT
    
public:
    explicit MIDIPitchWheelWorker(QObject *parent = 0);
    
    void setTether(int tether);
    void setPitch(int pitch);
    void keyDown(int direction); // direction is either -1, 0, 1
    void keyUp();
    
private:
    QTimer *timer;
    
    int tether;
    int pitch = 8192;
    int direction = 0; // direction is either -1, 0, 1
    
    bool sign_positive; // false: -, true: +
    
//protected:
public slots:
    void tick();
    
signals:
    void movePitchSlider(int step);
    
};



#include <QObject>
#include <QWidget>
#include <QGridLayout>
#include <QSlider>
#include <QLabel>
#include <QDebug>

class MIDIPitchWheel : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIPitchWheel(QWidget *parent = nullptr);
    ~MIDIPitchWheel();
    
    void movePitchWheel(int key);
    void pitchKeyPressed(int key);
    void pitchKeyReleased();
    
private:
    QSlider *slider_tether;
    QSlider *slider_pitch;
    
    QThread *thread;
    MIDIPitchWheelWorker *worker;
    
    void startPitchThread();
    void movePitchSlider(int position);
    void sliderMoved(int position);
    
signals:
    void pitchWheelMoved(int position);
};

#endif // MIDIPITCHWHEEL_H
