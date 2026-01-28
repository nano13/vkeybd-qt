#ifndef MIDI_CONTROL_CHANGE_SELECTOR_H
#define MIDI_CONTROL_CHANGE_SELECTOR_H

#include <QWidget>

class QTabWidget;

// ------------------- MIDICCTableWidget -------------------
class MIDICCTableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MIDICCTableWidget(QWidget *parent = nullptr);
};

// ------------------- MIDIControlChangeSelector -------------------
class MIDIControlChangeSelector : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIControlChangeSelector(QWidget *parent = nullptr);
    
private:
    QTabWidget *tabWidget;
};

#endif // MIDI_CONTROL_CHANGE_SELECTOR_H
