#ifndef MIDIPITCHWHEEL_H
#define MIDIPITCHWHEEL_H

#include <QObject>
#include <QTimer>
#include <QSlider>
#include <QLabel>
#include <QMap>
#include <QVariant>

#include "enums_structs.h"

/*
 * Pitch wheel, movable by keys:
 *  Left / Right: bend while pressed, springs back ("tether") when released
 *  End / Home:   bend 8 times slower, stays where it is when released
 *
 * Runs on a timer in the GUI thread (formerly one worker thread per tab).
 */
class MIDIPitchWheel : public QObject
{
    Q_OBJECT
public:
    explicit MIDIPitchWheel(QObject *parent = nullptr);

    QLabel *label_tether;
    QLabel *label_pitch;

    QSlider *slider_tether;
    QSlider *slider_pitch;

    void movePitchWheel(int key);
    void pitchKeyPressed(int key);
    void pitchKeyReleased();

    QMap<QString,int> getData();
    // sets tether and position without emitting pitchWheelMoved
    void setData(QMap<QString,QVariant> data);

private:
    static constexpr int CENTER = 8192;
    static constexpr int MAXIMUM = 16383;

    QTimer *timer;

    int tether = 64;
    int pitch = CENTER;
    int direction = 0;          // -1, 0, 1
    bool sign_positive = true;  // side of the center we are on
    bool reset_slider = true;   // spring back to the center when the key is released

    void tick();
    void movePitchSlider(int position);
    void sliderMoved(int position);
    void updateLabel(int position);

signals:
    void pitchWheelMoved(int position);
    void tetherChanged(int tether);
};

#endif // MIDIPITCHWHEEL_H
