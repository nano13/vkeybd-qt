#ifndef MIDICHANNELSELECTOR_H
#define MIDICHANNELSELECTOR_H

#include <QObject>
#include <QWidget>

#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>

#include <QDebug>

class MIDIKeyShiftWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIKeyShiftWidget(QWidget *parent = nullptr);
    
    QSpinBox *spin_key;
    
private:
    
signals:
    
private slots:
    void lowerShiftKeyPressed();
    void higherShiftKeyPressed();
};




#include <QObject>
#include <QWidget>

#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QDial>
#include <QComboBox>
#include <QLabel>
#include <QEvent>
#include <QKeyEvent>

#include "midi_sounds_list.h"
#include "interface_audio.h"

class MIDIChannelSelector : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIChannelSelector(InterfaceAudio *audio, QWidget *parent = nullptr);
    
    QList<QMap<QString, int> > getListOfActivatedChannels();
    void volumeDCAChanged(int value);
    void resendMIDIControls();
    
private:
    MIDISoundsList *midi_sounds_list = new MIDISoundsList;
    InterfaceAudio *audio;
    
    int volume_dca = 100;
    
    QList<QCheckBox*> list_of_checkboxes;
    QList<MIDIKeyShiftWidget*> list_of_keyshifts;
    QList<QSpinBox*> list_of_key_mins;
    QList<QSpinBox*> list_of_key_maxs;
    QList<QSlider*> list_of_volume_sliders;
    QList<QComboBox*> list_of_instrument_groups;
    QList<QComboBox*> list_of_instrument_banks;
    QList<QSpinBox*> list_of_msb;
    QList<QSpinBox*> list_of_lsb;
    
protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    
private slots:
    void volumeSliderMoved(int channel, int volume);
    void panSliderMoved(int channel, int value);
    void instrumentGroupChanged(int channel, QComboBox *combo_group, QComboBox *combo_instrument);
    void instrumentChanged(int channel, QComboBox *combo_instrument);
    void instrumentChangedNumeric(int channel, int program, int bank);
    void portamentoChanged(int channel, QSlider *slider);
    void attackChanged(int channel, QSlider *slider);
    void releaseChanged(int channel, QSlider *slider);
    
signals:
    void eventFiltered(QObject *obj, QEvent *ev);
};

#endif // MIDICHANNELSELECTOR_H
