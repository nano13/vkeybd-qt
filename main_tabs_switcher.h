#ifndef QRIGHTCLICKBUTTON_H
#define QRIGHTCLICKBUTTON_H

#include <QObject>
#include <QWidget>

#include <QPushButton>
#include <QMouseEvent>

class QRightClickButton : public QPushButton
{
    Q_OBJECT
    
public:
    explicit QRightClickButton(QString label, QWidget *parent = 0);
    
private slots:
    void mousePressEvent(QMouseEvent *e);
    
signals:
    void rightClicked();
    
public slots:
    
};
#endif // QRIGHTCLICKBUTTON_H



#ifndef MAINTABSSWITCHER_H
#define MAINTABSSWITCHER_H

#include <QObject>
#include <QWidget>

#include <QGridLayout>
#include <QPushButton>
#include <QList>
#include <QStyle>

#include <QDebug>

#include "config.h"

#include "interface_notify.h"
#include "interface_notify_dbus.h"

class MainTabsSwitcher : public QWidget
{
    Q_OBJECT
public:
    explicit MainTabsSwitcher(int keyboard_id = 0, Config *config = nullptr, InterfaceNotify *notify = nullptr, QWidget *parent = nullptr);
    
    // called if tab was changed by hotkey to keep this widget synchronised
    void pressButton(int button_id);
    QList<QString> getLabelsList();
    
    QList<int> getCheckedTabsList();
    
    void setLayerActive(int layer);
    int getLayerActive();
    
private:
    int keyboard_id;
    Config *config;
    QGridLayout *grid;
    
    QList<QRightClickButton*> list_of_buttons;
    QList<QString> list_labels;
    
    void leftClicked(QRightClickButton *button, int tab_id);
    void rightClicked(QRightClickButton *button, int tab_id);
    
    void activateLeftmostTab();
    
    void colorizeTabs(QPushButton *button, int layers_counter, int tabs_counter);
    
    InterfaceNotify *notify_dbus;
    
    int layers_number = 1;
    int layer_active = 0;
    
signals:
    void signalTabSwitched(int keyboard_id, int tab_id);
    void signalTabCheckChanged(QList<int> list_of_tab_ids);
};

#endif // MAINTABSSWITCHER_H
