#ifndef KEYMAP_EDITOR_H
#define KEYMAP_EDITOR_H

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

#include "keyboard_layout.h"
#include "keymap.h"

class KeyboardPC;

// a spin box for notes: shows "C4" instead of numbers, the minimum (-1) as "silent"
class NoteSpinBox : public QSpinBox
{
public:
    explicit NoteSpinBox(bool allow_silent, QWidget *parent = nullptr);

protected:
    QString textFromValue(int value) const override;
    int valueFromText(const QString &text) const override;
    QValidator::State validate(QString &text, int &pos) const override;
};

/*
 * The keymap editor of one keyboard section: a separate window, so the playing
 * view never changes by accident. It edits a working copy; Apply / OK hand it to
 * the section. While this window is active, the keys of its keyboard select keys
 * here instead of playing (MainTabs forwards them to physicalKeyPressed()).
 */
class KeymapEditor : public QWidget
{
    Q_OBJECT
public:
    // form_setting: "auto", "iso", "ansi", "jis"; auto_form: the form "auto" stands for now
    KeymapEditor(int keyboard_id, const Keymap &keymap, const QString &form_setting,
                 KeyboardForm auto_form, const QString &keymap_dir, QWidget *parent = nullptr);

    void physicalKeyPressed(int keycode);
    // "Learn" is on: the next incoming MIDI note sets the selected key
    bool isLearning() const;
    void learnMidiNote(int midi_note);
    // shows the global setting, without emitting
    void setShowSoundingNotesChecked(bool checked);

signals:
    void applied(const Keymap &keymap, const QString &form_setting);
    // takes effect at once, on all keyboards (not part of Apply / Cancel)
    void showSoundingNotesToggled(bool show);

private:
    int keyboard_id;
    Keymap working;
    KeyboardForm auto_form;
    QString keymap_dir;
    int selected_keycode = -1;

    QComboBox *combo_form;
    QComboBox *combo_pattern;
    NoteSpinBox *spin_start;
    KeyboardPC *keyboard;
    QLabel *label_key;
    NoteSpinBox *spin_note;
    QPushButton *button_learn;
    QPushButton *button_reset_key;
    QCheckBox *check_sounding_notes;

    QString formSetting() const;
    KeyboardForm currentForm() const;
    void showKeymap();
    void selectKey(int keycode);
    void showSelectedKey();

    void formChanged();
    void patternChanged();
    void startChanged(int value);
    void noteChanged(int value);
    void resetKey();
    void clearHandSetKeys();
    void loadFile();
    void saveFile();
    void apply();
};

#endif // KEYMAP_EDITOR_H
