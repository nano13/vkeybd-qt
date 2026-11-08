#ifndef MIDICHANNELSELECTOR_H
#define MIDICHANNELSELECTOR_H

#include <QObject>
#include <QWidget>

#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QVariant>
#include <QFile>

#include <QHideEvent>

#include "midi_sounds_list.h"
#include "midi_key_shift_widget.h"
#include "midi_engine.h"
#include "preset_model.h"
#include "enums_structs.h"

/*
 * Channel editor. One instance per keyboard, created on first use.
 *
 * bindTab() loads a tab's preset from the MidiEngine into the widgets (without
 * sending anything). Every edit is written back to the engine, which sends the
 * MIDI messages that are affected by the change.
 */
class MIDIChannelSelector : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIChannelSelector(MidiEngine *engine, QWidget *parent = nullptr);

    void bindTab(int tab);
    int boundTab() const;

private:
    QGridLayout *grid = new QGridLayout;

    MIDISoundsList *midi_sounds_list = new MIDISoundsList;
    MidiEngine *engine;

    // the tab (= MIDI port) the widgets currently show, -1 if none
    int port = -1;
    // true while bindTab() fills the widgets: nothing is written back or sent
    bool loading = false;

    void drawGUI();

    void loadChannel(int channel, const ChannelSettings &settings);
    ChannelSettings channelFromWidgets(int channel) const;
    void applyInstrumentToWidgets(int channel, int instrument_msb, int instrument_lsb);
    void fillInstrumentList(int channel, const QString &group);

    void clearCCEntries();
    QList<CCSetting> ccEntriesFromWidgets() const;

    void pushChannel(int channel);
    void pushCCEntries();

    QList<QCheckBox*> list_of_checkboxes;
    QList<QCheckBox*> list_of_checkboxes_b;
    QList<MIDIKeyShiftWidget*> list_of_keyshifts;
    QList<QSpinBox*> list_of_key_mins;
    QList<QSpinBox*> list_of_key_maxs;
    QList<QSlider*> list_of_volume_sliders;
    QList<QSlider*> list_of_pan_sliders;
    QList<QComboBox*> list_of_instrument_groups;
    QList<QComboBox*> list_of_instrument_banks;
    QList<QSpinBox*> list_of_msb;
    QList<QSpinBox*> list_of_lsb;
    QList<QComboBox*> list_of_velocities;
    QList<QSpinBox*> list_of_pitches;
    QList<QSlider*> list_of_portamentos;
    QList<QSlider*> list_of_attacks;
    QList<QSlider*> list_of_releases;
    QList<QSlider*> list_of_tremolos;

    QList<CCEntry> list_of_cc_entries;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;

private slots:
    void checkToggled(QCheckBox *check_master, QCheckBox *check_slave);
    void addNewCCEntry(int channel, QMap<QString, QVariant> cc_map);
    void addNewCCEntryRow(QGridLayout *grid, int channel, int row, QMap<QString, QVariant> cc_map);
    void delCCEntryRow(QGridLayout *grid, int channel, int row);

    void instrumentGroupChanged(int channel);
    void instrumentChanged(int channel, QString instrument);
    void instrumentChangedNumeric(int channel);

signals:
    void eventFiltered(QObject *obj, QEvent *ev);
    void closed();
};

#endif // MIDICHANNELSELECTOR_H
