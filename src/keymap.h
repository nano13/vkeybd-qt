#ifndef KEYMAP_H
#define KEYMAP_H

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>

#include "keyboard_layout.h"

enum class KeymapPattern
{
    None,           // no pattern: only the keys set by hand sound
    AccordionC,     // chromatic button accordion, C-system (the default)
    AccordionB,     // the same, B-system (bayan): rows a semitone apart the other way
    Janko,          // whole tones in each row, the rows a semitone apart
    WickiHayden,    // whole tones in each row, a fourth/fifth between the rows
    Piano           // tracker style: Z row + A row and Q row + number row as two piano octaves
};

/*
 * What the PC keys play: a pattern fills the keys, single keys can then be set
 * by hand (overrides). The overrides are stored per key code, so they stay the
 * same on every keyboard form; the pattern follows the form (on an ANSI keyboard
 * the keys are where they really are).
 *
 * Notes are the codes of the PC keyboard (MidiEngine source keys). With the
 * default octave box 48 a code sounds 36 semitones higher, so code 24 is C4
 * (middle C) -- that is how the notes are named in the editor and in the files.
 */
struct Keymap
{
    static constexpr int NOTE_MIN = 0;
    static constexpr int NOTE_MAX = 91;
    // code + 36 = the MIDI note it plays with the default octave box
    static constexpr int REFERENCE_OFFSET = 36;

    QString name;
    KeymapPattern pattern = KeymapPattern::AccordionC;
    // note of the pattern's first key (left of 1; for the piano pattern: Z)
    int start = 8;
    // keycode -> note, -1: silent
    QMap<int, int> overrides;

    // the notes of the pattern alone (only the keys it gives a note)
    QMap<int, int> patternNotes(KeyboardForm form) const;
    // pattern plus overrides; -1 or missing: silent
    int note(int keycode, KeyboardForm form) const;

    bool operator==(const Keymap &other) const;
    bool operator!=(const Keymap &other) const;

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &json, Keymap *keymap, QString *error);
    bool saveFile(const QString &path, QString *error) const;
    static bool loadFile(const QString &path, Keymap *keymap, QString *error);

    static QList<KeymapPattern> allPatterns();
    static QString patternName(KeymapPattern pattern);     // "accordion-c", ...
    static QString patternTitle(KeymapPattern pattern);    // "Accordion C-system", ...
    static bool patternFromName(const QString &name, KeymapPattern *pattern);
    static int defaultStart(KeymapPattern pattern);

    // "G#2", "C4" (code 24); empty for -1
    static QString noteName(int note);
    static bool noteFromName(const QString &name, int *note);
};

#endif // KEYMAP_H
