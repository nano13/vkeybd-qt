#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QObject>
#include <QGridLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QFile>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QIcon>
#include <QAbstractEventDispatcher>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QFrame>
#include <QFileDialog>

#include "main_tabs.h"
#include "menubar.h"
#include "config.h"
#include "help_message.h"
#include "help_about_widget.h"

#include "input_keyboard_window.h"
#include "combo_keyboard_select.h"

#include "midi_key_shift_widget.h"
#include "midi_key_shift_global.h"

#include "main_tabs_switcher.h"


#include "enums_structs.h"

#include "interface_notify.h"
#include "interface_notify_dbus.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(OutputSystem output, int number_of_keyboards, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QList<MainTabsSwitcher*> list_of_maintab_switchers;
    QList<MainTabs*> list_of_maintabs;
    Config *config;
    int number_of_keyboards;
    
    QList<QLineEdit*> list_of_line_udp_ips;
    QList<QSpinBox*> list_of_spin_ports;
    QList<QPushButton*> list_of_network_help_buttons;
    
    QMap<QString, QString> key_keycode_map = {};
    
    MainTabsSwitcher *switcher;
    void changeCurrentTab(int keyboard_id, int tab_id);
    void currentTabChanged(int keyboard_id, int tab_id);
    void tabsCheckChanged(int keyboard_id, QList<int> list_of_tab_ids);
    
    QWidget* newKeyboardInstance(int id, OutputSystem output);
    
    // per keyboard: selected input (KeyboardSelection or raw device index) and lock state
    QList<int> keyboard_selections;
    QList<bool> keyboard_locks;
    
    // "Focused window" input; its keys go to every keyboard that selected it
    InputKeyboardWindow *input_window = new InputKeyboardWindow(this);
    void updateWindowInput();
    
    MIDIKeyShiftGlobal *global_key_shift_widget = new MIDIKeyShiftGlobal(this);
    
    InterfaceNotify *notify_dbus;    

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void saveParamsQuick();
    void loadParamsQuick();
    void loadParamsAuto();
    void saveParams();
    void loadParams();
    //void openAllParams();
    void restoreParams(int maintab, QString tab, QMap<QString, QVariant> data);
    void restoreGeneral(int maintab, QMap<QString,QVariant> data);
    
    
    void showActionChanged(GUIElements elements, bool is_checked);
    
    void keyboardSelectionChanged(int keyboard_id, int selection, bool locked);
    
    void rawKeyPressed(int key);
    void rawKeyReleased(int key);
    //void MIDISignal(MIDISignalTypes type);
    
    void showGlobalKeyShift();
    void globalKeyShiftValueChanged(int value, bool is_relative);
    
    void globalResendMIDISettings();
    
    void helpAbout();
};
#endif // MAINWINDOW_H
