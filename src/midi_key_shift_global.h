#ifndef MIDIKEYSHIFTGLOBAL_H
#define MIDIKEYSHIFTGLOBAL_H

#include <QObject>
#include <QWidget>

#include <QGridLayout>
#include <QFrame>
#include <QFile>
#include <QLabel>

#include "midi_key_shift_widget.h"

class MIDIKeyShiftGlobal : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIKeyShiftGlobal(QWidget *parent = nullptr);
    
    // the global key shift changed (Print / Pause / Scroll Lock, or this window):
    // both fields show it, and the relative one continues from there
    void setGlobalValue(int value);
    
private:
    int last_relative_pitch = 0;
    MIDIKeyShiftWidget *key_shift_absolute;
    MIDIKeyShiftWidget *key_shift_relative;
    
    void valueChanged(int value, bool is_relative);
    
signals:
    void signalKeyShiftChanged(int value, bool is_relative);
};

#endif // MIDIKEYSHIFTGLOBAL_H
