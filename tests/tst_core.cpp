#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QSettings>
#include <QFile>

#include "midi_engine.h"
#include "preset_serializer.h"
#include "midi_sounds_list.h"

struct RecordedEvent
{
    QString type;
    int port;
    int channel;
    int value;
    int value2 = 0;
};

class RecordingInterface : public InterfaceAudio
{
public:
    QList<RecordedEvent> events;

    void keyPressEvent(int port, int channel, uint8_t midicode, uint8_t velocity) override
    {
        Q_UNUSED(velocity);
        this->events.append(RecordedEvent{"on", port, channel, midicode});
    }
    void keyReleaseEvent(int port, int channel, uint8_t midicode, uint8_t velocity) override
    {
        Q_UNUSED(velocity);
        this->events.append(RecordedEvent{"off", port, channel, midicode});
    }
    void keyPitchbendEvent(int port, int channel, int pitch) override
    {
        this->events.append(RecordedEvent{"pitchbend", port, channel, pitch});
    }
    void setProgramChangeEvent(int port, int channel, uint8_t program, int bank) override
    {
        this->events.append(RecordedEvent{"program", port, channel, program, bank});
    }
    void setVolumeChangeEvent(int port, int channel, uint8_t volume) override
    {
        this->events.append(RecordedEvent{"volume", port, channel, volume});
    }
    void setPanChangeEvent(int port, int channel, uint8_t value) override
    {
        this->events.append(RecordedEvent{"pan", port, channel, value});
    }
    void setControlChangeEvent(int port, int channel, uint8_t cc, uint8_t value) override
    {
        this->events.append(RecordedEvent{"cc", port, channel, cc, value});
    }
    void keyPanicEvent(int port, int channel) override
    {
        this->events.append(RecordedEvent{"panic", port, channel, 0});
    }
    void keyStopAllEvent(int port, int channel) override
    {
        this->events.append(RecordedEvent{"stop", port, channel, 0});
    }
    void keySustainEvent(int port, int channel, bool pressed) override
    {
        this->events.append(RecordedEvent{"sustain", port, channel, pressed ? 1 : 0});
    }
    
    int count(const QString &type) const
    {
        int n = 0;
        for (const RecordedEvent &event : this->events)
        {
            if (event.type == type)
                n++;
        }
        return n;
    }
};

class TestCore : public QObject
{
    Q_OBJECT

private:
    // source key 20 -> keycode 8, with the default octave (offset 48) -> note 56
    static constexpr int KEY = 20;
    static constexpr int NOTE = 56;

    static TabPreset examplePreset()
    {
        TabPreset preset;
        preset.master_key_shift = -12;
        preset.master_volume = 90;
        preset.octave_mask = "01000100";
        preset.pitchwheel_tether = 30;
        preset.pitchwheel_pitch = 9000;
        preset.resend_midi_auto = true;

        preset.channels[0].activated = false;
        preset.channels[5].activated = true;
        preset.channels[5].volume = 99;
        preset.channels[5].key_shift = -5;
        preset.channels[5].instrument_msb = -1;
        preset.channels[5].instrument_lsb = -1;
        preset.channels[5].instrument_bank = "[disabled]";
        preset.channels[5].instrument_name = "Wide Harpsichord";
        preset.channels[5].velocity = 112;
        preset.channels[5].pitch = -100;

        CCSetting cc_a;
        cc_a.channel = 2;
        cc_a.label = "modulation";
        cc_a.cc = 1;
        cc_a.value = 10;

        CCSetting cc_b;
        cc_b.channel = 5;
        cc_b.active = false;
        cc_b.label = "reverb, send";
        cc_b.cc = 91;
        cc_b.value = 40;
        cc_b.delay_ms = 20;

        CCSetting cc_c;
        cc_c.channel = 5;
        cc_c.cc = 93;
        cc_c.value = 127;
        cc_c.delay_ms = 5000;

        // channel order, as the serializer writes them
        preset.cc_entries = {cc_a, cc_b, cc_c};

        return preset;
    }

private slots:
    // ---------------- engine ----------------

    void releaseUsesSentNoteAfterShiftChange()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);

        TabPreset preset;
        engine.setPreset(0, preset);
        engine.noteOn(0, KEY);

        preset.master_key_shift = 1;            // e.g. [Pause] while the key is held
        engine.setPreset(0, preset);
        engine.noteOff(KEY);

        QCOMPARE(audio.events.length(), 2);
        QCOMPARE(audio.events.at(1).type, QString("off"));
        QCOMPARE(audio.events.at(1).value, NOTE);
        QCOMPARE(engine.heldNoteCount(), 0);
    }

    void releaseAfterTabSwitch()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);

        TabPreset tab_0;
        TabPreset tab_1;
        tab_1.channels[0].activated = false;
        tab_1.channels[3].activated = true;
        engine.setPreset(0, tab_0);
        engine.setPreset(1, tab_1);

        engine.noteOn(0, KEY);
        engine.noteOff(KEY);                    // the release arrives while tab 1 is active

        QCOMPARE(audio.events.length(), 2);
        QCOMPARE(audio.events.at(1).port, 0);
        QCOMPARE(audio.events.at(1).channel, 0);
    }

    void multiplexedKeyReleasedOnce()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());
        engine.setPreset(1, TabPreset());

        engine.noteOn(0, KEY);
        engine.noteOn(1, KEY);
        QCOMPARE(engine.heldNoteCount(), 2);

        engine.noteOff(KEY);
        engine.noteOff(KEY);                    // the injected release of the second tab
        QCOMPARE(engine.heldNoteCount(), 0);
        QCOMPARE(audio.events.length(), 4);
    }

    void outOfRangeNotesSkipped()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);

        TabPreset preset;
        preset.octave_mask = "10000000";
        preset.channels[0].key_shift = -64;
        engine.setPreset(0, preset);

        engine.noteOn(0, KEY);
        QVERIFY(audio.events.isEmpty());
    }

    void keyRangeFilter()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);

        TabPreset preset;
        preset.channels[0].key_min = 60;
        engine.setPreset(0, preset);

        engine.noteOn(0, KEY);
        QVERIFY(audio.events.isEmpty());
    }

    void octavesTimesChannels()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);

        TabPreset preset;
        preset.octave_mask = "00011000";
        preset.channels[1].activated = true;
        preset.channels[1].key_shift = 7;
        engine.setPreset(0, preset);

        engine.noteOn(0, KEY);
        QCOMPARE(audio.events.length(), 4);
        engine.noteOff(KEY);
        QCOMPARE(audio.events.length(), 8);
    }

    void retriggerReleasesFirst()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());

        engine.noteOn(0, KEY);
        engine.noteOn(0, KEY);                  // lost key-up
        QCOMPARE(audio.events.length(), 3);
        QCOMPARE(audio.events.at(1).type, QString("off"));
        QCOMPARE(engine.heldNoteCount(), 1);
    }

    void forgetTabDropsWithoutSending()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());
        engine.setPreset(1, TabPreset());

        engine.noteOn(0, KEY);
        engine.noteOn(1, KEY);
        engine.forgetTab(0);
        audio.events.clear();

        engine.noteOff(KEY);
        QCOMPARE(audio.events.length(), 1);
        QCOMPARE(audio.events.at(0).port, 1);
    }

    void pitchBendOncePerChannel()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);

        TabPreset preset;
        preset.octave_mask = "11111111";
        preset.channels[4].activated = true;
        engine.setPreset(0, preset);

        engine.pitchBend(0, 9000);
        QCOMPARE(audio.events.length(), 2);
    }

    void displaySignals()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());

        QSignalSpy spy_on(&engine, &MidiEngine::signalNoteOn);
        QSignalSpy spy_off(&engine, &MidiEngine::signalNoteOff);
        QSignalSpy spy_source(&engine, &MidiEngine::signalSourceKey);

        engine.noteOn(0, KEY);
        engine.noteOff(KEY);

        QCOMPARE(spy_on.count(), 1);
        QCOMPARE(spy_off.count(), 1);
        QCOMPARE(spy_source.count(), 2);
        QCOMPARE(spy_on.at(0).at(1).toInt(), NOTE);
    }

    // ---------------- engine: controls ----------------
    
    void setChannelSendsOnlyChanges()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(3, TabPreset());
        
        ChannelSettings settings = engine.preset(3).channels.at(0);
        settings.volume = 100;
        engine.setChannel(3, 0, settings);
        QCOMPARE(audio.events.length(), 1);
        QCOMPARE(audio.events.at(0).type, QString("volume"));
        QCOMPARE(audio.events.at(0).port, 3);
        QCOMPARE(audio.events.at(0).value, 100);
        
        // routing-only changes send nothing, but take effect immediately
        audio.events.clear();
        settings.velocity = 90;
        engine.setChannel(3, 0, settings);
        QVERIFY(audio.events.isEmpty());
        engine.noteOn(3, KEY);
        QCOMPARE(audio.events.length(), 1);
    }
    
    void disabledInstrumentSendsNoProgram()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());
        
        ChannelSettings settings = engine.preset(0).channels.at(0);
        settings.instrument_msb = -1;
        settings.instrument_lsb = -1;
        engine.setChannel(0, 0, settings);
        QCOMPARE(audio.count("program"), 0);
        
        settings.instrument_msb = 40;
        settings.instrument_lsb = 2;
        engine.setChannel(0, 0, settings);
        QCOMPARE(audio.count("program"), 1);
        QCOMPARE(audio.events.last().value, 40);
        QCOMPARE(audio.events.last().value2, 2);
    }
    
    void masterVolumeScalesAllChannels()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());
        
        engine.setMasterVolume(0, 50);
        QCOMPARE(audio.count("volume"), 16);
        QCOMPARE(audio.events.at(0).value, 63);     // 127 * 50 %
        
        audio.events.clear();
        engine.setMasterVolume(0, 50);              // unchanged: nothing
        QVERIFY(audio.events.isEmpty());
    }
    
    void resendActivatedChannelsProgramFirst()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        
        TabPreset preset;
        preset.channels[4].activated = true;
        CCSetting cc;
        cc.channel = 4;
        cc.cc = 91;
        cc.value = 30;
        preset.cc_entries = {cc};
        engine.setPreset(0, preset);
        
        engine.resendControls(0);
        QCOMPARE(audio.events.at(0).type, QString("program"));
        QCOMPARE(audio.count("program"), 2);
        QCOMPARE(audio.count("volume"), 2);
        QCOMPARE(audio.count("cc"), 1);             // delay 0: sent immediately
    }
    
    void resendDelayedCC()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        
        TabPreset preset;
        CCSetting cc;
        cc.cc = 93;
        cc.value = 31;
        cc.delay_ms = 20;
        preset.cc_entries = {cc};
        engine.setPreset(0, preset);
        
        engine.resendControls(0);
        QCOMPARE(audio.count("cc"), 0);
        QTRY_COMPARE(audio.count("cc"), 1);
    }
    
    void panicStopSustainOnActivatedChannels()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        TabPreset preset;
        preset.channels[2].activated = true;
        engine.setPreset(0, preset);
        
        engine.noteOn(0, KEY);
        engine.panic(0);
        QCOMPARE(audio.count("panic"), 2);
        QCOMPARE(engine.heldNoteCount(), 0);
        
        engine.stopAll(0);
        QCOMPARE(audio.count("stop"), 2);
        
        engine.sustain(0, true);
        QCOMPARE(audio.count("sustain"), 2);
    }
    
    void setPresetMainKeepsChannels()
    {
        MidiEngine engine(nullptr);
        TabPreset preset = examplePreset();
        engine.setPreset(0, preset);
        
        TabPreset main;
        main.master_key_shift = 12;
        main.channels[5].activated = false;
        engine.setPresetMain(0, main);
        
        QVERIFY(engine.preset(0).channels.at(5).activated);
        QCOMPARE(engine.preset(0).master_key_shift, 12);
        QCOMPARE(engine.preset(0).cc_entries.length(), 3);
    }
    
    void globalKeyShift()
    {
        MidiEngine engine(nullptr);
        engine.setPreset(0, TabPreset());
        TabPreset preset;
        preset.master_key_shift = 7;            // loaded: also the reset value
        engine.setPreset(1, preset);
        engine.setMasterKeyShift(0, 12);        // set by the user: also the reset value
        
        engine.globalKeyShift(2, true);
        QCOMPARE(engine.preset(0).master_key_shift, 14);
        QCOMPARE(engine.preset(1).master_key_shift, 9);
        
        engine.globalKeyShift(-5, false);
        QCOMPARE(engine.preset(0).master_key_shift, -5);
        QCOMPARE(engine.preset(1).master_key_shift, -5);
        
        engine.globalKeyShift(0, true);         // reset
        QCOMPARE(engine.preset(0).master_key_shift, 12);
        QCOMPARE(engine.preset(1).master_key_shift, 7);
        
        engine.globalKeyShift(100, false);
        QCOMPARE(engine.preset(0).master_key_shift, MidiEngine::KEY_SHIFT_LIMIT);
    }
    
    void globalKeyShiftMovesNotes()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());
        
        engine.globalKeyShift(1, true);
        engine.noteOn(0, KEY);
        QCOMPARE(audio.events.at(0).value, NOTE + 1);
    }
    
    // ---------------- MIDI input ----------------
    
    void midiInputPlaysAsPlayed()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());
        
        engine.midiNoteOn(0, 60, 100);
        QCOMPARE(audio.events.length(), 1);
        QCOMPARE(audio.events.at(0).value, 60);
        
        engine.midiNoteOff(60);
        QCOMPARE(audio.events.last().type, QString("off"));
        QCOMPARE(engine.heldNoteCount(), 0);
    }
    
    void midiInputSourceKeysMapToPcDisplayNotes()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());

        QSignalSpy sources(&engine, &MidiEngine::signalSourceKey);
        engine.midiNoteOn(0, 60, 100);
        QCOMPARE(sources.size(), 1);
        QCOMPARE(sources.at(0).at(0).toInt(), 0);
        const int held_source = sources.at(0).at(1).toInt();
        QCOMPARE(held_source, MidiEngine::MIDI_INPUT_HELD_KEY_BASE + 60);
        QCOMPARE(MidiEngine::displayNoteForSourceKey(held_source), 12);
        QVERIFY(sources.at(0).at(2).toBool());

        engine.midiNoteOff(60);
        QCOMPARE(sources.size(), 2);
        QCOMPARE(sources.at(1).at(1).toInt(), held_source);
        QVERIFY(!sources.at(1).at(2).toBool());
        QCOMPARE(MidiEngine::displayNoteForSourceKey(12), 12);
        QCOMPARE(engine.heldNoteCount(), 0);
    }

    void midiInputAndPCKeysAreIndependent()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        engine.setPreset(0, TabPreset());
        
        engine.midiNoteOn(0, 60, 100);
        engine.noteOn(0, KEY);
        engine.noteOff(KEY);                    // releases only the PC note
        QCOMPARE(engine.heldNoteCount(), 1);
        QCOMPARE(audio.events.last().value, NOTE);
        
        engine.midiNoteOff(60);
        QCOMPARE(engine.heldNoteCount(), 0);
    }
    
    void midiInputVelocityScaledByChannel()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        TabPreset preset;
        preset.channels[0].velocity = 96;       // ff: 1.5 x
        engine.setPreset(0, preset);
        
        engine.midiNoteOn(0, 60, 40);
        QCOMPARE(audio.events.last().value2, 60);
        engine.midiNoteOff(60);
        
        engine.midiNoteOn(0, 60, 100);
        QCOMPARE(audio.events.last().value2, 127);
    }
    
    void midiInputUsesOctavesAndShifts()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        TabPreset preset;
        preset.octave_mask = "00011000";        // one octave lower in addition
        preset.master_key_shift = 2;
        engine.setPreset(0, preset);
        
        engine.midiNoteOn(0, 60, 64);
        QCOMPARE(audio.events.length(), 2);
        QCOMPARE(audio.events.at(0).value, 50);
        QCOMPARE(audio.events.at(1).value, 62);
    }
    
    void midiInputControlChangeForwarded()
    {
        RecordingInterface audio;
        MidiEngine engine(&audio);
        TabPreset preset;
        preset.channels[3].activated = true;
        engine.setPreset(0, preset);
        
        engine.forwardControlChange(0, 1, 77);
        QCOMPARE(audio.count("cc"), 2);
        QCOMPARE(audio.events.at(0).value, 1);
        QCOMPARE(audio.events.at(0).value2, 77);
    }
    
    void midiMessageDecoding()
    {
        InterfaceAudio audio;
        QSignalSpy spy_on(&audio, &InterfaceAudio::signalMidiNoteOn);
        QSignalSpy spy_off(&audio, &InterfaceAudio::signalMidiNoteOff);
        QSignalSpy spy_cc(&audio, &InterfaceAudio::signalMidiControlChange);
        QSignalSpy spy_bend(&audio, &InterfaceAudio::signalMidiPitchBend);
        
        audio.dispatchMidiMessage(0x93, 60, 100);   // any channel
        audio.dispatchMidiMessage(0x90, 60, 0);     // note on with velocity 0
        audio.dispatchMidiMessage(0x81, 61, 10);
        audio.dispatchMidiMessage(0xB0, 64, 127);
        audio.dispatchMidiMessage(0xE0, 0, 64);
        audio.dispatchMidiMessage(0xC0, 5, 0);      // program change: ignored
        
        QCOMPARE(spy_on.count(), 1);
        QCOMPARE(spy_on.at(0).at(0).toInt(), 60);
        QCOMPARE(spy_on.at(0).at(1).toInt(), 100);
        QCOMPARE(spy_off.count(), 2);
        QCOMPARE(spy_cc.count(), 1);
        QCOMPARE(spy_bend.count(), 1);
        QCOMPARE(spy_bend.at(0).at(0).toInt(), 8192);
    }
    
    // ---------------- instruments ----------------
    
    void everyInstrumentRoundTrips()
    {
        MIDISoundsList sounds;
        int checked = 0;
        
        QList<QString> groups = sounds.getInstrumentGroups();
        for (const QString &group : groups)
        {
            QList<QString> names = sounds.getInstrumentsForGroupMIDIv2(group);
            for (const QString &name : names)
            {
                QMap<QString,int> codes = sounds.getMIDICodesForInstrument(group, name);
                QVERIFY2(codes.contains("msb"), qPrintable(group + " / " + name));
                
                QMap<QString,QString> back = sounds.getInstrumentForMIDICodes(codes["msb"] - 1, codes["lsb"]);
                QCOMPARE(back["group"], group);
                QCOMPARE(back["instrument"], name);
                checked++;
            }
        }
        QVERIFY(checked > 200);
    }
    
    // ---------------- serializer ----------------

    void iniRoundTrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.filePath("roundtrip.ini");

        TabPreset preset = examplePreset();
        {
            QSettings settings(path, QSettings::IniFormat);
            PresetSerializer::save(&settings, 0, "F01-1", preset);
            settings.sync();
        }

        QSettings settings(path, QSettings::IniFormat);
        TabPreset loaded = PresetSerializer::load(&settings, 0, "F01-1");
        QVERIFY(loaded == preset);

        // INI layout of the save files (same as written by the former Config code)
        QCOMPARE(settings.value("0/F01-1/005/volume").toInt(), 99);
        QCOMPARE(settings.value("0/F01-1/005/cc_001_active").toString(), QString("false"));
        QCOMPARE(settings.value("0/F01-1/main/keys").toString(), QString("01000100"));
    }

    void restoreDataRoundTrip()
    {
        TabPreset preset = examplePreset();
        TabPreset restored = PresetSerializer::fromRestoreData(PresetSerializer::toRestoreData(preset));
        QVERIFY(restored == preset);
    }

    void duplicateCCEntriesDropped()
    {
        QMap<QString, QVariant> channel;
        for (int id : {0, 1, 2})
        {
            QString prefix = QString("cc_%1_").arg(id, 3, 10, QLatin1Char('0'));
            channel[prefix + "active"] = "true";
            channel[prefix + "label"] = "reverb";
            channel[prefix + "key"] = "91";
            channel[prefix + "value"] = (id == 2) ? 50 : 40;   // the third one differs
            channel[prefix + "delay"] = 0;
        }
        QMap<QString, QVariant> data;
        data["0"] = channel;
        
        TabPreset preset = PresetSerializer::fromRestoreData(data);
        QCOMPARE(preset.cc_entries.length(), 2);
        QCOMPARE(preset.cc_entries.at(0).value, 40);
        QCOMPARE(preset.cc_entries.at(1).value, 50);
    }
    
    void missingKeysKeepDefaults()
    {
        QMap<QString, QVariant> channel;
        channel["volume"] = 50;
        QMap<QString, QVariant> data;
        data["0"] = channel;

        TabPreset preset = PresetSerializer::fromRestoreData(data);
        QVERIFY(preset.channels[0].activated);
        QCOMPARE(preset.channels[0].volume, 50);
        QCOMPARE(preset.channels[0].key_max, 127);
    }

    // Set VKEYBD_INI=/path/to/your/save.ini to check a real save file.
    // The file is copied first, the original is never touched.
    void existingSaveFileRoundTrip()
    {
        QString original = qEnvironmentVariable("VKEYBD_INI");
        if (original.isEmpty())
            QSKIP("set VKEYBD_INI to a vkeybd-qt save file to run this test");

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString source_path = dir.filePath("source.ini");
        QString target_path = dir.filePath("target.ini");
        QVERIFY(QFile::copy(original, source_path));

        QSettings source(source_path, QSettings::IniFormat);
        int checked = 0;

        QList<QString> keyboards = source.childGroups();
        for (const QString &keyboard : keyboards)
        {
            source.beginGroup(keyboard);
            QList<QString> labels = source.childGroups();
            source.endGroup();

            for (const QString &label : labels)
            {
                if (label == "general")
                    continue;

                TabPreset loaded = PresetSerializer::load(&source, keyboard.toInt(), label);
                {
                    QSettings target(target_path, QSettings::IniFormat);
                    target.clear();
                    PresetSerializer::save(&target, keyboard.toInt(), label, loaded);
                    target.sync();
                }
                QSettings target(target_path, QSettings::IniFormat);
                TabPreset reloaded = PresetSerializer::load(&target, keyboard.toInt(), label);

                if (reloaded != loaded)
                    QFAIL(qPrintable("round trip differs for " + keyboard + "/" + label));

                checked++;
            }
        }

        QVERIFY(checked > 0);
        qInfo() << "checked" << checked << "tabs";
    }
};

QTEST_GUILESS_MAIN(TestCore)
#include "tst_core.moc"
