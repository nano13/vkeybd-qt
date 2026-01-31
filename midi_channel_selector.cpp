#include "midi_channel_selector.h"

MIDIChannelSelector::MIDIChannelSelector(InterfaceAudio *interface_audio, int port, QWidget *parent) : QWidget(parent)
{
    this->interface_audio = interface_audio;
    this->port = port;
    
    drawGUI();
    
    installEventFilter(this);
    
    setObjectName("sub_window");
    //setAttribute(Qt::WA_TranslucentBackground);
    QFile css_file(":css_allen_heath");
    if (css_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        setStyleSheet(css_file.readAll());
    }
}

void MIDIChannelSelector::drawGUI()
{
    setLayout(this->grid);
    grid->setSizeConstraint( QLayout::SetFixedSize );
    
    QLabel *label_channel_a = new QLabel("Channel");
    QLabel *label_channel_b = new QLabel("Channel");
    QLabel *label_output = new QLabel(""); // nothing, CC-buttons are shown here
    QLabel *label_volume = new QLabel("Volume");
    QLabel *label_pan = new QLabel("Pan");
    QLabel *label_key_shift = new QLabel("Key Shift");
    QLabel *label_key_min = new QLabel("Key Min");
    QLabel *label_key_max = new QLabel("Key Max");
    QLabel *label_instrument_group = new QLabel("Instrument Group");
    QLabel *label_instrument = new QLabel("Instrument");
    QLabel *label_midi_group = new QLabel("MSB");
    QLabel *label_midi_bank = new QLabel("LSB");
    QLabel *label_midi_velocity = new QLabel("Velocity");
    QLabel *label_midi_pitch = new QLabel("Pitch");
    QLabel *label_portamento_time = new QLabel("Portamento");
    QLabel *label_attack = new QLabel("Attack");
    QLabel *label_release = new QLabel("Release");
    QLabel *label_tremolo = new QLabel("Tremolo");
    
    this->grid->addWidget(label_channel_a, 0, 0);
    this->grid->addWidget(label_output, 0, 1);
    this->grid->addWidget(label_volume, 0, 2);
    this->grid->addWidget(label_pan, 0, 3);
    this->grid->addWidget(label_key_shift, 0, 4);
    this->grid->addWidget(label_key_min, 0, 5);
    this->grid->addWidget(label_key_max, 0, 6);
    this->grid->addWidget(label_instrument_group, 0, 7);
    this->grid->addWidget(label_instrument, 0, 8);
    this->grid->addWidget(label_midi_group, 0, 9);
    this->grid->addWidget(label_midi_bank, 0, 10);
    this->grid->addWidget(label_midi_velocity, 0, 11);
    this->grid->addWidget(label_midi_pitch, 0, 12);
    this->grid->addWidget(label_portamento_time, 0, 13);
    this->grid->addWidget(label_attack, 0, 14);
    this->grid->addWidget(label_release, 0, 15);
    this->grid->addWidget(label_tremolo, 0, 16);
    this->grid->addWidget(label_channel_b, 0, 17);
    
    for (int i=1; i <= 16; i++)
    {
        QCheckBox *check_a = new QCheckBox(QString::number(i));
        QCheckBox *check_b = new QCheckBox(QString::number(i));
        connect(check_a, &QCheckBox::toggled, this, [this, check_a, check_b]{ checkToggled(check_a, check_b); });
        connect(check_b, &QCheckBox::toggled, this, [this, check_b, check_a]{ checkToggled(check_b, check_a); });
        
        QPushButton *button_cc_add = new QPushButton("add CC");
        QMap<QString,QVariant> cc_map;
        connect(button_cc_add, &QPushButton::clicked, this, [this, i, cc_map]{ addNewCCEntry(i-1, cc_map); });
        
        QSlider *slider_volume = new QSlider;
        slider_volume->setOrientation(Qt::Horizontal);
        slider_volume->setMinimum(0);
        slider_volume->setMaximum(127);
        slider_volume->setValue(127);
        connect(slider_volume, &QSlider::valueChanged, this, [this, i, slider_volume]{ MIDIChannelSelector::volumeSliderMoved(i-1, slider_volume->value()); });
        this->list_of_volume_sliders.append(slider_volume);
        
        QSlider *slider_pan = new QSlider;
        slider_pan->setOrientation(Qt::Horizontal);
        slider_pan->setRange(0, 127);
        slider_pan->setValue(64);
        connect(slider_pan, &QSlider::valueChanged, this, [this, i, slider_pan]{ MIDIChannelSelector::panSliderMoved(i-1, slider_pan->value()); });
        slider_pan->setObjectName("slider_pan");
        
        connect(slider_pan, &QSlider::sliderPressed, this, [slider_pan]{ slider_pan->setValue(64); });
        this->list_of_pan_sliders.append(slider_pan);
        
        MIDIKeyShiftWidget *key_shift = new MIDIKeyShiftWidget;
        QSpinBox *key_min = new QSpinBox();
        QSpinBox *key_max = new QSpinBox();
        //QSpinBox *key_shift = new QSpinBox();
        
        key_min->setMaximum(127);
        key_max->setMaximum(127);
        
        key_min->setValue(0);
        key_max->setValue(127);
        
        QComboBox *combo_instrument_group = new QComboBox;
        QList<QString> list_instrument_group = this->midi_sounds_list->getInstrumentGroups();
        list_instrument_group.append("[disabled]");
        combo_instrument_group->addItems(list_instrument_group);
        combo_instrument_group->setPlaceholderText("[non-default MSB/LSB set]");
        combo_instrument_group->setObjectName("instrument_selector");
        
        QComboBox *combo_instrument = new QComboBox;
        QList<QString> list_instrument = this->midi_sounds_list->getInstrumentsForGroupMIDIv1(list_instrument_group.at(0));
        combo_instrument->addItems(list_instrument);
        combo_instrument->setPlaceholderText("[non-default MSB/LSB set]");
        combo_instrument->setObjectName("instrument_selector");
        
        connect(combo_instrument_group, &QComboBox::currentTextChanged, this, [this, i, combo_instrument_group, combo_instrument]{ MIDIChannelSelector::instrumentGroupChanged(i-1, combo_instrument_group, combo_instrument); });
        //connect(combo_instrument_group, &QComboBox::currentTextChanged, this, [this, i, combo_instrument]{ MIDIChannelSelector::instrumentChanged(i-1, combo_instrument->currentText()); });
        
        connect(combo_instrument, &QComboBox::currentTextChanged, this, [this, i, combo_instrument]{ MIDIChannelSelector::instrumentChanged(i-1, combo_instrument->currentText()); });
        
        QSpinBox *midi_instrument_msb = new QSpinBox;
        QSpinBox *midi_instrument_lsb = new QSpinBox;
        midi_instrument_msb->setRange(-1, 127);
        midi_instrument_lsb->setRange(-1, 127);
        midi_instrument_msb->setValue(0);
        midi_instrument_lsb->setValue(0);
        midi_instrument_msb->setObjectName("instrument_selector");
        midi_instrument_lsb->setObjectName("instrument_selector");
        connect(
                    midi_instrument_msb,
                    &QSpinBox::textChanged,
                    this,
                    [this, i, midi_instrument_msb, midi_instrument_lsb]{
                        MIDIChannelSelector::instrumentChangedNumeric(
                                    i-1,
                                    midi_instrument_msb->value(),
                                    midi_instrument_lsb->value()
                        );
                    }
        );
        connect(
                    midi_instrument_lsb,
                    &QSpinBox::textChanged,
                    this,
                    [this, i, midi_instrument_msb, midi_instrument_lsb]{
                        MIDIChannelSelector::instrumentChangedNumeric(
                                    i-1,
                                    midi_instrument_msb->value(),
                                    midi_instrument_lsb->value()
                        );
                    }
        );
        
        QComboBox *combo_velocity = new QComboBox;
        combo_velocity->addItems(this->midi_sounds_list->getNuanceVelocities());
        combo_velocity->setCurrentIndex(5);
        combo_velocity->setObjectName("velocity_selector");
        connect(combo_velocity, &QComboBox::currentTextChanged, this, [this, i, combo_velocity]{ MIDIChannelSelector::velocityChanged(i-1, combo_velocity->currentText()); });
        this->list_of_velocities.append(combo_velocity);
        
        QSpinBox *spin_pitch = new QSpinBox;
        spin_pitch->setRange(-8192, 8191);
        spin_pitch->setValue(0);
        spin_pitch->setObjectName("pitch_selector");
        connect(spin_pitch, &QSpinBox::textChanged, this, [this, i, spin_pitch]{ MIDIChannelSelector::pitchChanged(i-1, spin_pitch->value()); });
        this->list_of_pitches.append(spin_pitch);
        
        QSlider *slider_portamento = new QSlider;
        slider_portamento->setOrientation(Qt::Horizontal);
        slider_portamento->setRange(0, 127);
        connect(slider_portamento, &QSlider::valueChanged, this, [this, i, slider_portamento]{ MIDIChannelSelector::portamentoChanged(i-1, slider_portamento->value()); });
        this->list_of_portamentos.append(slider_portamento);
        
        QSlider *slider_attack = new QSlider;
        slider_attack->setOrientation(Qt::Horizontal);
        slider_attack->setRange(0, 127);
        connect(slider_attack, &QSlider::valueChanged, this, [this, i, slider_attack]{ MIDIChannelSelector::attackChanged(i-1, slider_attack->value()); });
        this->list_of_attacks.append(slider_attack);
        
        QSlider *slider_release = new QSlider;
        slider_release->setOrientation(Qt::Horizontal);
        slider_release->setRange(0, 127);
        connect(slider_release, &QSlider::valueChanged, this, [this, i, slider_release]{ MIDIChannelSelector::releaseChanged(i-1, slider_release->value()); });
        this->list_of_releases.append(slider_release);
        
        QSlider *slider_tremolo = new QSlider;
        slider_tremolo->setOrientation(Qt::Horizontal);
        slider_tremolo->setRange(0, 127);
        connect(slider_tremolo, &QSlider::valueChanged, this, [this, i, slider_tremolo]{ MIDIChannelSelector::tremoloChanged(i-1, slider_tremolo->value()); });
        this->list_of_tremolos.append(slider_tremolo);
        
        
        // insert an empty row after each
        int row = 2 * i - 1; // odd rows: 1, 3, 5, ..., 31
        
        this->grid->addWidget(check_a, row, 0);
        this->grid->addWidget(button_cc_add, row, 1);
        this->grid->addWidget(slider_volume, row, 2);
        this->grid->addWidget(slider_pan, row, 3);
        this->grid->addWidget(key_shift, row, 4);
        
        this->grid->addWidget(key_min, row, 5);
        this->grid->addWidget(key_max, row, 6);
        
        this->grid->addWidget(combo_instrument_group, row, 7);
        this->grid->addWidget(combo_instrument, row, 8);
        this->grid->addWidget(midi_instrument_msb, row, 9);
        this->grid->addWidget(midi_instrument_lsb, row, 10);
        
        this->grid->addWidget(combo_velocity, row, 11);
        
        this->grid->addWidget(spin_pitch, row, 12);
        
        this->grid->addWidget(slider_portamento, row, 13);
        
        this->grid->addWidget(slider_attack, row, 14);
        this->grid->addWidget(slider_release, row, 15);
        
        this->grid->addWidget(slider_tremolo, row, 16);
        
        this->grid->addWidget(check_b, row, 17);
        
        if (i==1)
        {
            check_a->setChecked(true);
            check_b->setChecked(true);
        }
        
        this->list_of_checkboxes.append(check_a);
        this->list_of_keyshifts.append(key_shift);
        this->list_of_key_mins.append(key_min);
        this->list_of_key_maxs.append(key_max);
        this->list_of_instrument_groups.append(combo_instrument_group);
        this->list_of_instrument_banks.append(combo_instrument);
        this->list_of_msb.append(midi_instrument_msb);
        this->list_of_lsb.append(midi_instrument_lsb);
    }
}

QList<QMap<QString,QVariant>> MIDIChannelSelector::listOfChannels(bool only_activated)
{
    QList<QMap<QString,QVariant>> result;
    
    for (int channel=0; channel < this->list_of_checkboxes.length(); channel++)
    {
        QCheckBox *check = this->list_of_checkboxes.at(channel);
        if (!only_activated || check->isChecked())
        {
            QMap<QString,QVariant> map;
            
            map["channel"] = channel;
            
            map["activated"] = this->list_of_checkboxes.at(channel)->isChecked();
            
            int volume = this->list_of_volume_sliders.at(channel)->value();
            map["volume"] = volume;
            
            int pan = this->list_of_pan_sliders.at(channel)->value();
            map["pan"] = pan;
            
            int key_shift = this->list_of_keyshifts.at(channel)->value();
            map["key_shift"] = key_shift;
            
            int key_min = this->list_of_key_mins.at(channel)->value();
            map["key_min"] = key_min;
            
            int key_max = this->list_of_key_maxs.at(channel)->value();
            map["key_max"] = key_max;
            
            int instrument_msb = this->list_of_msb.at(channel)->value();
            map["instrument_msb"] = instrument_msb;
            
            int instrument_lsb = this->list_of_lsb.at(channel)->value();
            map["instrument_lsb"] = instrument_lsb;
            
            QString instrument_group = this->list_of_instrument_groups.at(channel)->currentText();
            map["instrument_bank"] = instrument_group;
            
            QString instrument_name = this->list_of_instrument_banks.at(channel)->currentText();
            map["instrument_name"] = instrument_name;
            
            QString str_velocity = this->list_of_velocities.at(channel)->currentText();
            int velocity = this->midi_sounds_list->getVelocityForString(str_velocity);
            map["velocity"] = velocity;
            
            int pitch = this->list_of_pitches.at(channel)->value();
            map["pitch"] = pitch;
            
            int portamento_time = this->list_of_portamentos.at(channel)->value();
            map["portamento_time"] = portamento_time;
            
            int attack = this->list_of_attacks.at(channel)->value();
            map["attack"] = attack;
            
            int release = this->list_of_releases.at(channel)->value();
            map["release"] = release;
            
            int tremolo = this->list_of_tremolos.at(channel)->value();
            map["tremolo"] = tremolo;
            
            for (int i = 0; i < list_of_cc_entries.size(); ++i)
            {
                CCEntry &entry = list_of_cc_entries[i];
                
                // match channel and row using spin_cc properties
                if (entry.channel == channel)
                {
                    QString istr = pad3(i);
                    
                    QString cc_i_active = "cc_" + istr + "_active";
                    map[cc_i_active] = entry.active->isChecked() ? "true" : "false";
                    
                    QString cc_i_label = "cc_" + istr + "_label";
                    map[cc_i_label] = entry.label->text();
                    
                    QString cc_i_key = "cc_" + istr + "_key";
                    map[cc_i_key] = QString::number(entry.key->value());
                    
                    QString cc_i_value = "cc_" + istr + "_value";
                    map[cc_i_value] = entry.value->value();
                }
            }
            
            result.append(map);
        }
    }
    
    return result;
}

void MIDIChannelSelector::restoreParams(QMap<QString,QVariant> data)
{
    for (int i=0; i < this->list_of_checkboxes.length(); i++)
    {
        QMap<QString,QVariant> channel = data[QString::number(i)].toMap();
        
        this->list_of_checkboxes.at(i)->setChecked(channel["activated"].toBool());
        
        this->list_of_volume_sliders.at(i)->setValue(channel["volume"].toInt());
        this->list_of_pan_sliders.at(i)->setValue(channel["pan"].toInt());
        this->list_of_keyshifts.at(i)->setValue(channel["key_shift"].toInt());
        this->list_of_key_mins.at(i)->setValue(channel["key_min"].toInt());
        this->list_of_key_maxs.at(i)->setValue(channel["key_max"].toInt());
        this->list_of_msb.at(i)->setValue(channel["instrument_msb"].toInt());
        this->list_of_lsb.at(i)->setValue(channel["instrument_lsb"].toInt());
        
        this->list_of_velocities.at(i)->setCurrentText(this->midi_sounds_list->getStringForVelocity(channel["velocity"].toInt()));
        this->list_of_pitches.at(i)->setValue(channel["pitch"].toInt());
        this->list_of_portamentos.at(i)->setValue(channel["portamento_time"].toInt());
        this->list_of_attacks.at(i)->setValue(channel["attack"].toInt());
        this->list_of_releases.at(i)->setValue(channel["release"].toInt());
        this->list_of_tremolos.at(i)->setValue(channel["tremolo"].toInt());
        
        QMap<QString,QVariant> cc_map;
        for (const auto &[key, value] : channel.toStdMap()) {
            if (key.startsWith("cc_")) {
                // This key corresponds to a CC entry
                
                int id = key.section('_', 1, 1).toInt();
                QString name = key.section('_', 2);
                
                if (!cc_map.contains("id"))
                {
                    // here we handle the first value of three
                    
                    cc_map["id"] = id;
                    cc_map[name] = value;
                    
                }
                else if (cc_map["id"] == id)
                {
                    // here we handle second and third value
                    
                    cc_map[name] = value;
                    
                    // check if we have all three, than load and reset cc_map
                    if (cc_map.contains("id") && cc_map.contains("key") && cc_map.contains("value"))
                    {
                        addNewCCEntry(i, cc_map);
                        
                        cc_map.clear();
                    }
                }
            }
        }
    }
}

void MIDIChannelSelector::addNewCCEntry(int channel, QMap<QString,QVariant> cc_map)
{
    int row = 2 * (channel+1); // even rows: 2, 4, 6, ..., 32
    
    if (QLayoutItem *item = grid->itemAtPosition(row, 2)) {
        // if we have the grid for the cc entries already
        if (QLayout *layout = item->layout()) {
            if (QGridLayout *grid = qobject_cast<QGridLayout*>(layout))
            {
                int new_row = grid->rowCount();
                
                addNewCCEntryRow(grid, channel, new_row, cc_map);
            }
        }
    }
    else
    {
        // we need to greate the grid and attach
        QGridLayout *grid = new QGridLayout;
        
        addNewCCEntryRow(grid, channel, 1, cc_map);
        
        this->grid->addLayout(grid, row, 2, 1, 12);
    }
}
void MIDIChannelSelector::addNewCCEntryRow(QGridLayout *grid, int channel, int row, QMap<QString,QVariant> cc_map)
{
    QPushButton *button_delete = new QPushButton("delete");
    button_delete->setObjectName("button_delete");
    connect(button_delete, &QPushButton::clicked, [this, grid, channel, row]{ delCCEntryRow(grid, channel, row); });
    
    QCheckBox *check_active = new QCheckBox;
    check_active->setToolTip("activate/deactivate this entry");
    if (cc_map.contains("active"))
    {
        if (cc_map["active"].toBool())
            check_active->setChecked(true);
        else
            check_active->setChecked(false);
    }
    else
        check_active->setChecked(true);
        
    
    QLineEdit *line_label = new QLineEdit;
    line_label->setMinimumWidth(300);
    line_label->setPlaceholderText("your label or description");
    if (cc_map.contains("label"))
        line_label->setText(cc_map["label"].toString());
    
    QSpinBox *spin_cc = new QSpinBox;
    spin_cc->setRange(0, 127);
    spin_cc->setToolTip("CC Key");
    if (cc_map.contains("key"))
        spin_cc->setValue(cc_map["key"].toInt());
    
    QSlider *slider_value = new QSlider(Qt::Horizontal);
    slider_value->setRange(0, 127);
    slider_value->setMinimumWidth(300);
    if (cc_map.contains("value"))
        slider_value->setValue(cc_map["value"].toInt());
    
    QSpinBox *spin_value = new QSpinBox;
    spin_value->setRange(0, 127);
    spin_value->setToolTip("CC Value");
    connect(spin_value, &QSpinBox::valueChanged, [this, channel, spin_cc, spin_value]{
        this->interface_audio->setControlChangeEvent(this->port, channel, spin_cc->value(), spin_value->value());
    });
    if (cc_map.contains("value"))
        spin_value->setValue(cc_map["value"].toInt());
    
    connect(slider_value, &QSlider::valueChanged, spin_value, &QSpinBox::setValue);
    connect(spin_value, &QSpinBox::valueChanged, slider_value, &QSlider::setValue);
    
    QLabel *label_delay = new QLabel("Delay to previous entry [ms]:");
    QSpinBox *spin_delay = new QSpinBox;
    spin_delay->setRange(0, 5000);
    spin_delay->setToolTip("Some MIDI Devices need a few milliseconds to process control messages. Here you can set up a timer to compensate.");
    spin_delay->setObjectName("velocity");
    
    grid->addWidget(button_delete, row, 1);
    grid->addWidget(check_active,  row, 2);
    grid->addWidget(line_label,    row, 3);
    grid->addWidget(spin_cc,       row, 4);
    grid->addWidget(slider_value,  row, 5);
    grid->addWidget(spin_value,    row, 10);
    grid->addWidget(label_delay,   row, 11);
    grid->addWidget(spin_delay,    row, 12);
    
    CCEntry entry;
    entry.active = check_active;
    entry.row = row;
    entry.channel = channel;
    entry.label = line_label;
    entry.key = spin_cc;
    entry.value = spin_value;
    entry.delay = spin_delay;
    this->list_of_cc_entries.append(entry);
}
void MIDIChannelSelector::delCCEntryRow(QGridLayout *grid, int channel, int row)
{
    if (!grid) return;
    
    // Show a confirmation dialog
    QMessageBox confirm;
    confirm.setWindowTitle("Confirm Delete");
    confirm.setText("Really delete this CC entry?");
    confirm.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirm.setDefaultButton(QMessageBox::No);
    
    if (confirm.exec() != QMessageBox::Yes)
    {
        // User canceled
        return;
    }
    
    // remove from list_of_cc_entries
    for (int i = 0; i < list_of_cc_entries.size(); ++i)
    {
        CCEntry &entry = list_of_cc_entries[i];
        
        // match channel and row using spin_cc properties
        if (entry.channel == channel &&
            entry.row == row)
        {
            list_of_cc_entries.removeAt(i);
            break;
        }
    }
    
    
    int cols = grid->columnCount();
    
    // Remove all widgets in the row
    for (int col = 0; col < cols; ++col)
    {
        if (QLayoutItem *item = grid->itemAtPosition(row, col))
        {
            if (QWidget *w = item->widget())
            {
                grid->removeWidget(w);
                delete w;
            }
            else if (QLayout *l = item->layout())
            {
                grid->removeItem(l);
                delete l;
            }
            else
            {
                grid->removeItem(item);
                delete item;
            }
        }
    }
}



InterfaceAudio* MIDIChannelSelector::selectedAudioInterface(int channel)
{
    //QComboBox *combo = this->list_of_midi_output_combos.at(channel);
    //return this->list_of_midi_outputs.at(combo->currentIndex());
    return this->interface_audio;
    
}

void MIDIChannelSelector::volumeSliderMoved(int channel, int volume)
{
    volume = volume * this->volume_dca / 100;
    if (volume > 127)
    {
        volume = 127;
    }
    
    InterfaceAudio *audio = selectedAudioInterface(channel);
    audio->setVolumeChangeEvent(this->port, channel, volume);
}
void MIDIChannelSelector::volumeDCAChanged(int value)
{
    this->volume_dca = value;
    
    QList<QMap<QString,QVariant>> channels = listOfChannels(false);
    for (int i=0; i < channels.length(); i++)
    {
        int channel = channels.at(i)["channel"].toInt();
        int volume = channels.at(i)["volume"].toInt();
        
        volumeSliderMoved(channel, volume);
    }
}

void MIDIChannelSelector::panSliderMoved(int channel, int value)
{
    InterfaceAudio *audio = selectedAudioInterface(channel);
    audio->setPanChangeEvent(this->port, channel, value);
}

void MIDIChannelSelector::instrumentGroupChanged(int channel, QComboBox *combo_group, QComboBox *combo_instrument)
{
    combo_instrument->blockSignals(true);
    combo_instrument->clear();
    
    QString group = combo_group->currentText();
    
    if (group == "[disabled]")
    {
        combo_instrument->setEnabled(false);
        list_of_msb.at(channel)->setEnabled(false);
        list_of_lsb.at(channel)->setEnabled(false);
        list_of_msb.at(channel)->setValue(-1);
        list_of_lsb.at(channel)->setValue(-1);
    }
    else
    {
        combo_instrument->setEnabled(true);
        list_of_msb.at(channel)->setEnabled(true);
        list_of_lsb.at(channel)->setEnabled(true);
        
        //QList<QString> instruments = this->midi_sounds_list->getInstrumentsForGroupMIDIv1(group);
        QList<QString> instruments = this->midi_sounds_list->getInstrumentsForGroupMIDIv2(group);
        combo_instrument->addItems(instruments);
        
        int midi = this->midi_sounds_list->getMIDICodeForBank(combo_group->currentText());
        list_of_msb.at(channel)->blockSignals(true);
        list_of_msb.at(channel)->setValue(midi);
        list_of_msb.at(channel)->blockSignals(false);
        
        list_of_lsb.at(channel)->blockSignals(true);
        list_of_lsb.at(channel)->setValue(0);
        list_of_lsb.at(channel)->blockSignals(false);
    }
    
    combo_instrument->blockSignals(false);
    
    //instrumentChanged(channel, combo_instrument->currentText());
}

void MIDIChannelSelector::instrumentChanged(int channel, QString instrument)
{
    //qDebug() << "changed-: "+instrument;
    
    QMap<QString,int> codes = this->midi_sounds_list->getMIDICodesForInstrument(instrument);
    int instrument_msb = codes["msb"] - 1;
    int instrument_lsb = codes["lsb"];
    
    this->list_of_msb.at(channel)->setValue(instrument_msb);
    this->list_of_lsb.at(channel)->setValue(instrument_lsb);
}

void MIDIChannelSelector::instrumentChangedNumeric(int channel, int instrument_msb, int instrument_lsb)
{
    InterfaceAudio *audio = selectedAudioInterface(channel);
    audio->setProgramChangeEvent(this->port, channel, instrument_msb, instrument_lsb);
    
    //qDebug() << "changed+: "+QString::number(channel)+" "+QString::number(instrument_msb)+" "+QString::number(instrument_lsb);
    audio->setProgramChangeEvent(this->port, channel, instrument_msb, instrument_lsb);
    
    // change spin boxes for group and bank accordingly
    QMap<QString,QString> names = this->midi_sounds_list->getInstrumentForMIDICodes(instrument_msb, instrument_lsb);
    //qDebug() << names;
    
    if (instrument_msb == -1 || instrument_lsb == -1)
    {
        this->list_of_instrument_groups.at(channel)->setCurrentText("[disabled]");
    }
    else
    {    
        this->list_of_instrument_banks.at(channel)->blockSignals(true);
        if (names["group"] == "")
        {
            this->list_of_instrument_groups.at(channel)->setCurrentIndex(-1);
        }
        else
        {
            this->list_of_instrument_groups.at(channel)->setCurrentText(names["group"]);
        }
        
        if (names["instrument"] == "")
        {
            this->list_of_instrument_banks.at(channel)->setCurrentIndex(-1);
        }
        else
        {
            this->list_of_instrument_banks.at(channel)->setCurrentText(names["instrument"]);
        }
        this->list_of_instrument_banks.at(channel)->blockSignals(false);
    }
}

void MIDIChannelSelector::velocityChanged(int channel, QString value)
{
    this->midi_sounds_list->getVelocityForString(value);
}

void MIDIChannelSelector::pitchChanged(int channel, int value)
{
    InterfaceAudio *audio = selectedAudioInterface(channel);
    value = value + 8192;
    audio->keyPitchbendEvent(this->port, channel, value);
}

void MIDIChannelSelector::portamentoChanged(int channel, int value)
{
    InterfaceAudio *audio = selectedAudioInterface(channel);
    audio->setPortamentoChanged(this->port, channel, value);
}

void MIDIChannelSelector::attackChanged(int channel, int value)
{
    InterfaceAudio *audio = selectedAudioInterface(channel);
    audio->setAttackChanged(this->port, channel, value);
}

void MIDIChannelSelector::releaseChanged(int channel, int value)
{
    InterfaceAudio *audio = selectedAudioInterface(channel);
    audio->setReleaseChanged(this->port, channel, value);
}

void MIDIChannelSelector::tremoloChanged(int channel, int value)
{
    InterfaceAudio *audio = selectedAudioInterface(channel);
    audio->setTremoloChanged(this->port, channel, value);
}

void MIDIChannelSelector::resendMIDIControls()
{
    // Get all activated channels
    QList<QMap<QString,QVariant>> channels = listOfChannels(true);
    
    for (const auto &channelData : channels)
    {
        int channel = channelData["channel"].toInt();
        InterfaceAudio *audio = selectedAudioInterface(channel);
        
        // 1️⃣ Send Program Change first
        if (channelData["instrument_bank"] != "[disabled]")
        {
            audio->setProgramChangeEvent(
                this->port,
                channel,
                channelData["instrument_msb"].toInt(),
                channelData["instrument_lsb"].toInt()
                );
        }
        // 2️⃣ Send sliders and other parameters AFTER Program Change
        volumeSliderMoved(channel, channelData["volume"].toInt());
        panSliderMoved(channel, channelData["pan"].toInt());
        portamentoChanged(channel, channelData["portamento_time"].toInt());
        attackChanged(channel, channelData["attack"].toInt());
        releaseChanged(channel, channelData["release"].toInt());
        tremoloChanged(channel, channelData["tremolo"].toInt());
        pitchChanged(channel, channelData["pitch"].toInt());
    }
    
    // 3️⃣ Send all active CC entries for this channel, respecting delays
    int cumulativeDelay = 0; // ms
    
    for (const CCEntry &entry : list_of_cc_entries)
    {
        if (!entry.active->isChecked())
            continue;
        
        cumulativeDelay += entry.delay->value(); // sum delays
        
        // Capture variables by value for the lambda
        int port = this->port;
        int channel = entry.channel;
        int key = entry.key->value();
        int value = entry.value->value();
        
        QTimer::singleShot(cumulativeDelay, this, [this, port, channel, key, value]() {
            this->interface_audio->setControlChangeEvent(port, channel, key, value);
        });
    }
    
}


bool MIDIChannelSelector::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress)//|| ev->type() == QEvent::KeyRelease)
    {
        QKeyEvent *event = static_cast<QKeyEvent*>(ev);
        
        if (event->isAutoRepeat() == false)
        {
            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            {
                setFocus();
                
                return false;
            }
            else if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9)
            {
                int channel_number = event->key() - Qt::Key_0;
                
                if (event->modifiers() & Qt::AltModifier)
                {
                    channel_number += 10;
                }
                else if (event->key() == Qt::Key_0)
                {
                    channel_number = 10;
                }
                
                if (channel_number <= this->list_of_checkboxes.length())
                {
                    this->list_of_checkboxes.at(channel_number-1)->toggle();
                }
                
                return true;
            }
            else if (event->key() == Qt::Key_Escape)
            {
                //this->hide();
                emit closed();
                
                return true;
            }
            
            emit eventFiltered(obj, ev);
            return true;
        }
    }
    else if (ev->type() == QEvent::KeyRelease)
    {
        QKeyEvent *event = static_cast<QKeyEvent*>(ev);
        
        if (event->isAutoRepeat() == false)
        {
            emit eventFiltered(obj, ev);
            return true;
        }
    }
    
    return false;
}

void MIDIChannelSelector::hideEvent(QHideEvent *ev)
{
    emit closed();
}

void MIDIChannelSelector::checkToggled(QCheckBox *check_master, QCheckBox *check_slave)
{
    if (check_master->isChecked())
    {
        check_slave->setChecked(true);
    }
    else
    {
        check_slave->setChecked(false);
    }
}

QString MIDIChannelSelector::pad3(int n)
{
    return QString("%1").arg(n, 3, 10, QLatin1Char('0'));
}
