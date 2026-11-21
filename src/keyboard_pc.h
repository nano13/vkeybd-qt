#ifndef KEYBOARDPC_H
#define KEYBOARDPC_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QColor>
#include <QHash>
#include <QList>
#include <QPoint>
#include <QSet>
#include <QDebug>

#include "enums_structs.h"
#include "keyboard_layout.h"
#include "keymap.h"

/*
 * One key of the PC keyboard display. Draws its note names as usual, and below
 * them, in a smaller font, the notes that sound right now when it is pressed:
 * - only octave numbers ("2 3") while they are the key's own note (the normal
 *   case, shifts by whole octaves),
 * - note names with octave ("C#4 F4") as soon as something transposes by a
 *   non-octave amount,
 * - "lowest…highest" if the list does not fit the key. The full list is in the
 *   tooltip.
 * C4 is the MIDI note 60 (middle C).
 */
class PcKeyButton : public QPushButton
{
    Q_OBJECT
    // text color while pressed; QSS: qproperty-pressedTextColor
    Q_PROPERTY(QColor pressedTextColor READ pressedTextColor WRITE setPressedTextColor)
public:
    explicit PcKeyButton(QWidget *parent = nullptr);

    // pitch class (0 = C) of the key, -1 for keys without a note
    void setKeyPitchClass(int pitch_class);
    void setSoundingNotes(const QList<int> &notes);
    QList<int> soundingNotes() const;
    // the small line as drawn into a key of this width (for tests)
    QString soundingText(int width) const;
    // off: only the note names are drawn (the tooltip still lists the notes)
    void setShowSoundingLine(bool show);
    // keymap editor: a frame around the selected key, a corner mark on keys set by hand
    void setSelected(bool selected);
    void setMarked(bool marked);

    QColor pressedTextColor() const;
    void setPressedTextColor(const QColor &color);

    static QString noteName(int midi_note);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int key_pitch_class = -1;
    QList<int> sounding_notes;
    bool show_sounding_line = true;
    bool selected = false;
    bool marked = false;
    QColor pressed_text_color = QColor(Qt::black);

    QFont smallFont() const;
    void updateToolTip();
};

/*
 * The PC keyboard display: shows the physical keys of one keyboard form
 * (ISO / ANSI / JIS) with the notes of its keymap, lights up pressed keys and
 * turns presses into MIDIPress / MIDIRelease. In edit mode (keymap editor) a
 * press only selects the key.
 */
class KeyboardPC : public QWidget
{
    Q_OBJECT
public:
    explicit KeyboardPC(QWidget *parent = nullptr);

    // The PC-keyboard mapping is authoritative for the piano view.
    // Rows and columns as drawn on the current form; -1: no note.
    QList<QList<int>> midiMapping() const;
    QList<int> midiNotes() const;
    bool setMidiMapping(const QList<QList<int>> &mapping);

    // what the keys play: the notes follow the current form
    void setKeymap(const Keymap &keymap);
    Keymap currentKeymap() const;

    // the physical form; changing it rebuilds the keys (held notes are released)
    void setKeyboardForm(KeyboardForm form);
    KeyboardForm keyboardForm() const;
    // automatic: a key that only exists on another form switches to that form
    void setAutoForm(bool automatic);
    bool autoForm() const;
    // all keys of the current form, and whether one can play a note
    QList<int> keycodes() const;
    bool isNoteKey(int keycode) const;

    void keyDownRaw(int keycode);
    void keyUpRaw(int keycode);
    void allKeysUp();

    // Highlight MIDI input without emitting MIDIPress/MIDIRelease or changing
    // the physical-key state. The note is in this widget's MIDI mapping range.
    void setMidiNoteDown(int note, bool down);
    void clearMidiNoteHighlights();

    // what every mapped note sounds as right now (key: the note of the mapping);
    // notes without an entry show nothing
    void setSoundingNotes(const QHash<int, QList<int>> &sounding);
    // the small line on the keys; without it the note rows are lower
    void setShowSoundingNotes(bool show);
    bool showSoundingNotes() const;

    // keymap editor: clicks select keys (keySelected) instead of playing them
    void setEditMode(bool edit);
    void setSelectedKey(int keycode);
    int selectedKey() const;
    // larger keys (editor); the fonts come from the style sheet
    void setScale(qreal scale);

private:
    static QString noteLabel(int note);
    static QString roleLabel(PcKeyRole role);
    static QString styleName(PcKeyRole role, int note);

    KeyboardForm form = KeyboardForm::ISO;
    bool auto_form = false;
    Keymap keymap;
    bool edit_mode = false;
    int selected_keycode = -1;
    qreal scale = 1.0;

    QList<QList<PcKeyDef>> key_layout;
    QHash<int, QPoint> key_position;    // keycode -> (column, row)

    QList<QList<PcKeyButton*>> list_of_buttons;
    QList<QList<int>> midi_codes;
    QList<QList<int>> active_midi_codes;
    QList<QList<bool>> physical_key_down;
    QSet<int> midi_held_notes;
    QHash<int, QList<int>> sounding;
    bool show_sounding_notes = true;

    void setupKeys();
    void drawButtons();
    void layoutButtons();
    int rowHeight(int row) const;
    void updateHeight();
    void updateLabels();
    void updateEditMarks();
    void updateSoundingNotes();
    void updateButtonVisual(int row, int col);
    void updateButtonVisuals();
    void releaseAllKeys();

    void pressKey(int row, int col);
    void releaseKey(int row, int col);
    void rolePressed(PcKeyRole role, bool pressed);

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void midiMappingChanged();
    void MIDIPress(int midicode);
    void MIDIRelease(int midicode);

    void octaveDown();
    void octaveUp();

    // the pedal keys clicked with the mouse
    void sustain(bool pressed);
    void sostenuto(bool pressed);
    void soft(bool pressed);

    // automatic form: a key proved another form (the display has switched already)
    void keyboardFormDetected(KeyboardForm form);
    // edit mode: a key was clicked
    void keySelected(int keycode);
};

#endif // KEYBOARDPC_H
