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
        // row button: the visible (and clickable) form of Esc + F<row>
        QPushButton *row_button = new QPushButton(QString::number(x+1));
        row_button->setObjectName("main_tab_switcher_row");
        row_button->setCheckable(true);
        row_button->setFixedSize(18, 18);
        row_button->setFocusPolicy(Qt::NoFocus);
        QString row_tooltip = QString("Row %1 — shortcut: Esc + F%1\n"
                                      "Hold Esc and press F%1 to make this row active;\n"
                                      "F1 … F12 then switch to the tabs of this row.")
                .arg(x+1);
        if (this->layers_number > 1)
            row_tooltip += QString("\n\nAll rows: Esc + F1 … Esc + F%1").arg(qMin(this->layers_number, 12));
        row_button->setToolTip(row_tooltip);
        this->list_of_row_buttons.append(row_button);
        this->grid->addWidget(row_button, x, 0);
        connect(row_button, &QPushButton::clicked, this, [this, x]{ setLayerActive(x); });
        
        for (int y=0; y < 12; y++)
        {
            tab_id++;
            
            QString label = "F" +
                    QString::number(y+1).rightJustified(2, '0') +
                    "-" +
                    QString::number(x+1);
            
            QRightClickButton *button = new QRightClickButton(label);
            // minimum width, fixed height; the columns stretch evenly, so the
            // buttons fill exactly the width of the section below
            button->setMinimumWidth(33);
            button->setFixedHeight(18);
            button->setFocusPolicy(Qt::NoFocus);
            button->setCheckable(true);
            button->setToolTip(QString("Tab %1\n"
                                       "Left click: play this tab\n"
                                       "Right click: play it additionally (several tabs at once)\n"
                                       "Shortcuts: F%2 selects, Menu + F%2 adds/removes it,\n"
                                       "while row %3 is active (row: Esc + F%3)")
                               .arg(label).arg(y+1).arg(x+1));
            this->list_of_buttons.append(button);
            this->list_labels.append(label);
            
            this->colorizeTabs(button, x, y);
            
            this->grid->addWidget(button, x, y + 1);
            
            connect(button, &QRightClickButton::clicked, this, [this, button, tab_id]{ leftClicked(button, tab_id); });
            connect(button, &QRightClickButton::rightClicked, this, [this, button, tab_id]{ rightClicked(button, tab_id); });
        }
    }
    
    // the tab columns share the width; the row button column keeps its size
    for (int column = 1; column <= 12; column++)
        this->grid->setColumnStretch(column, 1);
    
    // to activate first tab on startup
    this->list_of_buttons.first()->setChecked(true);
    updateRowButtons();
}

void MainTabsSwitcher::updateRowButtons()
{
    for (int row = 0; row < this->list_of_row_buttons.length(); row++)
        this->list_of_row_buttons.at(row)->setChecked(row == this->layer_active);
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
        // the last checked tab stays checked: one tab always has to be active
        if (getCheckedTabsList().length() <= 1)
            return;
        
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

void MainTabsSwitcher::toggleTabChecked(int tab_id)
{
    if (tab_id < 0 || tab_id >= this->list_of_buttons.length())
        return;
    rightClicked(this->list_of_buttons.at(tab_id), tab_id);
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
    // also when the active row is clicked again: a row button never unchecks itself
    updateRowButtons();
    
    if (layer >= 0 && layer < this->layers_number)
    {
        this->layer_active = layer;
        updateRowButtons();
        
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
