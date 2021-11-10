#include "midi_channel_selector.h"

MIDIChannelSelector::MIDIChannelSelector(InterfaceAudio *audio, QWidget *parent) : QWidget(parent)
{
    this->audio = audio;
    
    QGridLayout *grid = new QGridLayout;
    setLayout(grid);
    
    QLabel *channel_label = new QLabel("Channel");
    QLabel *label_volume = new QLabel("Volume");
    QLabel *label_key_shift = new QLabel("Key Shift");
    QLabel *key_min_label = new QLabel("Key Min");
    QLabel *key_max_label = new QLabel("Key Max");
    QLabel *label_instrument_group = new QLabel("Instrument Group");
    QLabel *label_instrument = new QLabel("Instrument");
    QLabel *portamento_time = new QLabel("Port. Time");
    portamento_time->setToolTip("Portamento Time");
    QLabel *label_attack = new QLabel("Attack");
    QLabel *label_release = new QLabel("Release");
    
    
    grid->addWidget(channel_label, 0, 0);
    grid->addWidget(label_volume, 0, 1);
    grid->addWidget(label_key_shift, 0, 2);
    grid->addWidget(key_min_label, 0, 3);
    grid->addWidget(key_max_label, 0, 4);
    grid->addWidget(label_instrument_group, 0, 5);
    grid->addWidget(label_instrument, 0, 6);
    grid->addWidget(portamento_time, 0, 7);
    grid->addWidget(label_attack, 0, 8);
    grid->addWidget(label_release, 0, 9);
    
    for (int i=1; i<=16; i++)
    {
        QCheckBox *check = new QCheckBox(QString::number(i));
        
        QSlider *slider_volume = new QSlider;
        slider_volume->setOrientation(Qt::Horizontal);
        slider_volume->setMinimum(0);
        slider_volume->setMaximum(127);
        slider_volume->setValue(127);
        connect(slider_volume, &QSlider::valueChanged, this, [this, i, slider_volume]{ MIDIChannelSelector::volumeSliderMoved(i, slider_volume->value()); });
        
        MIDIKeyShiftWidget *key_shift = new MIDIKeyShiftWidget;
        QSpinBox *key_min = new QSpinBox();
        QSpinBox *key_max = new QSpinBox();
        //QSpinBox *key_shift = new QSpinBox();
        
        key_min->setMaximum(127);
        key_max->setMaximum(127);
        
        key_min->setValue(0);
        key_max->setValue(127);
        
        QComboBox *combo_instrument_group = new QComboBox;
        //QStringList list_instrument_group = MIDISoundsList::getCategories();
        QList<QString> list_instrument_group = this->midi_sounds_list->getInstrumentGroups();
        combo_instrument_group->addItems(list_instrument_group);
        
        QComboBox *combo_instrument = new QComboBox;
        QList<QString> list_instrument = this->midi_sounds_list->getInstrumentsForGroupMIDIv1(list_instrument_group.at(0));
        combo_instrument->addItems(list_instrument);
        
        connect(combo_instrument_group, &QComboBox::currentTextChanged, this, [this, i, combo_instrument_group, combo_instrument]{ MIDIChannelSelector::instrumentGroupChanged(i-1, combo_instrument_group, combo_instrument); });
        connect(combo_instrument, &QComboBox::currentTextChanged, this, [this, i, combo_instrument]{ MIDIChannelSelector::instrumentChanged(i-1, combo_instrument); });
        
        //QDial *dial_portamento = new QDial();
        //dial_portamento->resize(20, 20);
        QSlider *dial_portamento = new QSlider;
        dial_portamento->setOrientation(Qt::Horizontal);
        
        QSlider *slider_attack = new QSlider;
        slider_attack->setOrientation(Qt::Horizontal);
        slider_attack->setRange(0, 127);
        connect(slider_attack, &QSlider::valueChanged, this, [this, i, slider_attack]{ MIDIChannelSelector::attackChanged(i, slider_attack); });
        
        QSlider *slider_release = new QSlider;
        slider_release->setOrientation(Qt::Horizontal);
        slider_release->setRange(0, 127);
        connect(slider_release, &QSlider::valueChanged, this, [this, i, slider_release]{ MIDIChannelSelector::releaseChanged(i, slider_release); });
        
        grid->addWidget(check, i, 0);
        grid->addWidget(slider_volume, i, 1);
        grid->addWidget(key_shift, i, 2);
        
        grid->addWidget(key_min, i, 3);
        
        grid->addWidget(key_max, i, 4);
        grid->addWidget(combo_instrument_group, i, 5);
        grid->addWidget(combo_instrument, i, 6);
        grid->addWidget(dial_portamento, i, 7);
        
        grid->addWidget(slider_attack, i, 8);
        grid->addWidget(slider_release, i, 9);
        
        if (i==1)
        {
            check->setChecked(true);
        }
        
        QString stylesheet = "QCheckBox:indicator:checked {"
                             "  color: white;"
                             "  background-color: black;"
                             "}";
        
        check->setStyleSheet(stylesheet);
        
        this->list_of_checkboxes.append(check);
        this->list_of_keyshifts.append(key_shift);
        this->list_of_key_mins.append(key_min);
        this->list_of_key_maxs.append(key_max);
    }
}

QList<QMap<QString,int>> MIDIChannelSelector::getListOfActivatedChannels()
{
    QList<QMap<QString,int>> result;
    
    for (int i=0; i < this->list_of_checkboxes.length(); i++)
    {
        QCheckBox *check = this->list_of_checkboxes.at(i);
        if (check->isChecked())
        {
            QMap<QString,int> map;
            
            map["channel"] = i;
            
            int key_shift = this->list_of_keyshifts.at(i)->spin_key->value();
            map["key_shift"] = key_shift;
            
            int key_min = this->list_of_key_mins.at(i)->value();
            map["key_min"] = key_min;
            
            int key_max = this->list_of_key_maxs.at(i)->value();
            map["key_max"] = key_max;
            
            result.append(map);
        }
    }
    
    return result;
}

void MIDIChannelSelector::volumeSliderMoved(int channel, int volume)
{
    emit volumeChangedSignal(channel, volume);
}

void MIDIChannelSelector::instrumentGroupChanged(int channel, QComboBox *combo_group, QComboBox *combo_instrument)
{
    combo_instrument->blockSignals(true);
    combo_instrument->clear();
    
    QString group = combo_group->currentText();
    QList<QString> instruments = this->midi_sounds_list->getInstrumentsForGroupMIDIv1(group);
    //QList<QString> instruments = this->midi_sounds_list->getInstrumentsForGroupMIDIv2(group);
    
    combo_instrument->addItems(instruments);
    combo_instrument->blockSignals(false);
    
    instrumentChanged(channel, combo_instrument);
}

void MIDIChannelSelector::instrumentChanged(int channel, QComboBox *combo_instrument)
{
    QString name = combo_instrument->currentText();
    qDebug() << "changed: "+name;
    
    QList<int> codes = this->midi_sounds_list->getMIDICodesForInstrument(name);
    int program = codes.at(0) - 1;
    int bank = codes.at(1);
    
    emit instrumentChangedSignal(channel, program, bank);
}

void MIDIChannelSelector::attackChanged(int channel, QSlider *slider)
{
    this->audio->setAttackChanged(channel, slider->value());
}

void MIDIChannelSelector::releaseChanged(int channel, QSlider *slider)
{
    this->audio->setReleaseChanged(channel, slider->value());
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
