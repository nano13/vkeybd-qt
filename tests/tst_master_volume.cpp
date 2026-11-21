#include <QtTest>

#include "enums_structs.h"
#include "midi_mastervolume.h"

/*
 * Down / Up change the volume temporarily: when released, it springs back to
 * the value it had before -- not to 100 %.
 */
class TestMasterVolume : public QObject
{
    Q_OBJECT
    
    static void hold(MIDIMasterVolume &volume, int key, int milliseconds)
    {
        volume.volumeKeyPressed(key);
        QTest::qWait(milliseconds);
        volume.volumeKeyReleased();
    }
    
private slots:
    void springsBackToTheValueBefore()
    {
        MIDIMasterVolume volume;
        volume.slider_tether->setValue(150); // fast, for the test
        volume.setValueSilently(100);
        
        // a permanent change sets the value to return to
        hold(volume, KeysRaw::PageDown, 60);
        QTest::qWait(50);
        const int base = volume.getValue();
        QVERIFY(base < 100);
        
        hold(volume, KeysRaw::Down, 60);
        QVERIFY(volume.getValue() < base);
        QTRY_COMPARE_WITH_TIMEOUT(volume.getValue(), base, 3000);
        
        hold(volume, KeysRaw::Up, 60);
        QVERIFY(volume.getValue() > base);
        QTRY_COMPARE_WITH_TIMEOUT(volume.getValue(), base, 3000);
    }
    
    void valueOfAnotherTabIsTheRestValue()
    {
        MIDIMasterVolume volume;
        volume.slider_tether->setValue(150);
        
        // e.g. switching to a tab at 70 %
        volume.setValueSilently(70);
        hold(volume, KeysRaw::Up, 60);
        QVERIFY(volume.getValue() > 70);
        QTRY_COMPARE_WITH_TIMEOUT(volume.getValue(), 70, 3000);
    }
    
    void pressingAgainWhileSpringingBackKeepsTheOriginalValue()
    {
        MIDIMasterVolume volume;
        volume.slider_tether->setValue(150);
        volume.setValueSilently(90);
        
        hold(volume, KeysRaw::Down, 60);
        QTest::qWait(10); // springing back, not there yet
        hold(volume, KeysRaw::Up, 120);
        QTRY_COMPARE_WITH_TIMEOUT(volume.getValue(), 90, 3000);
    }
};

QTEST_MAIN(TestMasterVolume)
#include "tst_master_volume.moc"
