#include "keyboard_piano.h"

#include <QSizePolicy>
#include <QtMath>

#include <algorithm>

KeyboardPiano::KeyboardPiano(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(this->keyboard_height);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

bool KeyboardPiano::isBlackKey(int midicode)
{
    switch (midicode % 12)
    {
        case 1:
        case 3:
        case 6:
        case 8:
        case 10:
            return true;
        default:
            return false;
    }
}

void KeyboardPiano::setMidiNotes(const QList<int> &notes)
{
    QList<int> unique_notes;
    for (int note : notes)
    {
        if (note >= 0 && note <= 127 && !unique_notes.contains(note))
            unique_notes.append(note);
    }
    std::sort(unique_notes.begin(), unique_notes.end());

    if (unique_notes == midiNotes())
        return;

    // Rebuild only the keys supplied by the PC keyboard's mapping.
    for (ButtonPiano *button : this->list_of_keys_midi)
        delete button;
    this->list_of_keys_midi.clear();

    // White keys must be behind the black keys, regardless of MIDI note order.
    for (int pass = 0; pass < 2; ++pass)
    {
        const bool black_keys = (pass == 1);
        for (int note : unique_notes)
        {
            if (isBlackKey(note) != black_keys)
                continue;

            ButtonPiano *button = new ButtonPiano(this);
            button->setObjectName(QStringLiteral("piano_note_%1").arg(note));
            if (black_keys)
                colorizeBlackKeys(button);
            else
                colorizeWhiteKeys(button);
            this->list_of_keys_midi.insert(note, button);
            button->show();
        }
    }

    layoutKeys();
}

QList<int> KeyboardPiano::midiNotes() const
{
    return this->list_of_keys_midi.keys();
}

void KeyboardPiano::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutKeys();
}

void KeyboardPiano::layoutKeys()
{
    const QList<int> notes = midiNotes();
    QList<int> white_notes;
    for (int note : notes)
    {
        if (!isBlackKey(note))
            white_notes.append(note);
    }

    const int white_count = white_notes.size();
    if (white_count == 0)
    {
        // A future custom mapping may contain only black notes.
        for (int i = 0; i < notes.size(); ++i)
        {
            const int left = qRound(static_cast<qreal>(i) * width() / notes.size());
            const int right = qRound(static_cast<qreal>(i + 1) * width() / notes.size());
            this->list_of_keys_midi.value(notes.at(i))->setGeometry(left, 0, right - left, height());
        }
        return;
    }

    // Distribute the existing white keys across the complete available width.
    for (int i = 0; i < white_count; ++i)
    {
        const int left = qRound(static_cast<qreal>(i) * width() / white_count);
        const int right = qRound(static_cast<qreal>(i + 1) * width() / white_count);
        this->list_of_keys_midi.value(white_notes.at(i))->setGeometry(left, 0, right - left, height());
    }

    // Black keys lie at the boundary before the next higher white note.
    // Clamp at the edges for partial octaves (the default begins with G#).
    const int black_width = qMax(1, qRound(static_cast<qreal>(width()) / white_count * 14 / 18));
    const int black_height = qRound(height() * 50.0 / 80.0);
    for (int note : notes)
    {
        if (!isBlackKey(note))
            continue;

        int next_white = 0;
        while (next_white < white_count && white_notes.at(next_white) < note)
            ++next_white;

        const int boundary = qRound(static_cast<qreal>(next_white) * width() / white_count);
        const int left = qBound(0, boundary - black_width / 2, qMax(0, width() - black_width));
        this->list_of_keys_midi.value(note)->setGeometry(left, 0, black_width, black_height);
    }
}

void KeyboardPiano::colorizeBlackKeys(ButtonPiano *button)
{
    const QString stylesheet = "QPushButton {"
                               "  color: white;"
                               "  background-color: black;"
                               "  border-style: outset;"
                               "  border-width: 1px;"
                               "  border-radius: 0px;"
                               "}"
                               "QPushButton:pressed {"
                               "  color: white;"
                               "  background-color: blue;"
                               "}";
    button->setStyleSheet(stylesheet);
}

void KeyboardPiano::colorizeWhiteKeys(ButtonPiano *button)
{
    const QString stylesheet = "QPushButton {"
                               "  color: black;"
                               "  background-color: white;"
                               "  border-style: outset;"
                               "  border-width: 1px;"
                               "  border-color: black;"
                               "  border-radius: 0px;"
                               "}"
                               "QPushButton:pressed {"
                               "  color: black;"
                               "  background-color: blue;"
                               "}";
    button->setStyleSheet(stylesheet);
}

void KeyboardPiano::keyPressed(int midicode)
{
    ButtonPiano *button = this->list_of_keys_midi.value(midicode, nullptr);
    if (button)
        button->press();
}

void KeyboardPiano::keyReleased(int midicode)
{
    ButtonPiano *button = this->list_of_keys_midi.value(midicode, nullptr);
    if (button)
        button->release();
}

void KeyboardPiano::allKeysUp()
{
    for (ButtonPiano *button : this->list_of_keys_midi)
        button->release();
}
