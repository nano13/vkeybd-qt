#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QGridLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QFile>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QIcon>
#include <QAbstractNativeEventFilter>
#include <QAbstractEventDispatcher>

//#include "orgelwerk.h"
//#include "keyboard_piano.h"
#include "main_tabs.h"
#include "menubar.h"
#include "config.h"
#include "help_message.h"
#include "help_about_widget.h"
#include "input_keyboard_raw.h"

#include <xcb/xcb.h>

//#include <QAbstractEventDispatcher>
//#include <QDebug>
//#include <QX11Info>
//#include <xcb/xcb.h>
//#include <X11/Xlib.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString output_system, int number_of_keyboards, QWidget *parent = nullptr);
    ~MainWindow();

private:
    QList<MainTabs*> list_of_maintabs;
    Config *config;
    
    QList<QLineEdit*> list_of_line_udp_ips;
    QList<QSpinBox*> list_of_spin_ports;
    QList<QPushButton*> list_of_network_help_buttons;
    
    QMap<QString, QString> key_keycode_map = {};
    
    QWidget* newKeyboardInstance(int id, QString mode);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    //bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private slots:
    void saveAllParams();
    void openAllParams();
    void restoreParams(int maintab, QString tab, QMap<QString, QVariant> data);
    void restoreGeneral(int maintab, QMap<QString,QVariant> data);
    
    void showActionChanged(QString name, bool is_checked);

};
#endif // MAINWINDOW_H
