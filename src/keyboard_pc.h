#ifndef KEYBOARDPC_H
#define KEYBOARDPC_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QResizeEvent>
#include <QList>
#include <QDebug>

#include "enums_structs.h"

class KeyboardPC : public QWidget
{
    Q_OBJECT
public:
    explicit KeyboardPC(QWidget *parent = nullptr);
    
    float button_scale = .799;
    
    // The PC-keyboard mapping is authoritative for the piano view.
    QList<QList<int>> midiMapping() const;
    QList<int> midiNotes() const;
    bool setMidiMapping(const QList<QList<int>> &mapping);

    void keyDownRaw(int keycode);
    void keyUpRaw(int keycode);
    void allKeysUp();
    
private:
    QList<QList<int>> getButtonSizes();
    QList<QList<QString>> getButtonColors();
    QList<QList<QString>> getButtonLabels();
    // "raw" means: as read directly from /dev/input
    QList<QList<int>> getButtonKeycodesRaw();
    static QList<QList<int>> defaultMIDICodes();
    
    QList<QList<QPushButton*>> list_of_buttons;
    QList<QList<int>> midi_codes;
    QList<QList<int>> active_midi_codes;
    
    void drawButtons();
    void layoutButtons();
    
    void buttonPressed(int midicode);

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void midiMappingChanged();
    void MIDIPress(int midicode);
    void MIDIRelease(int midicode);
    
    void octaveDown();
    void octaveUp();
    
};

#endif // KEYBOARDPC_H
