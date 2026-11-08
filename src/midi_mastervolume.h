#ifndef MIDI_MASTERVOLUME_H
#define MIDI_MASTERVOLUME_H

#include <QObject>
#include <QTimer>
#include <QLabel>
#include <QSlider>

#include "enums_structs.h"

/*
 * Master volume (DCA, percent), movable by keys:
 *  Down / Up:          change while pressed, springs back to 100 % ("tether") when released
 *  PageDown / PageUp:  change while pressed, stays where it is when released
 *
 * Runs on a timer in the GUI thread (formerly one worker thread per tab).
 */
class MIDIMasterVolume : public QObject
{
    Q_OBJECT
public:
    explicit MIDIMasterVolume(QObject *parent = nullptr);
    
    QLabel *label_volume;
    QLabel *label_tether;
    QSlider *slider_volume;
    QSlider *slider_tether;
    
    void volumeKeyPressed(int key);
    void volumeKeyReleased();
    int getValue();
    // emits sliderMoved
    void setValue(int value);
    // does not emit sliderMoved
    void setValueSilently(int value);
    
private:
    static constexpr int NEUTRAL = 100;
    
    QTimer *timer;
    
    qreal volume = NEUTRAL;
    qreal tether = .1;
    int direction = 0;          // -1, 0, 1
    bool sign_positive = true;  // side of 100 % we are on
    bool reset_slider = true;   // spring back to 100 % when the key is released
    
    void tick();
    void moveVolumeSlider(int value);
    void updateSliderLabel(int value);
    
private slots:
    void volumeSliderMoved(int value);
    
signals:
    void sliderMoved(int value);
};

#endif // MIDI_MASTERVOLUME_H
