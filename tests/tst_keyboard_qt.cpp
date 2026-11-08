#include <QtTest>
#include <QSignalSpy>

#include "input_keyboard_qt.h"

class TestKeyboardQt : public QObject
{
    Q_OBJECT

private slots:
    void pairedEventsUsePhysicalScanCode()
    {
        InputKeyboardQt input;
        QSignalSpy pressed(&input, &InputKeyboardQt::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardQt::keyReleaseSignal);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, 38, 0, 0);
        QVERIFY(input.callEventFilter(&input, &press));

        // A changed keyboard layout or Shift can change the Qt key symbol,
        // but not the scan code of the physical key.
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_At, Qt::ShiftModifier, 38, 0, 0);
        QVERIFY(input.callEventFilter(&input, &release));
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 1);
        QCOMPARE(pressed.at(0).at(0).toInt(), 30);
        QCOMPARE(released.at(0).at(0).toInt(), 30);
    }

    void shortcutOverrideDoesNotPress()
    {
        InputKeyboardQt input;
        QSignalSpy pressed(&input, &InputKeyboardQt::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardQt::keyReleaseSignal);

        QKeyEvent override_event(QEvent::ShortcutOverride, Qt::Key_B, Qt::NoModifier, 56, 0, 0);
        QVERIFY(input.callEventFilter(&input, &override_event));
        QVERIFY(override_event.isAccepted());
        QCOMPARE(pressed.size(), 0);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, 56, 0, 0);
        QVERIFY(input.callEventFilter(&input, &press));
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_B, Qt::NoModifier, 56, 0, 0);
        QVERIFY(input.callEventFilter(&input, &release));
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 1);
        QCOMPARE(released.at(0).at(0).toInt(), 48);
    }

    void repeatsDoNotCreateExtraPressesOrReleases()
    {
        InputKeyboardQt input;
        QSignalSpy pressed(&input, &InputKeyboardQt::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardQt::keyReleaseSignal);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_C, Qt::NoModifier, 54, 0, 0);
        QKeyEvent repeat_press(QEvent::KeyPress, Qt::Key_C, Qt::NoModifier, 54, 0, 0, QString(), true);
        QKeyEvent repeat_release(QEvent::KeyRelease, Qt::Key_C, Qt::NoModifier, 54, 0, 0, QString(), true);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_C, Qt::NoModifier, 54, 0, 0);
        QVERIFY(input.callEventFilter(&input, &press));
        QVERIFY(input.callEventFilter(&input, &repeat_press));
        QVERIFY(input.callEventFilter(&input, &repeat_release));
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 0);
        QVERIFY(input.callEventFilter(&input, &release));
        QCOMPARE(released.size(), 1);
    }

    void focusLossReleasesEveryHeldKey()
    {
        InputKeyboardQt input;
        QSignalSpy pressed(&input, &InputKeyboardQt::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardQt::keyReleaseSignal);

        QKeyEvent press_a(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, 38, 0, 0);
        QKeyEvent press_b(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, 56, 0, 0);
        QVERIFY(input.callEventFilter(&input, &press_a));
        QVERIFY(input.callEventFilter(&input, &press_b));
        QCOMPARE(pressed.size(), 2);

        input.releaseAllKeys();
        QCOMPARE(released.size(), 2);
        input.releaseAllKeys();
        QCOMPARE(released.size(), 2);

        QKeyEvent late_release(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier, 38, 0, 0);
        QVERIFY(input.callEventFilter(&input, &late_release));
        QCOMPARE(released.size(), 2);
    }

    void syntheticEventsDoNotInventRawKeys()
    {
        InputKeyboardQt input;
        QSignalSpy pressed(&input, &InputKeyboardQt::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardQt::keyReleaseSignal);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier);
        QVERIFY(!input.callEventFilter(&input, &press));
        QVERIFY(!input.callEventFilter(&input, &release));
        QCOMPARE(pressed.size(), 0);
        QCOMPARE(released.size(), 0);
    }
};

QTEST_GUILESS_MAIN(TestKeyboardQt)
#include "tst_keyboard_qt.moc"
