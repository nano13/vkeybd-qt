#include <QtTest>
#include <QSignalSpy>

#include "input_keyboard_window.h"

class TestKeyboardWindow : public QObject
{
    Q_OBJECT

private slots:
    void pairedEventsUsePhysicalScanCode()
    {
        InputKeyboardWindow input;
        QSignalSpy pressed(&input, &InputKeyboardWindow::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardWindow::keyReleaseSignal);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, 38, 0, 0);
        QVERIFY(input.handleKeyEvent(&press));

        // A changed keyboard layout or Shift can change the Qt key symbol,
        // but not the scan code of the physical key.
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_At, Qt::ShiftModifier, 38, 0, 0);
        QVERIFY(input.handleKeyEvent(&release));
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 1);
        QCOMPARE(pressed.at(0).at(0).toInt(), 30);
        QCOMPARE(released.at(0).at(0).toInt(), 30);
    }

    void shortcutOverrideDoesNotPress()
    {
        InputKeyboardWindow input;
        QSignalSpy pressed(&input, &InputKeyboardWindow::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardWindow::keyReleaseSignal);

        QKeyEvent override_event(QEvent::ShortcutOverride, Qt::Key_B, Qt::NoModifier, 56, 0, 0);
        QVERIFY(input.handleKeyEvent(&override_event));
        QVERIFY(override_event.isAccepted());
        QCOMPARE(pressed.size(), 0);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, 56, 0, 0);
        QVERIFY(input.handleKeyEvent(&press));
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_B, Qt::NoModifier, 56, 0, 0);
        QVERIFY(input.handleKeyEvent(&release));
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 1);
        QCOMPARE(released.at(0).at(0).toInt(), 48);
    }

    void repeatsDoNotCreateExtraPressesOrReleases()
    {
        InputKeyboardWindow input;
        QSignalSpy pressed(&input, &InputKeyboardWindow::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardWindow::keyReleaseSignal);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_C, Qt::NoModifier, 54, 0, 0);
        QKeyEvent repeat_press(QEvent::KeyPress, Qt::Key_C, Qt::NoModifier, 54, 0, 0, QString(), true);
        QKeyEvent repeat_release(QEvent::KeyRelease, Qt::Key_C, Qt::NoModifier, 54, 0, 0, QString(), true);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_C, Qt::NoModifier, 54, 0, 0);
        QVERIFY(input.handleKeyEvent(&press));
        QVERIFY(input.handleKeyEvent(&repeat_press));
        QVERIFY(input.handleKeyEvent(&repeat_release));
        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 0);
        QVERIFY(input.handleKeyEvent(&release));
        QCOMPARE(released.size(), 1);
    }

    void focusLossReleasesEveryHeldKey()
    {
        InputKeyboardWindow input;
        QSignalSpy pressed(&input, &InputKeyboardWindow::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardWindow::keyReleaseSignal);

        QKeyEvent press_a(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, 38, 0, 0);
        QKeyEvent press_b(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, 56, 0, 0);
        QVERIFY(input.handleKeyEvent(&press_a));
        QVERIFY(input.handleKeyEvent(&press_b));
        QCOMPARE(pressed.size(), 2);

        input.releaseAllKeys();
        QCOMPARE(released.size(), 2);
        input.releaseAllKeys();
        QCOMPARE(released.size(), 2);

        QKeyEvent late_release(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier, 38, 0, 0);
        QVERIFY(input.handleKeyEvent(&late_release));
        QCOMPARE(released.size(), 2);
    }

    void syntheticEventsDoNotInventRawKeys()
    {
        InputKeyboardWindow input;
        QSignalSpy pressed(&input, &InputKeyboardWindow::keyPressSignal);
        QSignalSpy released(&input, &InputKeyboardWindow::keyReleaseSignal);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier);
        QVERIFY(!input.handleKeyEvent(&press));
        QVERIFY(!input.handleKeyEvent(&release));
        QCOMPARE(pressed.size(), 0);
        QCOMPARE(released.size(), 0);
    }
};

QTEST_GUILESS_MAIN(TestKeyboardWindow)
#include "tst_keyboard_window.moc"
