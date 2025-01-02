#include "main_window.h"

MainWindow::MainWindow(OutputSystem output, int number_of_keyboards, QWidget *parent)
    : QMainWindow(parent)
{
    this->number_of_keyboards = number_of_keyboards;
    
    QWidget *main_container_widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    main_container_widget->setLayout(layout);
    setCentralWidget(main_container_widget);
    
    QFile css_file(":css_allen_heath");
    if (css_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        setStyleSheet(css_file.readAll());
    }
    
    if (output != OutputSystem::Network)
    {
        MenuBar *menu = new MenuBar;
        connect(menu, &MenuBar::signalSaveQuick, this, &MainWindow::saveParamsQuick);
        connect(menu, &MenuBar::signalLoadQuick, this, &MainWindow::loadParamsQuick);
        connect(menu, &MenuBar::signalSave, this, &MainWindow::saveParams);
        connect(menu, &MenuBar::signalLoad, this, &MainWindow::loadParams);
        
        connect(menu, &MenuBar::signalMIDISaveQuick, this, &MainWindow::saveMIDIQuick);
        connect(menu, &MenuBar::signalMIDILoadQuick, this, &MainWindow::loadMIDIQuick);
        connect(menu, &MenuBar::signalMIDISave, this, &MainWindow::saveMIDI);
        connect(menu, &MenuBar::signalMIDILoad, this, &MainWindow::loadMIDI);
        
        connect(menu, &MenuBar::signalShowActionChanged, this, &MainWindow::showActionChanged);
        connect(menu, &MenuBar::signalGlobalKeyShift, this, &MainWindow::showGlobalKeyShift);
        connect(menu, &MenuBar::signalResendMIDISettings, this, &MainWindow::globalResendMIDISettings);
        
        connect(menu, &MenuBar::signalHelpAbout, this, &MainWindow::helpAbout);
        setMenuBar(menu);
    }
    
    for (int i=0; i < this->number_of_keyboards; i++)
    {
        layout->addWidget(newKeyboardInstance(i, output));
    }

    connect(this->inputQt, &InputKeyboardQt::keyPressSignal, this, &MainWindow::rawKeyPressed);
    connect(this->inputQt, &InputKeyboardQt::keyReleaseSignal, this, &MainWindow::rawKeyReleased);                    
    
    connect(this->global_key_shift_widget, &MIDIKeyShiftGlobal::signalKeyShiftChanged, this, &MainWindow::globalKeyShiftValueChanged);
}

MainWindow::~MainWindow()
{
    
}

QWidget* MainWindow::newKeyboardInstance(int id, OutputSystem output)
{
    QWidget *widget = new QWidget;
    //QVBoxLayout *layout = new QVBoxLayout;
    QGridLayout *grid = new QGridLayout;
    //widget->setLayout(layout);
    widget->setLayout(grid);
    
    this->config = new Config;
    connect(this->config, &Config::restoreParams, this, &MainWindow::restoreParams);
    connect(this->config, &Config::restoreGeneral, this, &MainWindow::restoreGeneral);
    
    QPushButton *button_keyboard_lock = new QPushButton("Lock");
    button_keyboard_lock->setObjectName("button_grab");
    
    QPushButton *button_keyboard_rescan = new QPushButton();
    button_keyboard_rescan->setIcon(QIcon::fromTheme("system-reboot"));
    button_keyboard_rescan->setToolTip("Rescan Keyboards");
    
    ComboKeyboardSelect *combo_keyboard_selector = new ComboKeyboardSelect();
    InputKeyboardSelect *input_keyboard_select = new InputKeyboardSelect(combo_keyboard_selector, button_keyboard_lock, button_keyboard_rescan);
    connect(input_keyboard_select, &InputKeyboardSelect::keyboardSelectionChangedSignal, this, &MainWindow::keyboardSelectionChanged);
    input_keyboard_select->keyboardRescan();
    // set default value
    combo_keyboard_selector->setCurrentIndex(KeyboardSelection::Default);
    //combo_keyboard_selector->currentIndexChanged(combo_keyboard_selector->currentIndex());
    input_keyboard_select->keyboardSelectionChanged(combo_keyboard_selector->currentIndex());
    
    QLineEdit *line_udp_ip = new QLineEdit(this);
    line_udp_ip->setText("127.0.0.1");
    line_udp_ip->setToolTip("For remote-controlling: IP-Address of the interface you want to listen on (the IP-address of this machine).");
    line_udp_ip->setObjectName("network_select");
    //line_udp_ip->hide();
    this->list_of_line_udp_ips.append(line_udp_ip);
    
    QSpinBox *spin_port = new QSpinBox;
    spin_port->setMinimum(1025);
    spin_port->setMaximum(65535);
    spin_port->setValue(20020);
    spin_port->setToolTip("Network Listen Port");
    spin_port->setObjectName("network_select");
    //spin_port->hide();
    this->list_of_spin_ports.append(spin_port);
    
    this->switcher = new MainTabsSwitcher(id, this->config, this);
    connect(this->switcher, &MainTabsSwitcher::signalTabSwitched, this, &MainWindow::changeCurrentTab);
    connect(this->switcher, &MainTabsSwitcher::signalTabCheckChanged, this, &MainWindow::tabsCheckChanged);
    this->list_of_maintab_switchers.append(this->switcher);
    
    QList<QString> labels = this->switcher->getLabelsList();
    
    MainTabs *tabs = new MainTabs(labels, id, this->config, output, input_keyboard_select, line_udp_ip, spin_port, this);
    //connect(tabs, &MainTabs::currentChanged, this, [this, id](MainTabs::currentChanged index){ currentTabChanged(index, id); });
    connect(tabs, &MainTabs::currentChanged, this, [this, tabs, id](int index){ currentTabChanged(id, index); });
    //connect(tabs, &MainTabs::useInputKbdQtNativeSignal, this, &MainWindow::useInputKbdQtNative);
    //connect(tabs, &MainTabs::useInputKbdQtDefaultSignal, this, &MainWindow::useInputKbdQtDefault);
    //connect(tabs, &MainTabs::keyboardSelectionChanged, this, &MainWindow::keyboardSelectionChanged);
    connect(tabs, &MainTabs::signalKeyShiftChanged, this, &MainWindow::globalKeyShiftValueChanged);
    
    this->list_of_maintabs.append(tabs);
    
    QPushButton *button_lock_help = new QPushButton;
    button_lock_help->setIcon(QIcon::fromTheme("dialog-question"));
    button_lock_help->setToolTip("help");
    button_lock_help->hide();
    
    QPushButton *button_network_help = new QPushButton;
    button_network_help->setIcon(QIcon::fromTheme("dialog-question"));
    button_network_help->setToolTip("help");
    connect(button_network_help, &QPushButton::clicked, this, []{ new HelpMessage(":help_tntware"); });
    button_network_help->hide();
    this->list_of_network_help_buttons.append(button_network_help);
    
    grid->addWidget(combo_keyboard_selector, 1, 0);
    grid->addWidget(button_keyboard_lock, 1, 1);
    grid->addWidget(button_keyboard_rescan, 1, 2);
    //grid->addWidget(button_lock_help, 1, 3);
    if (output == OutputSystem::Network)
    {
        QLabel *label_udp_client_ip = new QLabel("IP-Address of remote vkeybd-qt instance:");
        grid->addWidget(label_udp_client_ip, 2, 0);
        
        QLabel *label_udp_client_port = new QLabel("Port:");
        grid->addWidget(label_udp_client_port, 2, 1);
    }
    else
    {
        line_udp_ip->hide();
        spin_port->hide();
    }
    grid->addWidget(line_udp_ip, 3, 0);
    grid->addWidget(spin_port, 3, 1, 1, 2);
    
    grid->addWidget(switcher, 4, 0, 1, 3);
    
    if (output != OutputSystem::Network)
    {
        grid->addWidget(tabs, 5, 0, 1, 3);
    }
    
    //int width = this->width();
    //resize(width, 900);
    //resize(10, 10);
    
    qDebug() << "native parent: " << nativeParentWidget();
    
    return widget;
}

void MainWindow::changeCurrentTab(int keyboard_id, int tab_id)
{
    this->list_of_maintabs.at(keyboard_id)->blockSignals(true);
    this->list_of_maintabs.at(keyboard_id)->setCurrentIndex(tab_id);
    this->list_of_maintabs.at(keyboard_id)->blockSignals(false);
}
void MainWindow::currentTabChanged(int keyboard_id, int tab_id)
{
    //this->list_of_maintab_switchers.at(keyboard_id)->blockSignals(true);
    this->list_of_maintab_switchers.at(keyboard_id)->pressButton(tab_id);
    //this->list_of_maintab_switchers.at(keyboard_id)->blockSignals(false);
}

void MainWindow::tabsCheckChanged(QList<int> list_of_tab_ids)
{
    qDebug() << "CHECK CHANGED" << list_of_tab_ids;
    //this->list_of_checked_tabs = list_of_tab_ids;
    for (int i=0; i < this->list_of_maintabs.length(); i++)
    {
        this->list_of_maintabs.at(i)->listOfCheckTabsChanged(list_of_tab_ids);
    }
}

void MainWindow::saveParams()
{
    QString filepath = this->config->getLastSavePath();
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Settings"), filepath, tr("INI Files (*.ini)"));
    
    if (filename.length() > 0)
        this->config->saveLastSavePath(filename);
    
    QSettings *settings = this->config->openSaveFile(filename);
    
    for (int i=0; i < this->list_of_maintabs.length(); i++)
    {
        this->list_of_maintabs.at(i)->saveParams(settings);
    }
}
void MainWindow::loadParams()
{
    QString filepath = this->config->getLastSavePath();
    QString filename = QFileDialog::getOpenFileName(this, tr("Load Settings"), filepath, tr("INI Files (*.ini)"));
    
    if (filename.length() > 0)
        this->config->saveLastSavePath(filename);
    
    QSettings *settings = this->config->openSaveFile(filename);
    
    this->config->loadChannelSettings(settings);
}

void MainWindow::saveParamsQuick()
{
    QSettings *settings = this->config->openQuicksaveFile(this->number_of_keyboards);
    
    for (int i=0; i < this->list_of_maintabs.length(); i++)
    {
        this->list_of_maintabs.at(i)->saveParams(settings);
    }
}
void MainWindow::loadParamsQuick()
{
    QSettings *settings = this->config->openQuicksaveFile(this->number_of_keyboards);
    this->config->loadChannelSettings(settings);
}
void MainWindow::restoreParams(int maintab, QString tab, QMap<QString, QVariant> data)
{
    if (this->list_of_maintabs.length() >= maintab+1)
    {
        this->list_of_maintabs.at(maintab)->restoreParams(tab, data);
    }
    
}
void MainWindow::restoreGeneral(int maintab, QMap<QString,QVariant> data)
{
    if (this->list_of_maintabs.length() >= maintab+1)
    {
        this->list_of_line_udp_ips.at(maintab)->setText(data["network_ip"].toString());
        this->list_of_spin_ports.at(maintab)->setValue(data["network_port"].toInt());
    }
}

void MainWindow::saveMIDIQuick()
{
    for (int i=0; i < this->list_of_maintabs.length(); i++)
    {
        this->list_of_maintabs.at(i)->saveMIDIConnections();
    }
}
void MainWindow::loadMIDIQuick()
{
    for (int i=0; i < this->list_of_maintabs.length(); i++)
    {
        this->list_of_maintabs.at(i)->loadMIDIConnections();
    }
}
void MainWindow::saveMIDI()
{
    
}
void MainWindow::loadMIDI()
{
    
}

void MainWindow::showActionChanged(GUIElements elements, bool is_checked)
{
    if (elements == GUIElements::Network)
    {
        for (auto &line : this->list_of_line_udp_ips)
        {
            if (is_checked)
                line->show();
            else
                line->hide();
        }
        for (auto &spin : this->list_of_spin_ports)
        {
            if (is_checked)
                spin->show();
            else
                spin->hide();
        }
        for (auto &button : this->list_of_network_help_buttons)
        {
            if (is_checked)
            {
                //button->show();
            }
            else
            {
                button->hide();
            }
        }
    }
    else
    {
        for (auto &maintab : this->list_of_maintabs)
        {
            maintab->showHideGUIElements(elements, is_checked);
        }
    }
    
    //resize(10, 10);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    return this->inputQt->callEventFilter(obj, ev);
}

void MainWindow::keyboardSelectionChanged(int selection, bool locked)
{
    this->keyboard_selection = selection;
    
    if (selection == KeyboardSelection::Default)
    {
        installEventFilter(this);
        
        if (locked)
            grabKeyboard();
        else
            releaseKeyboard();
    }
    else if (selection == KeyboardSelection::Native)
    {
        if (locked)
            grabKeyboard();
        else
            releaseKeyboard();
    }
    else
    {
        removeEventFilter(this);
    }
}

void MainWindow::rawKeyPressed(int key)
{
    this->list_of_maintabs.first()->rawKeyPressed(key);
}
void MainWindow::rawKeyReleased(int key)
{
    this->list_of_maintabs.first()->rawKeyReleased(key);
}

void MainWindow::showGlobalKeyShift()
{
    //connect(global_key_shift_widget, &MIDIKeyShiftGlobal::signalKeyShiftChanged, this, &MainWindow::globalPitchShiftValueChanged);
    
    global_key_shift_widget->show();
}
void MainWindow::globalKeyShiftValueChanged(int value, bool is_relative)
{
    for (int i=0; i < this->list_of_maintabs.length(); i++)
    {
        this->list_of_maintabs.at(i)->globalKeyShiftChanged(value, is_relative);
    }
}

void MainWindow::globalResendMIDISettings()
{
    for (int i=0; i < this->list_of_maintabs.length(); i++)
    {
        this->list_of_maintabs.at(i)->globalResendMIDISettings();
    }
}

void MainWindow::helpAbout()
{
    HelpAboutWidget *about = new HelpAboutWidget;
    about->setMinimumSize(600, 600);
    about->show();
}
