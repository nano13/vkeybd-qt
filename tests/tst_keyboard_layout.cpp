#include <QSignalSpy>
#include <QtTest>
#include <QFontMetrics>

#include "button_piano.h"
#include "keyboard_pc.h"
#include "keyboard_piano.h"
#include "keymap.h"
#include "keyboard_layout.h"

#include <QTemporaryDir>

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

    void midiInputHighlightsPcKeysWithoutSendingNotes()
    {
        KeyboardPC pc;
        const QList<QPushButton*> buttons = pc.findChildren<QPushButton*>();
        QSignalSpy pressed(&pc, &KeyboardPC::MIDIPress);
        QSignalSpy released(&pc, &KeyboardPC::MIDIRelease);

        // Incoming MIDI 60 maps to PC note 12, which is on row 2, column 2.
        QPushButton *middle_c = buttons.at(14 + 1);
        QVERIFY(!middle_c->isDown());
        pc.setMidiNoteDown(12, true);
        QVERIFY(middle_c->isDown());
        QCOMPARE(pressed.size(), 0);
        QCOMPARE(released.size(), 0);

        // Pressing and releasing the physical key must not clear the MIDI hold.
        pc.keyDownRaw(16);
        QCOMPARE(pressed.size(), 1);
        pc.keyUpRaw(16);
        QCOMPARE(released.size(), 1);
        QVERIFY(middle_c->isDown());

        pc.setMidiNoteDown(12, false);
        QVERIFY(!middle_c->isDown());
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 1);

        // The reverse order is also safe: MIDI release cannot cancel a PC hold.
        pc.keyDownRaw(16);
        pc.setMidiNoteDown(12, true);
        pc.setMidiNoteDown(12, false);
        QVERIFY(middle_c->isDown());
        pc.keyUpRaw(16);
        QVERIFY(!middle_c->isDown());
        QCOMPARE(pressed.size(), 2);
        QCOMPARE(released.size(), 2);
    }

    void midiInputHighlightsAllMappedKeysAndClearsOnRemap()
    {
        KeyboardPC pc;
        const QList<QPushButton*> buttons = pc.findChildren<QPushButton*>();
        QSignalSpy pressed(&pc, &KeyboardPC::MIDIPress);
        QSignalSpy released(&pc, &KeyboardPC::MIDIRelease);

        // Two PC buttons share MIDI note 8.
        QPushButton *top = buttons.at(0);
        QPushButton *bottom = buttons.at(14 + 14 + 13);
        pc.setMidiNoteDown(8, true);
        QVERIFY(top->isDown());
        QVERIFY(bottom->isDown());

        QList<QList<int>> mapping = pc.midiMapping();
        mapping[0][0] = 60;
        mapping[3][0] = 60;
        QVERIFY(pc.setMidiMapping(mapping));
        QVERIFY(!top->isDown());
        QVERIFY(!bottom->isDown());

        pc.setMidiNoteDown(60, true);
        QVERIFY(top->isDown());
        QVERIFY(bottom->isDown());
        pc.clearMidiNoteHighlights();
        QVERIFY(!top->isDown());
        QVERIFY(!bottom->isDown());
        QCOMPARE(pressed.size(), 0);
        QCOMPARE(released.size(), 0);
    }

    void labelsAndColorsFollowTheMapping()
    {
        KeyboardPC pc;
        const QList<PcKeyButton*> keys = pc.findChildren<PcKeyButton*>();
        // both spellings on the black keys, also on the "=" key (MIDI 44)
        QCOMPARE(keys.at(0)->text(), QString("Ab\nG#"));
        QCOMPARE(keys.at(12)->text(), QString("Ab\nG#"));
        QCOMPARE(keys.at(14 + 1)->text(), QString("C"));
        QCOMPARE(keys.at(0)->objectName(), QString("pc_black"));
        QCOMPARE(keys.at(14 + 1)->objectName(), QString("pc_white"));

        QList<QList<int>> mapping = pc.midiMapping();
        mapping[0][0] = 12;
        QVERIFY(pc.setMidiMapping(mapping));
        QCOMPARE(keys.at(0)->text(), QString("C"));
        QCOMPARE(keys.at(0)->objectName(), QString("pc_white"));

        // function keys have no note
        mapping[4][3] = 60;
        QVERIFY(!pc.setMidiMapping(mapping));
    }

    void soundingLineShowsOctavesOrNames()
    {
        PcKeyButton key(nullptr);
        key.setKeyPitchClass(8); // G#
        QCOMPARE(key.soundingText(1000), QString());

        key.setSoundingNotes({56});
        QCOMPARE(key.soundingText(1000), QString("3"));
        key.setSoundingNotes({44, 56});
        QCOMPARE(key.soundingText(1000), QString("2 3"));
        QVERIFY(key.toolTip().contains("G#2, G#3"));

        // transposed by a non-octave amount: the sounding name, then the octaves
        key.setSoundingNotes({49, 61});
        QCOMPARE(key.soundingText(1000), QString("C# 3 4"));
        // different names (e.g. one channel a fifth up): full names
        key.setSoundingNotes({44, 51});
        QCOMPARE(key.soundingText(1000), QString("G#2 D#3"));

        // too long for the key: lowest ... highest
        key.setSoundingNotes({44, 56, 68, 80});
        QCOMPARE(key.soundingText(1000), QString("2 3 4 5"));
        const int narrow = QFontMetrics(key.font()).horizontalAdvance(QString("2") + QChar(0x2026) + "5");
        QCOMPARE(key.soundingText(narrow), QString("2") + QChar(0x2026) + "5");
    }

    void soundingNotesReachTheKeys()
    {
        KeyboardPC pc;
        const QList<PcKeyButton*> keys = pc.findChildren<PcKeyButton*>();
        QHash<int, QList<int>> sounding;
        sounding.insert(8, {44, 56});
        pc.setSoundingNotes(sounding);
        // both keys of note 8 (first and fourth row)
        QCOMPARE(keys.at(0)->soundingNotes(), (QList<int>{44, 56}));
        QCOMPARE(keys.at(14 + 14 + 13)->soundingNotes(), (QList<int>{44, 56}));
        QVERIFY(keys.at(1)->soundingNotes().isEmpty());
    }

    void soundingLineCanBeSwitchedOff()
    {
        KeyboardPC pc;
        const QList<PcKeyButton*> keys = pc.findChildren<PcKeyButton*>();
        QVERIFY(pc.showSoundingNotes());
        const int tall = pc.height();
        pc.setShowSoundingNotes(false);
        QVERIFY(pc.height() < tall);
        // the notes are still known (tooltip), only the line is not drawn
        QHash<int, QList<int>> sounding;
        sounding.insert(8, {44, 56});
        pc.setSoundingNotes(sounding);
        QCOMPARE(keys.at(0)->soundingNotes(), (QList<int>{44, 56}));
        QVERIFY(keys.at(0)->toolTip().contains("G#2"));
        pc.setShowSoundingNotes(true);
        QCOMPARE(pc.height(), tall);
    }

    void pedalKeysWorkWithTheMouse()
    {
        KeyboardPC pc;
        const QList<PcKeyButton*> keys = pc.findChildren<PcKeyButton*>();
        const int bottom_row = 14 + 14 + 13 + 13;
        QSignalSpy sustain(&pc, &KeyboardPC::sustain);
        QSignalSpy pressed(&pc, &KeyboardPC::MIDIPress);
        keys.at(bottom_row + 3)->click();
        QCOMPARE(sustain.size(), 2);
        QCOMPARE(sustain.at(0).at(0).toBool(), true);
        QCOMPARE(sustain.at(1).at(0).toBool(), false);
        QCOMPARE(pressed.size(), 0);

        // the physical key only lights up the display, the pedal is MainTabs' business
        pc.keyDownRaw(KeysRaw::Space);
        QVERIFY(keys.at(bottom_row + 3)->isDown());
        QCOMPARE(sustain.size(), 2);
        pc.keyUpRaw(KeysRaw::Space);
        QVERIFY(!keys.at(bottom_row + 3)->isDown());
    }

    void noteNamesRoundTrip()
    {
        QCOMPARE(Keymap::noteName(8), QString("G#2"));
        QCOMPARE(Keymap::noteName(24), QString("C4"));     // middle C
        QCOMPARE(Keymap::noteName(-1), QString());
        int note = -5;
        QVERIFY(Keymap::noteFromName("C4", &note));
        QCOMPARE(note, 24);
        QVERIFY(Keymap::noteFromName("eb3", &note));
        QCOMPARE(note, 15);
        QVERIFY(!Keymap::noteFromName("H3", &note));
        QVERIFY(!Keymap::noteFromName("C1", &note));       // below the range
        for (int n = Keymap::NOTE_MIN; n <= Keymap::NOTE_MAX; ++n)
        {
            int back = -1;
            QVERIFY(Keymap::noteFromName(Keymap::noteName(n), &back));
            QCOMPARE(back, n);
        }
    }

    void defaultKeymapIsTheAccordionLayout()
    {
        KeyboardPC pc;
        const Keymap keymap;
        QCOMPARE(keymap.note(41, KeyboardForm::ISO), 8);    // left of 1: G#
        QCOMPARE(keymap.note(28, KeyboardForm::ISO), 48);   // ISO Enter
        QCOMPARE(keymap.note(42, KeyboardForm::ISO), 8);    // row 4 = row 1
        QCOMPARE(pc.midiNotes().first(), 8);
        QCOMPARE(pc.midiNotes().last(), 48);
    }

    void ansiKeepsTheKeyPositions()
    {
        Keymap keymap;
        // the keys sit where they are: Z keeps its note, Enter and backslash move
        QCOMPARE(keymap.note(44, KeyboardForm::ANSI), keymap.note(44, KeyboardForm::ISO));
        QCOMPARE(keymap.note(86, KeyboardForm::ANSI), -1);
        QCOMPARE(keymap.note(43, KeyboardForm::ANSI), 48);  // end of the Q row
        QCOMPARE(keymap.note(28, KeyboardForm::ANSI), 46);  // end of the A row

        KeyboardPC pc;
        pc.setKeyboardForm(KeyboardForm::ANSI);
        QVERIFY(!pc.keycodes().contains(86));
        QCOMPARE(pc.findChildren<PcKeyButton*>().size(), pc.keycodes().size());
        QSignalSpy pressed(&pc, &KeyboardPC::MIDIPress);
        pc.keyDownRaw(28);
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(pressed.at(0).at(0).toInt(), 46);
    }

    void jisHasItsExtraKeys()
    {
        KeyboardPC pc;
        pc.setKeyboardForm(KeyboardForm::JIS);
        QVERIFY(pc.keycodes().contains(124));
        QVERIFY(pc.keycodes().contains(89));
        QVERIFY(pc.isNoteKey(94));      // Muhenkan: can get a note by hand
        QVERIFY(!pc.isNoteKey(KeysRaw::Space));
        Keymap keymap;
        QVERIFY(keymap.note(124, KeyboardForm::JIS) >= 0);
        QCOMPARE(keymap.note(94, KeyboardForm::JIS), -1);
    }

    void automaticFormFollowsProvingKeys()
    {
        KeyboardPC pc;
        pc.setKeyboardForm(KeyboardForm::ANSI);
        pc.setAutoForm(true);
        QSignalSpy detected(&pc, &KeyboardPC::keyboardFormDetected);
        QSignalSpy pressed(&pc, &KeyboardPC::MIDIPress);
        pc.keyDownRaw(86);      // only ISO has it
        QCOMPARE(pc.keyboardForm(), KeyboardForm::ISO);
        QCOMPARE(detected.size(), 1);
        QCOMPARE(pressed.size(), 1);    // and the key plays right away
        pc.keyUpRaw(86);

        // without automatic form nothing changes
        pc.setAutoForm(false);
        pc.keyDownRaw(124);
        QCOMPARE(pc.keyboardForm(), KeyboardForm::ISO);
    }

    void handSetKeysFollowTheKeyOnEveryForm()
    {
        Keymap keymap;
        keymap.overrides.insert(86, 30);    // ISO only
        keymap.overrides.insert(30, -1);    // A: silent
        KeyboardPC pc;
        pc.setKeymap(keymap);
        QCOMPARE(pc.currentKeymap().note(86, KeyboardForm::ISO), 30);
        pc.setKeyboardForm(KeyboardForm::ANSI);
        QVERIFY(!pc.midiNotes().contains(-1));
        pc.setKeyboardForm(KeyboardForm::ISO);
        QSignalSpy pressed(&pc, &KeyboardPC::MIDIPress);
        pc.keyDownRaw(86);
        pc.keyDownRaw(30);
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(pressed.at(0).at(0).toInt(), 30);
    }

    void keymapJsonRoundTrip()
    {
        Keymap keymap;
        keymap.name = "test";
        keymap.pattern = KeymapPattern::Piano;
        keymap.start = 12;
        keymap.overrides.insert(86, 30);
        keymap.overrides.insert(30, -1);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("test.json");
        QString error;
        QVERIFY2(keymap.saveFile(path, &error), qPrintable(error));
        Keymap loaded;
        QVERIFY2(Keymap::loadFile(path, &loaded, &error), qPrintable(error));
        QVERIFY(loaded == keymap);

        QJsonObject broken = keymap.toJson();
        broken.insert("pattern", "nonsense");
        QVERIFY(!Keymap::fromJson(broken, &loaded, &error));
    }

    void patternsFillTheKeys()
    {
        Keymap piano;
        piano.pattern = KeymapPattern::Piano;
        piano.start = 12;
        QCOMPARE(piano.note(44, KeyboardForm::ISO), 12);    // Z: C
        QCOMPARE(piano.note(31, KeyboardForm::ISO), 13);    // S: C#
        QCOMPARE(piano.note(16, KeyboardForm::ISO), 24);    // Q: C one octave up

        Keymap none;
        none.pattern = KeymapPattern::None;
        QVERIFY(none.patternNotes(KeyboardForm::ISO).isEmpty());

        const QList<KeymapPattern> patterns = Keymap::allPatterns();
        for (const KeymapPattern pattern : patterns)
        {
            Keymap keymap;
            keymap.pattern = pattern;
            keymap.start = Keymap::defaultStart(pattern);
            const QMap<int, int> notes = keymap.patternNotes(KeyboardForm::ISO);
            for (const int note : notes)
                QVERIFY(note >= Keymap::NOTE_MIN && note <= Keymap::NOTE_MAX);
            if (pattern != KeymapPattern::None)
                QVERIFY(notes.size() > 20);
        }
    }

    void editModeSelectsInsteadOfPlaying()
    {
        KeyboardPC pc;
        pc.setEditMode(true);
        const QList<PcKeyButton*> keys = pc.findChildren<PcKeyButton*>();
        QSignalSpy selected(&pc, &KeyboardPC::keySelected);
        QSignalSpy pressed(&pc, &KeyboardPC::MIDIPress);
        QSignalSpy sustain(&pc, &KeyboardPC::sustain);
        keys.at(0)->click();
        keys.at(14 + 14 + 13 + 13 + 3)->click();    // space bar
        QCOMPARE(selected.size(), 2);
        QCOMPARE(selected.at(0).at(0).toInt(), 41);
        QCOMPARE(selected.at(1).at(0).toInt(), int(KeysRaw::Space));
        QCOMPARE(pressed.size(), 0);
        QCOMPARE(sustain.size(), 0);
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
