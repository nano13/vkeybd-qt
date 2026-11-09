#include <QSignalSpy>
#include <QtTest>

#include "button_piano.h"
#include "keyboard_pc.h"
#include "keyboard_piano.h"

class TestKeyboardLayout : public QObject
{
    Q_OBJECT
private slots:
    void pianoUsesPcMapping()
    {
        KeyboardPC pc;
        KeyboardPiano piano;
        piano.setMidiNotes(pc.midiNotes());

        QList<int> expected;
        for (int note = 8; note <= 48; ++note)
            expected.append(note);

        QCOMPARE(pc.midiNotes(), expected);
        QCOMPARE(piano.midiNotes(), expected);
        QCOMPARE(piano.findChildren<ButtonPiano*>().size(), expected.size());
        for (int note : expected)
        {
            ButtonPiano *button = piano.findChild<ButtonPiano*>(QStringLiteral("piano_note_%1").arg(note));
            QVERIFY2(button != nullptr, qPrintable(QString::number(note)));
        }

        ButtonPiano *lowest = piano.findChild<ButtonPiano*>("piano_note_8");
        QVERIFY(lowest != nullptr);
        piano.keyPressed(8);
        QVERIFY(lowest->isDown());
        piano.keyReleased(8);
        QVERIFY(!lowest->isDown());
    }

    void pianoFillsRowAtDifferentWidths()
    {
        KeyboardPC pc;
        KeyboardPiano piano;
        piano.setMidiNotes(pc.midiNotes());
        piano.show(); // Qt delivers resize events immediately to visible widgets.

        for (int target_width : {360, 440, 600})
        {
            piano.resize(target_width, piano.height());
            QCOMPARE(piano.width(), target_width);
            QCOMPARE(piano.sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);

            const QList<ButtonPiano*> keys = piano.findChildren<ButtonPiano*>();
            int min_white_x = target_width;
            int max_white_right = 0;
            for (ButtonPiano *button : keys)
            {
                const QRect rect = button->geometry();
                QVERIFY(rect.left() >= 0);
                QVERIFY(rect.right() < target_width);
                QVERIFY(rect.top() >= 0);
                QVERIFY(rect.bottom() < piano.height());
                QVERIFY(rect.width() > 0);
                QVERIFY(rect.height() > 0);

                const int midi_note = button->objectName().mid(11).toInt();
                const int pitch = midi_note % 12;
                const bool black = pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10;
                if (!black)
                {
                    min_white_x = qMin(min_white_x, rect.left());
                    max_white_right = qMax(max_white_right, rect.right());
                }
            }
            QCOMPARE(min_white_x, 0);
            QCOMPARE(max_white_right, target_width - 1);
        }
    }

    void mappingChangeUpdatesPianoAndReleasesOldNote()
    {
        KeyboardPC pc;
        KeyboardPiano piano;
        piano.setMidiNotes(pc.midiNotes());
        connect(&pc, &KeyboardPC::midiMappingChanged, &piano, [&pc, &piano]{
            piano.setMidiNotes(pc.midiNotes());
        });

        QSignalSpy pressed(&pc, &KeyboardPC::MIDIPress);
        QSignalSpy released(&pc, &KeyboardPC::MIDIRelease);
        QSignalSpy changed(&pc, &KeyboardPC::midiMappingChanged);
        pc.keyDownRaw(41); // first PC key sends MIDI 8
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(pressed.at(0).at(0).toInt(), 8);

        QList<QList<int>> mapping = pc.midiMapping();
        mapping[0][0] = 60;
        mapping[3][0] = 60;
        QVERIFY(pc.setMidiMapping(mapping));
        QCOMPARE(changed.size(), 1);
        QCOMPARE(released.size(), 1);
        QCOMPARE(released.at(0).at(0).toInt(), 8);
        QVERIFY(!pc.midiNotes().contains(8));
        QVERIFY(pc.midiNotes().contains(60));
        QCOMPARE(piano.midiNotes(), pc.midiNotes());
        QVERIFY(piano.findChild<ButtonPiano*>("piano_note_60") != nullptr);
        QVERIFY(piano.findChild<ButtonPiano*>("piano_note_8") == nullptr);

        // Releasing the physical key after a remap must not release MIDI 60.
        pc.keyUpRaw(41);
        QCOMPARE(released.size(), 1);
        pc.keyDownRaw(41);
        QCOMPARE(pressed.size(), 2);
        QCOMPARE(pressed.at(1).at(0).toInt(), 60);
        pc.keyUpRaw(41);
        QCOMPARE(released.size(), 2);
        QCOMPARE(released.at(1).at(0).toInt(), 60);

        mapping[0].append(61);
        QVERIFY(!pc.setMidiMapping(mapping));
        QCOMPARE(changed.size(), 1);
    }

    void sparseMappingKeepsOnlyAssignedKeys()
    {
        KeyboardPiano piano;
        piano.setMidiNotes({60, 61, 61, 64, -1, 128});
        QCOMPARE(piano.midiNotes(), (QList<int>{60, 61, 64}));
        QCOMPARE(piano.findChildren<ButtonPiano*>().size(), 3);

        piano.setMidiNotes({1, 3, 6});
        QCOMPARE(piano.midiNotes(), (QList<int>{1, 3, 6}));
        QCOMPARE(piano.findChildren<ButtonPiano*>().size(), 3);
    }
};

QTEST_MAIN(TestKeyboardLayout)
#include "tst_keyboard_layout.moc"
