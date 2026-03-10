#include "main_tabs_switcher.h"


QRightClickButton::QRightClickButton(QString label, QWidget *parent)
    : QPushButton{parent}
{
    setText(label);
}

void QRightClickButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton)
        emit rightClicked();
    else
        QPushButton::mousePressEvent(e);
}



MainTabsSwitcher::MainTabsSwitcher(int keyboard_id, Config *config, InterfaceNotify *notify, QWidget *parent)
    : QWidget{parent}
{
    this->keyboard_id = keyboard_id;
    this->config = config;
    this->notify_dbus = notify;
    
    this->grid = new QGridLayout;
    setLayout(this->grid);
    this->grid->setContentsMargins(0, 0, 0, 0);
    this->grid->setHorizontalSpacing(0);
    this->grid->setVerticalSpacing(0);
    
    int tab_id = -1;
    this->layers_number = this->config->getNumberOfLayers();
    for (int x=0; x < this->layers_number; x++)
    {
        for (int y=0; y < 12; y++)
        {
            tab_id++;
            
            QString label = "F" +
                    QString::number(y+1).rightJustified(2, '0') +
                    "-" +
                    QString::number(x+1);
            
            QRightClickButton *button = new QRightClickButton(label);
            button->setMaximumSize(33, 18);
            button->setCheckable(true);
            this->list_of_buttons.append(button);
            this->list_labels.append(label);
            
            this->colorizeTabs(button, x, y);
            
            this->grid->addWidget(button, x, y);
            
            connect(button, &QRightClickButton::clicked, this, [this, button, tab_id]{ leftClicked(button, tab_id); });
            connect(button, &QRightClickButton::rightClicked, this, [this, button, tab_id]{ rightClicked(button, tab_id); });
        }
    }
    
    // to activate first tab on startup
    this->list_of_buttons.first()->setChecked(true);
}

void MainTabsSwitcher::leftClicked(QRightClickButton *button, int tab_id)
{
    emit signalTabSwitched(this->keyboard_id, tab_id);
    
    QList<int> tabs_checked;
    
    // uncheck all the other buttons
    for (int i=0; i < this->list_of_buttons.length(); i++)
    {
        QRightClickButton *button_current = this->list_of_buttons.at(i);
        if (button_current != button)
        {
            button_current->setChecked(false);
        }
        else
        {
            tabs_checked.append(i);
        }
    }
    
    button->setChecked(true);
    
    emit signalTabCheckChanged(tabs_checked);
}
void MainTabsSwitcher::rightClicked(QRightClickButton *button, int tab_id)
{
    if (button->isChecked())
    {
        button->setChecked(false);
        
        // we need to check if the currently active tab became inactive. if this happens, change to the first active tab instead
        activateLeftmostTab();
    }
    else
    {
        button->setChecked(true);
    }
    
    QList<int> tabs = getCheckedTabsList();
    emit signalTabCheckChanged(tabs);
}

void MainTabsSwitcher::pressButton(int button_id)
{
    for (int i=0; i < this->list_of_buttons.length(); i++)
    {
        this->list_of_buttons.at(i)->setChecked(false);
    }
    
    this->list_of_buttons.at(button_id)->setChecked(true);
    QString label = this->list_of_buttons.at(button_id)->text();
    
    QList<int> tabs = getCheckedTabsList();
    emit signalTabCheckChanged(tabs);
    
    this->notify_dbus->sendTabChangeNotification(label);
}

QList<QString> MainTabsSwitcher::getLabelsList()
{
    return this->list_labels;
}

QList<int> MainTabsSwitcher::getCheckedTabsList()
{
    QList<int> result;
    
    for (int i=0; i < this->list_of_buttons.length(); i++)
    {
        if (this->list_of_buttons.at(i)->isChecked())
        {
            result.append(i);
        }
    }
    
    return result;
}

void MainTabsSwitcher::setLayerActive(int layer)
{
    // layers_number counts from 1 because it comes from config.
    // letting the human put in 0 for one layer of keyboards would be a bit strange ...
    // however layer counts from 0
    if (layer < this->layers_number)
    {
        this->layer_active = layer;
        
        // change colors of selected layer
        int tab_id = -1;
        for (int x=0; x < this->layers_number; x++)
        {
            for (int y=0; y < 12; y++)
            {
                tab_id++;
                
                QRightClickButton *button = this->list_of_buttons.at(tab_id);
                
                this->colorizeTabs(button, x, y);
                
                button->style()->unpolish(button);
                button->style()->polish(button);
                button->update();
            }
        }
    }
}
int MainTabsSwitcher::getLayerActive()
{
    return this->layer_active;
}

void MainTabsSwitcher::activateLeftmostTab()
{
    QList<int> checked_tabs = getCheckedTabsList();
    int tab_id = checked_tabs.first();
    
    emit signalTabSwitched(this->keyboard_id, tab_id);
}

void MainTabsSwitcher::colorizeTabs(QPushButton *button, int layers_counter, int tabs_counter)
{
    if (layers_counter == this->layer_active)
    {
        if (tabs_counter >= 4 & tabs_counter <= 7)
            button->setObjectName("main_tab_switcher_button_a");
        else
            button->setObjectName("main_tab_switcher_button_b");
    }
    else
    {
        if (tabs_counter >= 4 & tabs_counter <= 7)
            button->setObjectName("main_tab_switcher_button_b");
        else
            button->setObjectName("main_tab_switcher_button_a");
    }
}
