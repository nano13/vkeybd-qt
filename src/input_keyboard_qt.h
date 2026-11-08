#ifndef INPUTKEYBOARDQT_H
#define INPUTKEYBOARDQT_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QSet>
#include <QMap>
#include <QLocale>
#include <QDebug>

#include "enums_structs.h"

class InputKeyboardQt : public QObject
{
    Q_OBJECT
public:
    explicit InputKeyboardQt(QObject *parent = nullptr);
    
    bool callEventFilter(QObject *obj, QEvent *ev);
    void releaseAllKeys();
    
private:
    // Track physical keys instead of Qt key symbols: Shift/layout changes must not
    // prevent a release from matching its original press.
    QSet<int> held_keys;

    int mapFromKeyboardLayoutToLinuxRaw(int key);
    
    void sendUDPMessage(QString message);
    
protected:
    
signals:
    void keyPressSignal(int key);
    void keyReleaseSignal(int key);
    //void toggleKeyboardLockSignal();
    
};

#endif // INPUTKEYBOARDQT_H
