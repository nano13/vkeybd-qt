#include "midi_channel_selector.h"

#include <QSignalBlocker>

MIDIChannelSelector::MIDIChannelSelector(MidiEngine *engine, QWidget *parent) : QWidget(parent)
{
    this->engine = engine;

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
    this->grid->setSizeConstraint( QLayout::SetFixedSize );

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

    QList<QString> list_instrument_group = this->midi_sounds_list->getInstrumentGroups();
    list_instrument_group.append("[disabled]");

    for (int i=1; i <= 16; i++)
    {
        const int channel = i - 1;

        QCheckBox *check_a = new QCheckBox(QString::number(i));
        QCheckBox *check_b = new QCheckBox(QString::number(i));
        connect(check_a, &QCheckBox::toggled, this, [this, check_a, check_b]{ checkToggled(check_a, check_b); });
        connect(check_b, &QCheckBox::toggled, this, [this, check_b, check_a]{ checkToggled(check_b, check_a); });

        QPushButton *button_cc_add = new QPushButton("add CC");
        connect(button_cc_add, &QPushButton::clicked, this, [this, channel]{ addNewCCEntry(channel, QMap<QString,QVariant>()); });

        QSlider *slider_volume = new QSlider;
        slider_volume->setOrientation(Qt::Horizontal);
        slider_volume->setRange(0, 127);
        slider_volume->setValue(127);

        QSlider *slider_pan = new QSlider;
        slider_pan->setOrientation(Qt::Horizontal);
        slider_pan->setRange(0, 127);
        slider_pan->setValue(64);
        slider_pan->setObjectName("slider_pan");
        connect(slider_pan, &QSlider::sliderPressed, this, [slider_pan]{ slider_pan->setValue(64); });

        MIDIKeyShiftWidget *key_shift = new MIDIKeyShiftWidget;
        QSpinBox *key_min = new QSpinBox();
        QSpinBox *key_max = new QSpinBox();
        key_min->setMaximum(127);
        key_max->setMaximum(127);
        key_min->setValue(0);
        key_max->setValue(127);

        QComboBox *combo_instrument_group = new QComboBox;
        combo_instrument_group->addItems(list_instrument_group);
        combo_instrument_group->setPlaceholderText("[non-default MSB/LSB set]");
        combo_instrument_group->setObjectName("instrument_selector");

        QComboBox *combo_instrument = new QComboBox;
        combo_instrument->setPlaceholderText("[non-default MSB/LSB set]");
        combo_instrument->setObjectName("instrument_selector");

        QSpinBox *midi_instrument_msb = new QSpinBox;
        QSpinBox *midi_instrument_lsb = new QSpinBox;
        midi_instrument_msb->setRange(-1, 127);
        midi_instrument_lsb->setRange(-1, 127);
        midi_instrument_msb->setValue(0);
        midi_instrument_lsb->setValue(0);
        midi_instrument_msb->setObjectName("instrument_selector");
        midi_instrument_lsb->setObjectName("instrument_selector");

        QComboBox *combo_velocity = new QComboBox;
        combo_velocity->addItems(this->midi_sounds_list->getNuanceVelocities());
        combo_velocity->setCurrentIndex(5);
        combo_velocity->setObjectName("velocity_selector");

        QSpinBox *spin_pitch = new QSpinBox;
        spin_pitch->setRange(-8192, 8191);
        spin_pitch->setValue(0);
        spin_pitch->setObjectName("pitch_selector");

        QSlider *slider_portamento = new QSlider;
        slider_portamento->setOrientation(Qt::Horizontal);
        slider_portamento->setRange(0, 127);

        QSlider *slider_attack = new QSlider;
        slider_attack->setOrientation(Qt::Horizontal);
        slider_attack->setRange(0, 127);

        QSlider *slider_release = new QSlider;
        slider_release->setOrientation(Qt::Horizontal);
        slider_release->setRange(0, 127);

        QSlider *slider_tremolo = new QSlider;
        slider_tremolo->setOrientation(Qt::Horizontal);
        slider_tremolo->setRange(0, 127);

        // instrument selection keeps the four widgets consistent and then writes back
        connect(combo_instrument_group, &QComboBox::activated, this, [this, channel]{ instrumentGroupChanged(channel); });
        connect(combo_instrument, &QComboBox::activated, this, [this, channel, combo_instrument]{ instrumentChanged(channel, combo_instrument->currentText()); });
        connect(midi_instrument_msb, &QSpinBox::valueChanged, this, [this, channel]{ instrumentChangedNumeric(channel); });
        connect(midi_instrument_lsb, &QSpinBox::valueChanged, this, [this, channel]{ instrumentChangedNumeric(channel); });

        // everything else: write the channel back to the engine, which sends what changed
        connect(check_a, &QCheckBox::toggled, this, [this, channel]{ pushChannel(channel); });
        connect(slider_volume, &QSlider::valueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(slider_pan, &QSlider::valueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(key_shift, &MIDIKeyShiftWidget::signalValueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(key_min, &QSpinBox::valueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(key_max, &QSpinBox::valueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(combo_velocity, &QComboBox::currentTextChanged, this, [this, channel]{ pushChannel(channel); });
        connect(spin_pitch, &QSpinBox::valueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(slider_portamento, &QSlider::valueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(slider_attack, &QSlider::valueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(slider_release, &QSlider::valueChanged, this, [this, channel]{ pushChannel(channel); });
        connect(slider_tremolo, &QSlider::valueChanged, this, [this, channel]{ pushChannel(channel); });

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

        this->list_of_checkboxes.append(check_a);
        this->list_of_checkboxes_b.append(check_b);
        this->list_of_volume_sliders.append(slider_volume);
        this->list_of_pan_sliders.append(slider_pan);
        this->list_of_keyshifts.append(key_shift);
        this->list_of_key_mins.append(key_min);
        this->list_of_key_maxs.append(key_max);
        this->list_of_instrument_groups.append(combo_instrument_group);
        this->list_of_instrument_banks.append(combo_instrument);
        this->list_of_msb.append(midi_instrument_msb);
        this->list_of_lsb.append(midi_instrument_lsb);
        this->list_of_velocities.append(combo_velocity);
        this->list_of_pitches.append(spin_pitch);
        this->list_of_portamentos.append(slider_portamento);
        this->list_of_attacks.append(slider_attack);
        this->list_of_releases.append(slider_release);
        this->list_of_tremolos.append(slider_tremolo);
    }
}

// ---------------------------------------------------------------------------
// binding

void MIDIChannelSelector::bindTab(int tab)
{
    this->loading = true;
    this->port = tab;

    TabPreset preset = this->engine->preset(tab);

    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        loadChannel(c, preset.channels.at(c));
    }

    clearCCEntries();
    for (const CCSetting &cc : preset.cc_entries)
    {
        QMap<QString, QVariant> cc_map;
        cc_map["active"] = cc.active;
        cc_map["label"] = cc.label;
        cc_map["key"] = cc.cc;
        cc_map["value"] = cc.value;
        cc_map["delay"] = cc.delay_ms;
        addNewCCEntry(cc.channel, cc_map);
    }

    this->loading = false;
}

int MIDIChannelSelector::boundTab() const
{
    return this->port;
}

void MIDIChannelSelector::loadChannel(int channel, const ChannelSettings &settings)
{
    // this->loading is set: the change handlers neither write back nor send anything
    this->list_of_checkboxes.at(channel)->setChecked(settings.activated);
    this->list_of_checkboxes_b.at(channel)->setChecked(settings.activated);
    this->list_of_volume_sliders.at(channel)->setValue(settings.volume);
    this->list_of_pan_sliders.at(channel)->setValue(settings.pan);
    this->list_of_keyshifts.at(channel)->setValue(settings.key_shift);
    this->list_of_key_mins.at(channel)->setValue(settings.key_min);
    this->list_of_key_maxs.at(channel)->setValue(settings.key_max);
    applyInstrumentToWidgets(channel, settings.instrument_msb, settings.instrument_lsb);
    this->list_of_velocities.at(channel)->setCurrentText(this->midi_sounds_list->getStringForVelocity(settings.velocity));
    this->list_of_pitches.at(channel)->setValue(settings.pitch);
    this->list_of_portamentos.at(channel)->setValue(settings.portamento_time);
    this->list_of_attacks.at(channel)->setValue(settings.attack);
    this->list_of_releases.at(channel)->setValue(settings.release);
    this->list_of_tremolos.at(channel)->setValue(settings.tremolo);
}

ChannelSettings MIDIChannelSelector::channelFromWidgets(int channel) const
{
    ChannelSettings settings;

    settings.activated = this->list_of_checkboxes.at(channel)->isChecked();
    settings.volume = this->list_of_volume_sliders.at(channel)->value();
    settings.pan = this->list_of_pan_sliders.at(channel)->value();
    settings.key_shift = this->list_of_keyshifts.at(channel)->value();
    settings.key_min = this->list_of_key_mins.at(channel)->value();
    settings.key_max = this->list_of_key_maxs.at(channel)->value();
    settings.instrument_msb = this->list_of_msb.at(channel)->value();
    settings.instrument_lsb = this->list_of_lsb.at(channel)->value();
    settings.instrument_bank = this->list_of_instrument_groups.at(channel)->currentText();
    settings.instrument_name = this->list_of_instrument_banks.at(channel)->currentText();
    settings.velocity = this->midi_sounds_list->getVelocityForString(this->list_of_velocities.at(channel)->currentText());
    settings.pitch = this->list_of_pitches.at(channel)->value();
    settings.portamento_time = this->list_of_portamentos.at(channel)->value();
    settings.attack = this->list_of_attacks.at(channel)->value();
    settings.release = this->list_of_releases.at(channel)->value();
    settings.tremolo = this->list_of_tremolos.at(channel)->value();

    return settings;
}

void MIDIChannelSelector::pushChannel(int channel)
{
    if (this->loading || this->port < 0)
        return;

    this->engine->setChannel(this->port, channel, channelFromWidgets(channel));
}

void MIDIChannelSelector::pushCCEntries()
{
    if (this->loading || this->port < 0)
        return;

    this->engine->setCCEntries(this->port, ccEntriesFromWidgets());
}

// ---------------------------------------------------------------------------
// instruments

void MIDIChannelSelector::fillInstrumentList(int channel, const QString &group)
{
    QComboBox *combo_instrument = this->list_of_instrument_banks.at(channel);

    const QSignalBlocker blocker(combo_instrument);
    combo_instrument->clear();
    combo_instrument->addItems(this->midi_sounds_list->getInstrumentsForGroupMIDIv2(group));
}

void MIDIChannelSelector::applyInstrumentToWidgets(int channel, int instrument_msb, int instrument_lsb)
{
    // shows the given MSB/LSB in all four instrument widgets, without any feedback between them
    QComboBox *combo_group = this->list_of_instrument_groups.at(channel);
    QComboBox *combo_instrument = this->list_of_instrument_banks.at(channel);
    QSpinBox *spin_msb = this->list_of_msb.at(channel);
    QSpinBox *spin_lsb = this->list_of_lsb.at(channel);

    const QSignalBlocker block_group(combo_group);
    const QSignalBlocker block_instrument(combo_instrument);
    const QSignalBlocker block_msb(spin_msb);
    const QSignalBlocker block_lsb(spin_lsb);

    spin_msb->setValue(instrument_msb);
    spin_lsb->setValue(instrument_lsb);

    const bool disabled = (instrument_msb < 0 || instrument_lsb < 0);
    if (disabled)
    {
        combo_group->setCurrentText("[disabled]");
        combo_instrument->clear();
    }
    else
    {
        QMap<QString,QString> names = this->midi_sounds_list->getInstrumentForMIDICodes(instrument_msb, instrument_lsb);

        if (names["group"].isEmpty())
        {
            // non-default MSB/LSB: the placeholder texts are shown
            combo_group->setCurrentIndex(-1);
            combo_instrument->clear();
        }
        else
        {
            combo_group->setCurrentText(names["group"]);
            fillInstrumentList(channel, names["group"]);

            if (names["instrument"].isEmpty())
                combo_instrument->setCurrentIndex(-1);
            else
                combo_instrument->setCurrentText(names["instrument"]);
        }
    }

    combo_instrument->setEnabled(!disabled);
    spin_msb->setEnabled(!disabled);
    spin_lsb->setEnabled(!disabled);
}

void MIDIChannelSelector::instrumentGroupChanged(int channel)
{
    QString group = this->list_of_instrument_groups.at(channel)->currentText();

    if (group == "[disabled]")
    {
        applyInstrumentToWidgets(channel, -1, -1);
        pushChannel(channel);
        return;
    }

    // select (and send) the first instrument of the new group
    fillInstrumentList(channel, group);
    QComboBox *combo_instrument = this->list_of_instrument_banks.at(channel);
    if (combo_instrument->count() > 0)
    {
        instrumentChanged(channel, combo_instrument->itemText(0));
    }
}

void MIDIChannelSelector::instrumentChanged(int channel, QString instrument)
{
    QString group = this->list_of_instrument_groups.at(channel)->currentText();

    QMap<QString,int> codes = this->midi_sounds_list->getMIDICodesForInstrument(group, instrument);
    if (!codes.contains("msb"))
        return;

    // the list counts programs from 1, MIDI from 0
    applyInstrumentToWidgets(channel, codes["msb"] - 1, codes["lsb"]);
    pushChannel(channel);
}

void MIDIChannelSelector::instrumentChangedNumeric(int channel)
{
    int instrument_msb = this->list_of_msb.at(channel)->value();
    int instrument_lsb = this->list_of_lsb.at(channel)->value();

    applyInstrumentToWidgets(channel, instrument_msb, instrument_lsb);
    pushChannel(channel);
}

// ---------------------------------------------------------------------------
// CC entries

void MIDIChannelSelector::addNewCCEntry(int channel, QMap<QString,QVariant> cc_map)
{
    int row = 2 * (channel+1); // even rows: 2, 4, 6, ..., 32

    if (QLayoutItem *item = this->grid->itemAtPosition(row, 2))
    {
        // if we have the grid for the cc entries already
        if (QLayout *layout = item->layout())
        {
            if (QGridLayout *cc_grid = qobject_cast<QGridLayout*>(layout))
            {
                int new_row = cc_grid->rowCount();

                addNewCCEntryRow(cc_grid, channel, new_row, cc_map);
            }
        }
    }
    else
    {
        // we need to create the grid and attach
        QGridLayout *cc_grid = new QGridLayout;

        addNewCCEntryRow(cc_grid, channel, 1, cc_map);

        this->grid->addLayout(cc_grid, row, 2, 1, 12);
    }
}
void MIDIChannelSelector::addNewCCEntryRow(QGridLayout *grid, int channel, int row, QMap<QString,QVariant> cc_map)
{
    QPushButton *button_delete = new QPushButton("delete");
    button_delete->setObjectName("button_delete");
    connect(button_delete, &QPushButton::clicked, this, [this, grid, channel, row]{ delCCEntryRow(grid, channel, row); });

    QCheckBox *check_active = new QCheckBox;
    check_active->setToolTip("activate/deactivate this entry");
    if (cc_map.contains("active"))
        check_active->setChecked(cc_map["active"].toBool());
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
    if (cc_map.contains("value"))
        spin_value->setValue(cc_map["value"].toInt());

    QLabel *label_delay = new QLabel("Delay to previous entry [ms]:");
    QSpinBox *spin_delay = new QSpinBox;
    spin_delay->setRange(0, 5000);
    spin_delay->setToolTip("Some MIDI Devices need a few milliseconds to process control messages. Here you can set up a timer to compensate.");
    spin_delay->setObjectName("velocity");
    if (cc_map.contains("delay"))
        spin_delay->setValue(cc_map["delay"].toInt());

    // connected after the initial values are set
    connect(slider_value, &QSlider::valueChanged, spin_value, &QSpinBox::setValue);
    connect(spin_value, &QSpinBox::valueChanged, slider_value, &QSlider::setValue);
    connect(spin_value, &QSpinBox::valueChanged, this, [this, channel, spin_cc, spin_value]{
        if (this->loading || this->port < 0)
            return;
        this->engine->sendControlChange(this->port, channel, spin_cc->value(), spin_value->value());
    });

    connect(check_active, &QCheckBox::toggled, this, &MIDIChannelSelector::pushCCEntries);
    connect(line_label, &QLineEdit::textChanged, this, &MIDIChannelSelector::pushCCEntries);
    connect(spin_cc, &QSpinBox::valueChanged, this, &MIDIChannelSelector::pushCCEntries);
    connect(spin_value, &QSpinBox::valueChanged, this, &MIDIChannelSelector::pushCCEntries);
    connect(spin_delay, &QSpinBox::valueChanged, this, &MIDIChannelSelector::pushCCEntries);

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

    pushCCEntries();
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
    for (int i = 0; i < this->list_of_cc_entries.size(); ++i)
    {
        const CCEntry &entry = this->list_of_cc_entries.at(i);

        if (entry.channel == channel && entry.row == row)
        {
            this->list_of_cc_entries.removeAt(i);
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

    pushCCEntries();
}

void MIDIChannelSelector::clearCCEntries()
{
    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        QLayoutItem *item = this->grid->itemAtPosition(2 * (c+1), 2);
        if (!item || !item->layout())
            continue;

        QLayout *cc_layout = item->layout();
        QLayoutItem *child = nullptr;
        while ((child = cc_layout->takeAt(0)) != nullptr)
        {
            delete child->widget();
            delete child;
        }

        this->grid->removeItem(cc_layout);
        delete cc_layout;
    }

    this->list_of_cc_entries.clear();
}

QList<CCSetting> MIDIChannelSelector::ccEntriesFromWidgets() const
{
    QList<CCSetting> result;

    for (const CCEntry &entry : this->list_of_cc_entries)
    {
        CCSetting cc;
        cc.channel = entry.channel;
        cc.active = entry.active->isChecked();
        cc.label = entry.label->text();
        cc.cc = entry.key->value();
        cc.value = entry.value->value();
        cc.delay_ms = entry.delay->value();
        result.append(cc);
    }

    return result;
}

// ---------------------------------------------------------------------------

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

                if (channel_number >= 1 && channel_number <= this->list_of_checkboxes.length())
                {
                    this->list_of_checkboxes.at(channel_number-1)->toggle();
                }

                return true;
            }
            else if (event->key() == Qt::Key_Escape)
            {
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
    QWidget::hideEvent(ev);

    emit closed();
}

void MIDIChannelSelector::checkToggled(QCheckBox *check_master, QCheckBox *check_slave)
{
    check_slave->setChecked(check_master->isChecked());
}
