#ifndef MIDI_CONTROL_CHANGE_SELECTOR_H
#define MIDI_CONTROL_CHANGE_SELECTOR_H

#include <QWidget>

class QTabWidget;

class MIDIControlChangeSelector : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIControlChangeSelector(QWidget *parent = nullptr);
    
private:
    QTabWidget *tabWidget;
    
    // Helper to setup each dynamic tab
    void setupDynamicTab(int tabIndex);
};

#endif // MIDI_CONTROL_CHANGE_SELECTOR_H
