#ifndef MIDICHANNELSELECTOR_H
#define MIDICHANNELSELECTOR_H

#include <QObject>
#include <QWidget>

#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QDial>
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
#include "interface_audio.h"
#include "midi_key_shift_widget.h"
#include "enums_structs.h"

class MIDIChannelSelector : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIChannelSelector(InterfaceAudio *interface_audio, int port, QWidget *parent = nullptr);
    
    QList<QMap<QString, QVariant> > listOfChannels(bool only_activated=true);
    void restoreParams(QMap<QString,QVariant> data);
    void volumeDCAChanged(int value);
    void resendMIDIControls();
    
private:
    QGridLayout *grid = new QGridLayout;
    
    MIDISoundsList *midi_sounds_list = new MIDISoundsList;
    int port;
    InterfaceAudio *interface_audio;
    
    int volume_dca = 100;
    
    void drawGUI();
    
    QString ADD_NEW_AUDIO_OUTPUT_LABEL = "[add new interface]";
    
    InterfaceAudio* selectedAudioInterface(int channel);
    
    QList<QCheckBox*> list_of_checkboxes;
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
    bool eventFilter(QObject *obj, QEvent *ev);
    void hideEvent(QHideEvent *ev);
    
private slots:
    void checkToggled(QCheckBox *check_master, QCheckBox *check_slave);
    void addNewCCEntry(int channel);
    void addNewCCEntryRow(QGridLayout *grid, int channel, int row);
    void delCCEntryRow(QGridLayout *grid, int channel, int row);
    void volumeSliderMoved(int channel, int volume);
    void panSliderMoved(int channel, int value);
    void instrumentGroupChanged(int channel, QComboBox *combo_group, QComboBox *combo_instrument);
    void instrumentChanged(int channel, QString instrument);
    void instrumentChangedNumeric(int channel, int instrument_msb, int instrument_lsb);
    void velocityChanged(int channel, QString value);
    void pitchChanged(int channel, int value);
    void portamentoChanged(int channel, int value);
    void attackChanged(int channel, int value);
    void releaseChanged(int channel, int value);
    void tremoloChanged(int channel, int value);
    
signals:
    void eventFiltered(QObject *obj, QEvent *ev);
    void closed();
};

#endif // MIDICHANNELSELECTOR_H
