#include "keymap.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSaveFile>

namespace
{
const char *FILE_TAG = "keydesk-midi-keymap";
constexpr int FILE_VERSION = 1;

// tracker style: offsets from the pattern's start (C) per key code
// Z row = white keys, A row = black keys of the lower octave; Q row + number row the upper one
QMap<int, int> pianoOffsets()
{
    return {
        {44, 0}, {31, 1}, {45, 2}, {32, 3}, {46, 4}, {47, 5}, {34, 6}, {48, 7}, {35, 8}, {49, 9},
        {36, 10}, {50, 11}, {51, 12}, {38, 13}, {52, 14}, {39, 15}, {53, 16},
        {16, 12}, {3, 13}, {17, 14}, {4, 15}, {18, 16}, {19, 17}, {6, 18}, {20, 19}, {7, 20}, {21, 21},
        {8, 22}, {22, 23}, {23, 24}, {10, 25}, {24, 26}, {11, 27}, {25, 28}, {26, 29}, {13, 30}, {27, 31}
    };
}

// grid patterns: note = start + row offset + step * column
bool gridPattern(KeymapPattern pattern, QList<int> *row_offsets, int *step)
{
    switch (pattern)
    {
        case KeymapPattern::AccordionC:
            *row_offsets = {0, 1, 2, 0};    // rows of minor thirds, a semitone apart; row 4 = row 1
            *step = 3;
            return true;
        case KeymapPattern::AccordionB:
            *row_offsets = {0, -1, -2, 0};
            *step = 3;
            return true;
        case KeymapPattern::Janko:
            *row_offsets = {0, 1, 0, 1};
            *step = 2;
            return true;
        case KeymapPattern::WickiHayden:
            *row_offsets = {0, -5, -10, -15};
            *step = 2;
            return true;
        case KeymapPattern::None:
        case KeymapPattern::Piano:
            break;
    }
    return false;
}

bool validNote(int note)
{
    return note >= Keymap::NOTE_MIN && note <= Keymap::NOTE_MAX;
}
}

QMap<int, int> Keymap::patternNotes(KeyboardForm form) const
{
    QMap<int, int> notes;

    if (this->pattern == KeymapPattern::Piano)
    {
        const QMap<int, int> offsets = pianoOffsets();
        for (QMap<int, int>::const_iterator it = offsets.cbegin(); it != offsets.cend(); ++it)
        {
            const int note = this->start + it.value();
            if (validNote(note))
                notes.insert(it.key(), note);
        }
        return notes;
    }

    QList<int> row_offsets;
    int step = 0;
    if (!gridPattern(this->pattern, &row_offsets, &step))
        return notes;

    const QList<QList<PcKeyDef>> rows = KeyboardLayout::keys(form);
    for (int row = 0; row < rows.size() && row < row_offsets.size(); ++row)
    {
        for (const PcKeyDef &key : rows.at(row))
        {
            if (key.role != PcKeyRole::Note || key.grid_col < 0)
                continue;
            const int note = this->start + row_offsets.at(row) + step * key.grid_col;
            if (validNote(note))
                notes.insert(key.keycode, note);
        }
    }
    return notes;
}

int Keymap::note(int keycode, KeyboardForm form) const
{
    if (this->overrides.contains(keycode))
        return this->overrides.value(keycode);
    return patternNotes(form).value(keycode, -1);
}

bool Keymap::operator==(const Keymap &other) const
{
    return this->name == other.name && this->pattern == other.pattern
        && this->start == other.start && this->overrides == other.overrides;
}

bool Keymap::operator!=(const Keymap &other) const
{
    return !(*this == other);
}

QJsonObject Keymap::toJson() const
{
    QJsonObject keys;
    for (QMap<int, int>::const_iterator it = this->overrides.cbegin(); it != this->overrides.cend(); ++it)
    {
        // null: silent
        keys.insert(QString::number(it.key()), it.value() < 0 ? QJsonValue() : QJsonValue(noteName(it.value())));
    }

    QJsonObject json;
    json.insert(FILE_TAG, FILE_VERSION);
    if (!this->name.isEmpty())
        json.insert("name", this->name);
    json.insert("pattern", patternName(this->pattern));
    json.insert("start", noteName(this->start));
    json.insert("keys", keys);
    return json;
}

bool Keymap::fromJson(const QJsonObject &json, Keymap *keymap, QString *error)
{
    if (json.value(FILE_TAG).toInt() != FILE_VERSION)
    {
        *error = "not a KeyDeskMIDI keymap (or a newer version)";
        return false;
    }

    Keymap result;
    result.name = json.value("name").toString();
    if (!patternFromName(json.value("pattern").toString(), &result.pattern))
    {
        *error = "unknown pattern: " + json.value("pattern").toString();
        return false;
    }
    if (!noteFromName(json.value("start").toString(), &result.start))
    {
        *error = "invalid start note: " + json.value("start").toString();
        return false;
    }

    const QJsonObject keys = json.value("keys").toObject();
    for (QJsonObject::const_iterator it = keys.constBegin(); it != keys.constEnd(); ++it)
    {
        bool ok = false;
        const int keycode = it.key().toInt(&ok);
        if (!ok || keycode < 0)
        {
            *error = "invalid key code: " + it.key();
            return false;
        }
        int note = -1;
        if (!it.value().isNull() && !noteFromName(it.value().toString(), &note))
        {
            *error = "invalid note for key " + it.key() + ": " + it.value().toString();
            return false;
        }
        result.overrides.insert(keycode, note);
    }

    *keymap = result;
    return true;
}

bool Keymap::saveFile(const QString &path, QString *error) const
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        *error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        *error = file.errorString();
        return false;
    }
    return true;
}

bool Keymap::loadFile(const QString &path, Keymap *keymap, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        *error = file.errorString();
        return false;
    }
    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (!document.isObject())
    {
        *error = parse_error.errorString();
        return false;
    }
    return fromJson(document.object(), keymap, error);
}

QList<KeymapPattern> Keymap::allPatterns()
{
    return {KeymapPattern::AccordionC, KeymapPattern::AccordionB, KeymapPattern::Janko,
            KeymapPattern::WickiHayden, KeymapPattern::Piano, KeymapPattern::None};
}

QString Keymap::patternName(KeymapPattern pattern)
{
    switch (pattern)
    {
        case KeymapPattern::None:        return "none";
        case KeymapPattern::AccordionC:  return "accordion-c";
        case KeymapPattern::AccordionB:  return "accordion-b";
        case KeymapPattern::Janko:       return "janko";
        case KeymapPattern::WickiHayden: return "wicki-hayden";
        case KeymapPattern::Piano:       return "piano";
    }
    return "none";
}

QString Keymap::patternTitle(KeymapPattern pattern)
{
    switch (pattern)
    {
        case KeymapPattern::None:        return "None (only keys set by hand)";
        case KeymapPattern::AccordionC:  return "Accordion C-system";
        case KeymapPattern::AccordionB:  return "Accordion B-system (bayan)";
        case KeymapPattern::Janko:       return "Jankó";
        case KeymapPattern::WickiHayden: return "Wicki-Hayden";
        case KeymapPattern::Piano:       return "Piano (tracker style)";
    }
    return QString();
}

bool Keymap::patternFromName(const QString &name, KeymapPattern *pattern)
{
    const QList<KeymapPattern> patterns = allPatterns();
    for (const KeymapPattern candidate : patterns)
    {
        if (patternName(candidate) == name)
        {
            *pattern = candidate;
            return true;
        }
    }
    return false;
}

int Keymap::defaultStart(KeymapPattern pattern)
{
    switch (pattern)
    {
        case KeymapPattern::AccordionC:  return 8;     // G#2 ... C6, as always
        case KeymapPattern::AccordionB:  return 10;
        case KeymapPattern::Janko:       return 12;
        case KeymapPattern::WickiHayden: return 27;    // lowest key (left Shift): C3
        case KeymapPattern::Piano:       return 12;    // Z: C3
        case KeymapPattern::None:        break;
    }
    return 12;
}

QString Keymap::noteName(int note)
{
    if (note < 0)
        return QString();
    static const char *names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const int midi = note + REFERENCE_OFFSET;
    return QString::fromLatin1(names[midi % 12]) + QString::number(midi / 12 - 1);
}

bool Keymap::noteFromName(const QString &name, int *note)
{
    static const QRegularExpression pattern("^([A-Ga-g])([#b]?)(-?\\d+)$");
    const QRegularExpressionMatch match = pattern.match(name.trimmed());
    if (!match.hasMatch())
        return false;

    static const int naturals[7] = {9, 11, 0, 2, 4, 5, 7};   // A B C D E F G
    int pitch = naturals[match.captured(1).toUpper().at(0).unicode() - 'A'];
    if (match.captured(2) == "#")
        pitch += 1;
    else if (match.captured(2) == "b")
        pitch -= 1;

    const int midi = (match.captured(3).toInt() + 1) * 12 + pitch;
    const int result = midi - REFERENCE_OFFSET;
    if (!validNote(result))
        return false;
    *note = result;
    return true;
}
