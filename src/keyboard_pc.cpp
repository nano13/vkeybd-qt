#include "keyboard_pc.h"

#include <QFontMetrics>
#include <QPainter>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QStringList>
#include <QSet>

#include <algorithm>

namespace
{
// height of the key rows in pixels (the width follows the widget)
// without the sounding-notes line: the original compact keyboard
constexpr int ROW_HEIGHT_COMPACT = 26;
// with it: the note rows need room for the extra line, the pedal row less
constexpr int ROW_HEIGHT_SOUNDING = 36;
constexpr int ROW_HEIGHT_FUNCTIONS = 26;

int pitchClass(int note)
{
    return ((note % 12) + 12) % 12;
}

bool isAccidental(int pitch_class)
{
    return pitch_class == 1 || pitch_class == 3 || pitch_class == 6 || pitch_class == 8 || pitch_class == 10;
}

// MIDI 60 = C4
int octaveNumber(int note)
{
    return note / 12 - 1;
}

QString pitchName(int note)
{
    static const char *names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return QString::fromLatin1(names[pitchClass(note)]);
}
}

// ---------------------------------------------------------------------------
// PcKeyButton

PcKeyButton::PcKeyButton(QWidget *parent) : QPushButton(parent)
{
}

void PcKeyButton::setKeyPitchClass(int pitch_class)
{
    if (this->key_pitch_class == pitch_class)
        return;
    this->key_pitch_class = pitch_class;
    updateToolTip();
    update();
}

void PcKeyButton::setSoundingNotes(const QList<int> &notes)
{
    if (this->sounding_notes == notes)
        return;
    this->sounding_notes = notes;
    updateToolTip();
    update();
}

QList<int> PcKeyButton::soundingNotes() const
{
    return this->sounding_notes;
}

void PcKeyButton::setSelected(bool selected)
{
    if (this->selected == selected)
        return;
    this->selected = selected;
    update();
}

void PcKeyButton::setMarked(bool marked)
{
    if (this->marked == marked)
        return;
    this->marked = marked;
    update();
}

void PcKeyButton::setShowSoundingLine(bool show)
{
    if (this->show_sounding_line == show)
        return;
    this->show_sounding_line = show;
    update();
}

QColor PcKeyButton::pressedTextColor() const
{
    return this->pressed_text_color;
}

void PcKeyButton::setPressedTextColor(const QColor &color)
{
    this->pressed_text_color = color;
    update();
}

QString PcKeyButton::noteName(int midi_note)
{
    return pitchName(midi_note) + QString::number(octaveNumber(midi_note));
}

QFont PcKeyButton::smallFont() const
{
    QFont small = font();
    small.setBold(false);
    if (small.pointSizeF() > 0)
        small.setPointSizeF(qMax(5.5, small.pointSizeF() * 0.85));
    else if (small.pixelSize() > 0)
        small.setPixelSize(qMax(7, qRound(small.pixelSize() * 0.85)));
    return small;
}

QString PcKeyButton::soundingText(int width) const
{
    if (this->sounding_notes.isEmpty() || this->key_pitch_class < 0)
        return QString();

    const QFontMetrics metrics(smallFont());
    const QString ellipsis(QChar(0x2026));

    QSet<int> pitch_classes;
    QStringList octaves;
    for (int note : this->sounding_notes)
    {
        pitch_classes.insert(pitchClass(note));
        octaves.append(QString::number(octaveNumber(note)));
    }

    QStringList candidates;
    if (pitch_classes.size() == 1)
    {
        // one note name in one or more octaves: the octave numbers, with the name
        // only if it is not the key's own (shifted by a non-octave amount)
        const int sounding_class = pitch_classes.values().at(0);
        const QString name = sounding_class == this->key_pitch_class ? QString() : pitchName(sounding_class);
        const QString prefix = name.isEmpty() ? QString() : name + QLatin1Char(' ');
        candidates.append(prefix + octaves.join(QLatin1Char(' ')));
        if (octaves.size() > 1)
            candidates.append(prefix + octaves.first() + ellipsis + octaves.last());
        if (!name.isEmpty())
            candidates.append(name);
    }
    else
    {
        // different note names (e.g. a channel shifted by a fifth): full names
        QStringList names;
        for (int note : this->sounding_notes)
            names.append(noteName(note));
        candidates.append(names.join(QLatin1Char(' ')));
        candidates.append(names.first() + ellipsis + names.last());
        candidates.append(names.first() + ellipsis);
    }

    for (const QString &candidate : candidates)
    {
        if (metrics.horizontalAdvance(candidate) <= width)
            return candidate;
    }
    // nothing fits: the shortest one, the tooltip has everything
    return candidates.last();
}

void PcKeyButton::updateToolTip()
{
    if (this->key_pitch_class < 0)
    {
        setToolTip(QString());
        return;
    }
    if (this->sounding_notes.isEmpty())
    {
        setToolTip("Sounds: nothing (no activated channel covers this key)");
        return;
    }
    QStringList names;
    for (int note : this->sounding_notes)
        names.append(noteName(note));
    setToolTip("Sounds: " + names.join(", ") + "  (C4 = middle C)");
}

void PcKeyButton::paintEvent(QPaintEvent *event)
{
    const QString line = this->show_sounding_line ? soundingText(width() - 2) : QString();
    if (line.isEmpty())
    {
        // exactly the normal button, as styled (QSS)
        QPushButton::paintEvent(event);
    }
    else
    {
        QStylePainter painter(this);
        QStyleOptionButton option;
        initStyleOption(&option);
        const QFont small = smallFont();
        const int small_height = QFontMetrics(small).height();

        // background and frame over the whole key, the note names as styled,
        // just moved up to make room for the small line
        painter.drawControl(QStyle::CE_PushButtonBevel, option);
        QStyleOptionButton label = option;
        label.rect = option.rect.adjusted(0, 0, 0, -small_height);
        painter.drawControl(QStyle::CE_PushButtonLabel, label);

        const QColor color = isDown() ? this->pressed_text_color : palette().color(QPalette::ButtonText);
        painter.setPen(color);
        painter.setFont(small);
        const QRect area = rect().adjusted(1, 1, -1, -1);
        painter.drawText(QRect(area.left(), area.bottom() + 1 - small_height, area.width(), small_height),
                         Qt::AlignCenter, line);
    }

    // keymap editor: the selected key gets a frame, keys set by hand a corner mark
    if (this->selected || this->marked)
    {
        QPainter overlay(this);
        overlay.setRenderHint(QPainter::Antialiasing);
        if (this->marked)
        {
            const int size = qMax(6, height() / 4);
            QPolygon corner;
            corner << QPoint(width() - size, 0) << QPoint(width(), 0) << QPoint(width(), size);
            overlay.setPen(Qt::NoPen);
            overlay.setBrush(QColor(0xff, 0x8c, 0x00));
            overlay.drawPolygon(corner);
        }
        if (this->selected)
        {
            overlay.setBrush(Qt::NoBrush);
            overlay.setPen(QPen(QColor(0xff, 0x30, 0x30), 3));
            overlay.drawRect(rect().adjusted(1, 1, -2, -2));
        }
    }
}

// ---------------------------------------------------------------------------
// KeyboardPC

KeyboardPC::KeyboardPC(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setupKeys();
}

void KeyboardPC::setupKeys()
{
    this->key_layout = KeyboardLayout::keys(this->form);
    this->key_position.clear();
    this->midi_codes.clear();
    this->active_midi_codes.clear();
    this->physical_key_down.clear();

    for (int row = 0; row < this->key_layout.size(); ++row)
    {
        QList<int> notes;
        for (int col = 0; col < this->key_layout.at(row).size(); ++col)
        {
            const PcKeyDef &key = this->key_layout.at(row).at(col);
            notes.append(key.role == PcKeyRole::Note ? this->keymap.note(key.keycode, this->form) : -1);
            this->key_position.insert(key.keycode, QPoint(col, row));
        }
        this->midi_codes.append(notes);
        this->active_midi_codes.append(QList<int>(notes.size(), -1));
        this->physical_key_down.append(QList<bool>(notes.size(), false));
    }

    setMinimumWidth(qRound(360 * this->scale));
    updateHeight();

    drawButtons();
    updateLabels();
    updateEditMarks();
    layoutButtons();
}

QString KeyboardPC::noteLabel(int note)
{
    if (note < 0)
        return QString();
    // both spellings of the black keys: flat above, sharp below
    static const char *labels[12] = {"C", "Db\nC#", "D", "Eb\nD#", "E", "F", "Gb\nF#", "G", "Ab\nG#", "A", "Bb\nA#", "B"};
    return QString::fromLatin1(labels[pitchClass(note)]);
}

QString KeyboardPC::roleLabel(PcKeyRole role)
{
    switch (role)
    {
        case PcKeyRole::OctaveDown:  return "<<";
        case PcKeyRole::OctaveUp:    return ">>";
        case PcKeyRole::Soft:        return "soft";
        case PcKeyRole::Sostenuto:   return "soste\nnuto";
        case PcKeyRole::Sustain:     return "sustain";
        case PcKeyRole::TabModifier: return "+tab";
        case PcKeyRole::Note:        break;
    }
    return QString();
}

QString KeyboardPC::styleName(PcKeyRole role, int note)
{
    switch (role)
    {
        case PcKeyRole::OctaveDown:
        case PcKeyRole::OctaveUp:    return "pc_blue";
        case PcKeyRole::Soft:
        case PcKeyRole::Sostenuto:
        case PcKeyRole::Sustain:     return "pc_gold";
        case PcKeyRole::TabModifier: return "pc_grey";
        case PcKeyRole::Note:        break;
    }
    if (note >= 0 && isAccidental(pitchClass(note)))
        return "pc_black";
    return "pc_white";
}

void KeyboardPC::drawButtons()
{
    for (int row = 0; row < this->key_layout.size(); row++)
    {
        QList<PcKeyButton*> list_of_buttons_row;
        for (int col = 0; col < this->key_layout.at(row).size(); col++)
        {
            PcKeyButton *button = new PcKeyButton(this);
            const PcKeyRole role = this->key_layout.at(row).at(col).role;
            const int keycode = this->key_layout.at(row).at(col).keycode;
            button->setShowSoundingLine(this->show_sounding_notes);

            connect(button, &QPushButton::pressed, this, [this, row, col, role, keycode]{
                if (this->edit_mode)
                {
                    emit keySelected(keycode);
                    return;
                }
                if (role == PcKeyRole::Note)
                    pressKey(row, col);
                else
                    rolePressed(role, true);
            });
            connect(button, &QPushButton::released, this, [this, row, col, role]{
                if (this->edit_mode)
                {
                    updateButtonVisual(row, col);
                    return;
                }
                if (role == PcKeyRole::Note)
                {
                    releaseKey(row, col);
                }
                else
                {
                    rolePressed(role, false);
                    updateButtonVisual(row, col);
                }
            });

            button->show();
            list_of_buttons_row.append(button);
        }
        this->list_of_buttons.append(list_of_buttons_row);
    }
}

void KeyboardPC::releaseAllKeys()
{
    // the notes of the keys still down must be released before their keys change
    for (int row = 0; row < this->active_midi_codes.size(); ++row)
    {
        for (int col = 0; col < this->active_midi_codes.at(row).size(); ++col)
        {
            const int old_note = this->active_midi_codes.at(row).at(col);
            this->physical_key_down[row][col] = false;
            this->active_midi_codes[row][col] = -1;
            if (old_note >= 0)
                emit MIDIRelease(old_note);
        }
    }
}

void KeyboardPC::setKeyboardForm(KeyboardForm form)
{
    if (form == this->form)
        return;

    releaseAllKeys();
    for (const QList<PcKeyButton*> &row : this->list_of_buttons)
    {
        for (PcKeyButton *button : row)
            delete button;
    }
    this->list_of_buttons.clear();

    this->form = form;
    setupKeys();
    updateButtonVisuals();
    emit midiMappingChanged();
}

KeyboardForm KeyboardPC::keyboardForm() const
{
    return this->form;
}

void KeyboardPC::setAutoForm(bool automatic)
{
    this->auto_form = automatic;
}

bool KeyboardPC::autoForm() const
{
    return this->auto_form;
}

QList<int> KeyboardPC::keycodes() const
{
    QList<int> result;
    for (const QList<PcKeyDef> &row : this->key_layout)
    {
        for (const PcKeyDef &key : row)
            result.append(key.keycode);
    }
    return result;
}

bool KeyboardPC::isNoteKey(int keycode) const
{
    if (!this->key_position.contains(keycode))
        return false;
    const QPoint position = this->key_position.value(keycode);
    return this->key_layout.at(position.y()).at(position.x()).role == PcKeyRole::Note;
}

void KeyboardPC::setKeymap(const Keymap &keymap)
{
    this->keymap = keymap;
    QList<QList<int>> mapping;
    for (const QList<PcKeyDef> &row : this->key_layout)
    {
        QList<int> notes;
        for (const PcKeyDef &key : row)
            notes.append(key.role == PcKeyRole::Note ? keymap.note(key.keycode, this->form) : -1);
        mapping.append(notes);
    }
    setMidiMapping(mapping);
    updateEditMarks();
}

Keymap KeyboardPC::currentKeymap() const
{
    return this->keymap;
}

void KeyboardPC::setEditMode(bool edit)
{
    if (this->edit_mode == edit)
        return;
    releaseAllKeys();
    this->edit_mode = edit;
    updateButtonVisuals();
    updateEditMarks();
}

void KeyboardPC::setSelectedKey(int keycode)
{
    this->selected_keycode = keycode;
    updateEditMarks();
}

int KeyboardPC::selectedKey() const
{
    return this->selected_keycode;
}

void KeyboardPC::updateEditMarks()
{
    for (int row = 0; row < this->list_of_buttons.size(); ++row)
    {
        for (int col = 0; col < this->list_of_buttons.at(row).size(); ++col)
        {
            const int keycode = this->key_layout.at(row).at(col).keycode;
            PcKeyButton *button = this->list_of_buttons.at(row).at(col);
            button->setSelected(this->edit_mode && keycode == this->selected_keycode);
            button->setMarked(this->edit_mode && this->keymap.overrides.contains(keycode));
        }
    }
}

void KeyboardPC::setScale(qreal scale)
{
    this->scale = scale;
    setMinimumWidth(qRound(360 * this->scale));
    updateHeight();
    layoutButtons();
}

void KeyboardPC::rolePressed(PcKeyRole role, bool pressed)
{
    switch (role)
    {
        case PcKeyRole::OctaveDown:
            if (pressed)
                emit octaveDown();
            break;
        case PcKeyRole::OctaveUp:
            if (pressed)
                emit octaveUp();
            break;
        case PcKeyRole::Soft:
            emit soft(pressed);
            break;
        case PcKeyRole::Sostenuto:
            emit sostenuto(pressed);
            break;
        case PcKeyRole::Sustain:
            emit sustain(pressed);
            break;
        case PcKeyRole::TabModifier:
        case PcKeyRole::Note:
            // Menu is a modifier for the F-keys, a click has nothing to modify
            break;
    }
}

void KeyboardPC::pressKey(int row, int col)
{
    this->physical_key_down[row][col] = true;
    const int note = this->midi_codes.at(row).at(col);
    this->active_midi_codes[row][col] = note;
    if (note >= 0)
        emit MIDIPress(note);
}

void KeyboardPC::releaseKey(int row, int col)
{
    this->physical_key_down[row][col] = false;
    const int note = this->active_midi_codes.at(row).at(col);
    this->active_midi_codes[row][col] = -1;
    updateButtonVisual(row, col);
    if (note >= 0)
        emit MIDIRelease(note);
}

void KeyboardPC::updateLabels()
{
    for (int row = 0; row < this->list_of_buttons.size(); ++row)
    {
        for (int col = 0; col < this->list_of_buttons.at(row).size(); ++col)
        {
            PcKeyButton *button = this->list_of_buttons.at(row).at(col);
            const PcKeyRole role = this->key_layout.at(row).at(col).role;
            const int note = this->midi_codes.at(row).at(col);

            button->setText(role == PcKeyRole::Note ? noteLabel(note) : roleLabel(role));
            button->setKeyPitchClass(role == PcKeyRole::Note && note >= 0 ? pitchClass(note) : -1);

            // the pc_* QSS rules decide colors and font (size, bold note names):
            // re-polish, so they always apply, whenever the name was set
            const QString style = styleName(role, note);
            if (button->objectName() != style)
            {
                button->setObjectName(style);
                button->style()->unpolish(button);
                button->style()->polish(button);
            }
        }
    }
    updateSoundingNotes();
}

void KeyboardPC::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutButtons();
}

int KeyboardPC::rowHeight(int row) const
{
    bool has_notes = false;
    for (const PcKeyDef &key : this->key_layout.at(row))
    {
        if (key.role == PcKeyRole::Note)
            has_notes = true;
    }
    int height = ROW_HEIGHT_COMPACT;
    if (this->show_sounding_notes)
        height = has_notes ? ROW_HEIGHT_SOUNDING : ROW_HEIGHT_FUNCTIONS;
    return qRound(height * this->scale);
}

void KeyboardPC::updateHeight()
{
    int total = 0;
    for (int row = 0; row < this->key_layout.size(); row++)
        total += rowHeight(row);
    setFixedHeight(total);
}

void KeyboardPC::setShowSoundingNotes(bool show)
{
    if (this->show_sounding_notes == show)
        return;
    this->show_sounding_notes = show;
    for (const QList<PcKeyButton*> &row : this->list_of_buttons)
    {
        for (PcKeyButton *button : row)
            button->setShowSoundingLine(show);
    }
    updateHeight();
    layoutButtons();
}

bool KeyboardPC::showSoundingNotes() const
{
    return this->show_sounding_notes;
}

void KeyboardPC::layoutButtons()
{
    const int row_count = this->key_layout.size();
    int total_height = 0;
    for (int row = 0; row < row_count; row++)
        total_height += rowHeight(row);
    if (total_height <= 0)
        return;

    int y = 0;
    for (int row = 0; row < row_count; row++)
    {
        const QList<PcKeyDef> &keys = this->key_layout.at(row);
        int row_width = 0;
        for (const PcKeyDef &key : keys)
            row_width += key.width;

        // the rows keep their proportions if the widget is not exactly as high as wanted
        const int top = qRound(static_cast<qreal>(y) * height() / total_height);
        y += rowHeight(row);
        const int bottom = qRound(static_cast<qreal>(y) * height() / total_height);
        int x = 0;

        for (int col = 0; col < keys.size(); col++)
        {
            const int left = qRound(static_cast<qreal>(x) * width() / row_width);
            x += keys.at(col).width;
            const int right = qRound(static_cast<qreal>(x) * width() / row_width);
            this->list_of_buttons.at(row).at(col)->setGeometry(left, top, right - left, bottom - top);
        }
    }
}

QList<QList<int>> KeyboardPC::midiMapping() const
{
    return this->midi_codes;
}

QList<int> KeyboardPC::midiNotes() const
{
    QList<int> notes;
    for (const QList<int> &row : this->midi_codes)
    {
        for (int note : row)
        {
            if (note >= 0 && !notes.contains(note))
                notes.append(note);
        }
    }
    std::sort(notes.begin(), notes.end());
    return notes;
}

bool KeyboardPC::setMidiMapping(const QList<QList<int>> &mapping)
{
    if (mapping.size() != this->midi_codes.size())
        return false;

    for (int row = 0; row < mapping.size(); ++row)
    {
        if (mapping.at(row).size() != this->midi_codes.at(row).size())
            return false;
        for (int col = 0; col < mapping.at(row).size(); ++col)
        {
            const int note = mapping.at(row).at(col);
            if (note < -1 || note > 127)
                return false;
            // function keys (pedals, octave) have no note
            if (this->key_layout.at(row).at(col).role != PcKeyRole::Note && note != -1)
                return false;
        }
    }

    if (mapping == this->midi_codes)
        return true;

    // A remap while keys are down must release the notes from the OLD mapping.
    releaseAllKeys();

    this->midi_codes = mapping;
    updateLabels();
    updateButtonVisuals();
    emit midiMappingChanged();
    return true;
}

void KeyboardPC::keyDownRaw(int keycode)
{
    if (!this->key_position.contains(keycode))
    {
        // a key that only exists on another keyboard form proves that form
        KeyboardForm detected = this->form;
        if (!this->auto_form || !KeyboardLayout::formFromKeycode(keycode, &detected) || detected == this->form)
            return;
        setKeyboardForm(detected);
        emit keyboardFormDetected(detected);
        if (!this->key_position.contains(keycode))
            return;
    }
    const QPoint position = this->key_position.value(keycode);
    const int row = position.y();
    const int col = position.x();

    if (this->physical_key_down.at(row).at(col))
        return;

    this->physical_key_down[row][col] = true;
    updateButtonVisual(row, col);
    if (this->edit_mode)
        return;
    const int note = this->midi_codes.at(row).at(col);
    this->active_midi_codes[row][col] = note;
    if (note >= 0)
        emit MIDIPress(note);
}

void KeyboardPC::keyUpRaw(int keycode)
{
    if (!this->key_position.contains(keycode))
        return;
    const QPoint position = this->key_position.value(keycode);
    const int row = position.y();
    const int col = position.x();

    if (!this->physical_key_down.at(row).at(col))
        return;

    this->physical_key_down[row][col] = false;
    const int note = this->active_midi_codes.at(row).at(col);
    this->active_midi_codes[row][col] = -1;
    updateButtonVisual(row, col);
    if (note >= 0)
        emit MIDIRelease(note);
}

void KeyboardPC::allKeysUp()
{
    this->midi_held_notes.clear();
    for (int row = 0; row < this->list_of_buttons.size(); ++row)
    {
        for (int col = 0; col < this->list_of_buttons.at(row).size(); ++col)
        {
            this->physical_key_down[row][col] = false;
            this->active_midi_codes[row][col] = -1;
            updateButtonVisual(row, col);
        }
    }
}

void KeyboardPC::setMidiNoteDown(int note, bool down)
{
    if (down)
        this->midi_held_notes.insert(note);
    else
        this->midi_held_notes.remove(note);
    updateButtonVisuals();
}

void KeyboardPC::clearMidiNoteHighlights()
{
    this->midi_held_notes.clear();
    updateButtonVisuals();
}

void KeyboardPC::setSoundingNotes(const QHash<int, QList<int>> &sounding)
{
    this->sounding = sounding;
    updateSoundingNotes();
}

void KeyboardPC::updateSoundingNotes()
{
    for (int row = 0; row < this->list_of_buttons.size(); ++row)
    {
        for (int col = 0; col < this->list_of_buttons.at(row).size(); ++col)
        {
            const int note = this->midi_codes.at(row).at(col);
            const bool is_note = this->key_layout.at(row).at(col).role == PcKeyRole::Note && note >= 0;
            this->list_of_buttons.at(row).at(col)->setSoundingNotes(is_note ? this->sounding.value(note) : QList<int>());
        }
    }
}

void KeyboardPC::updateButtonVisual(int row, int col)
{
    const int note = this->midi_codes.at(row).at(col);
    const bool highlighted = this->physical_key_down.at(row).at(col)
        || (note >= 0 && this->midi_held_notes.contains(note));
    this->list_of_buttons.at(row).at(col)->setDown(highlighted);
}

void KeyboardPC::updateButtonVisuals()
{
    for (int row = 0; row < this->list_of_buttons.size(); ++row)
    {
        for (int col = 0; col < this->list_of_buttons.at(row).size(); ++col)
            updateButtonVisual(row, col);
    }
}
