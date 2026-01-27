#ifndef INPUTKEYBOARDRAW_H
#define INPUTKEYBOARDRAW_H

#include <QObject>
#include <QSocketNotifier>
#include <QThread>
#include <linux/input.h>
#include <unistd.h>
#include "enums_structs.h"

// ---------------- Worker ----------------
class InputKeyboardRawWorker : public QObject
{
    Q_OBJECT
public:
    explicit InputKeyboardRawWorker(int fd, QObject *parent = nullptr);
    ~InputKeyboardRawWorker() override;
    
    void stop();
    
public slots:
    void initialize();
    
signals:
    void rawKeyPressed(int code);
    void rawKeyReleased(int code);
    void deviceDisconnected();
    
private slots:
    void readEvent(int);
    
private:
    int fd = -1;
    QSocketNotifier *notifier = nullptr;
};

// ---------------- Meta ----------------
class InputKeyboardRawMeta : public QObject
{
    Q_OBJECT
public:
    explicit InputKeyboardRawMeta(QObject *parent = nullptr);
    
    QList<QMap<QString, QString>> detectKeyboards();
    QList<QString> getKeyboardNames();
    QString getPathForName(QString name);
    
private:
    QString getKeyboardName(QMap<QString, QString> keyboard);
};





// ---------------- Controller ----------------
class InputKeyboardRawController : public QObject
{
    Q_OBJECT
public:
    explicit InputKeyboardRawController(QObject *parent = nullptr);
    ~InputKeyboardRawController();
    
    void keyboardListen(QString devpath);
    void keyboardLock(QString devpath);
    void keyboardHelper(QString devpath, KeyboardMode mode);
    void keyboardRelease();
    
private slots:
    void rawKeyPressed(int keycode);
    void rawKeyReleased(int keycode);
    
signals:
    void deviceNotAvailable(QString message);
    void signalRawKeyPressed(int keycode);
    void signalRawKeyReleased(int keycode);
    
private:
    QThread *thread = nullptr;
    InputKeyboardRawWorker *worker = nullptr;
};

#endif // INPUTKEYBOARDRAW_H
