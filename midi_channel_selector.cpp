#include "midi_channel_selector.h"

MIDIChannelSelector::MIDIChannelSelector(QWidget *parent) : QWidget(parent)
{
    QGridLayout *grid = new QGridLayout;
    setLayout(grid);
    
    //grid->setContentsMargins(0, 0, 0, 0);
    
    QLabel *channel_label = new QLabel("Channel");
    QLabel *label_key_shift = new QLabel("Key Shift");
    QLabel *key_min_label = new QLabel("Key Min");
    QLabel *key_max_label = new QLabel("Key Max");
    QLabel *key_shift_label = new QLabel("Key Shift");
    QLabel *portamento_time = new QLabel("Port. Time");
    portamento_time->setToolTip("Portamento Time");
    
    
    grid->addWidget(channel_label, 0, 0);
    grid->addWidget(label_key_shift, 0, 1);
    grid->addWidget(key_min_label, 0, 2);
    grid->addWidget(key_max_label, 0, 4);
    grid->addWidget(key_shift_label, 0, 5);
    grid->addWidget(portamento_time, 0, 6);
    
    for (int i=1; i<=16; i++)
    {
        QCheckBox *check = new QCheckBox(QString::number(i));
        MIDIKeyShiftWidget *key_shift = new MIDIKeyShiftWidget;
        QSpinBox *key_min = new QSpinBox();
        QSpinBox *key_max = new QSpinBox();
        //QSpinBox *key_shift = new QSpinBox();
        
        key_min->setMaximum(127);
        key_max->setMaximum(127);
        
        key_min->setValue(0);
        key_max->setValue(127);
        
        QComboBox *combo_categories = new QComboBox;
        QStringList list_categories = MIDISoundsList::getCategories();
        combo_categories->addItems(list_categories);
        
        //QDial *dial_portamento = new QDial();
        //dial_portamento->resize(20, 20);
        QSlider *dial_portamento = new QSlider;
        dial_portamento->setOrientation(Qt::Horizontal);
        
        grid->addWidget(check, i, 0);
        grid->addWidget(key_shift, i, 1);
        
        grid->addWidget(key_min, i, 2);
        
        grid->addWidget(key_max, i, 4);
        grid->addWidget(combo_categories, i, 5);
        grid->addWidget(dial_portamento, i, 6);
        
        if (i==1)
        {
            check->setChecked(true);
        }
        
        QString stylesheet = "QCheckBox:indicator:checked {"
                             "  color: white;"
                             "  background-color: black;"
                             "}";
        
        check->setStyleSheet(stylesheet);
    }
}




MIDIKeyShiftWidget::MIDIKeyShiftWidget(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    QPushButton *button_lower = new QPushButton("<<");
    QPushButton *button_higher = new QPushButton(">>");
    button_higher->setMaximumWidth(30);
    button_lower->setMaximumWidth(30);
    
    connect(button_higher, &QPushButton::clicked, this, &MIDIKeyShiftWidget::higherShiftKeyPressed);
    connect(button_lower, &QPushButton::clicked, this, &MIDIKeyShiftWidget::lowerShiftKeyPressed);
    
    this->spin_key = new QSpinBox;
    this->spin_key->setMinimum(-64);
    this->spin_key->setMaximum(64);
    
    layout->addWidget(button_lower);
    layout->addWidget(this->spin_key);
    layout->addWidget(button_higher);
}

void MIDIKeyShiftWidget::lowerShiftKeyPressed()
{
    int val = this->spin_key->value();
    if (val >= -64+12)
    {
        val -= 12;
        this->spin_key->setValue(val);
    }
}

void MIDIKeyShiftWidget::higherShiftKeyPressed()
{
    int val = this->spin_key->value();
    if (val <= 64-12)
    {
        val += 12;
        this->spin_key->setValue(val);
    }
}
