#ifndef KEYBOARDPIANO_H
#define KEYBOARDPIANO_H

#include <QList>
#include <QMap>
#include <QResizeEvent>
#include <QWidget>

#include "button_piano.h"

// A visual representation of the MIDI notes assigned to the PC keyboard.
// KeyboardPC owns the note mapping; this widget does not invent an octave range.
class KeyboardPiano : public QWidget
{
    Q_OBJECT
public:
    explicit KeyboardPiano(QWidget *parent = nullptr);

    void setMidiNotes(const QList<int> &notes);
    QList<int> midiNotes() const;

    void keyPressed(int midicode);
    void keyReleased(int midicode);
    void allKeysUp();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    static bool isBlackKey(int midicode);
    void layoutKeys();
    void colorizeBlackKeys(ButtonPiano *button);
    void colorizeWhiteKeys(ButtonPiano *button);

    QMap<int, ButtonPiano*> list_of_keys_midi;
    static constexpr int keyboard_height = 79;
};

#endif // KEYBOARDPIANO_H
