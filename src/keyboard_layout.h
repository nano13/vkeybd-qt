#ifndef KEYBOARD_LAYOUT_H
#define KEYBOARD_LAYOUT_H

#include <QList>
#include <QString>

/*
 * The physical forms of PC keyboards. KeyDeskMIDI reads key positions (Linux
 * input key codes), not characters, so QWERTY / QWERTZ / AZERTY do not matter;
 * only which keys exist and where they sit:
 * - ISO  (most of Europe): the < key (86) next to the left Shift, the tall Enter
 *   with the # key (43) left of it
 * - ANSI (US, much of Asia): no key 86, a wide left Shift, backslash (43) at the
 *   end of the row above Enter
 * - JIS  (Japan): additionally ¥ (124) in the number row, ろ (89) next to the
 *   right Shift, Muhenkan / Henkan / Kana (94, 92, 93) around the space bar
 */
enum class KeyboardForm
{
    ISO,
    ANSI,
    JIS
};

enum class PcKeyRole
{
    Note,           // plays a note (or nothing, if the keymap leaves it silent)
    OctaveDown,
    OctaveUp,
    Soft,
    Sostenuto,
    Sustain,
    TabModifier     // Menu: modifier for the F-keys
};

struct PcKeyDef
{
    int keycode;        // Linux input key code
    int width;          // in layout units, a normal key is 30
    PcKeyRole role;
    // column in the note grid of the isomorphic patterns (one step = one key to
    // the right; the key below is down-right); -1: no pattern assigns a note
    int grid_col;
};

namespace KeyboardLayout
{
    // rows from the number row down to the space bar row
    QList<QList<PcKeyDef>> keys(KeyboardForm form);

    QString formName(KeyboardForm form);            // "iso", "ansi", "jis"
    QString formTitle(KeyboardForm form);           // "ISO (Europe)", ...
    bool formFromName(const QString &name, KeyboardForm *form);
    QList<KeyboardForm> allForms();

    // best guess from the operating system (keyboard type, keyboard / system language)
    KeyboardForm guessSystemForm();
    // a key that only exists on one form proves it (86: ISO; 89, 92, 93, 94, 124: JIS)
    bool formFromKeycode(int keycode, KeyboardForm *form);
}

#endif // KEYBOARD_LAYOUT_H
