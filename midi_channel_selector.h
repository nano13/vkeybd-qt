#ifndef MIDICHANNELSELECTOR_H
#define MIDICHANNELSELECTOR_H

#include <QObject>
#include <QWidget>

#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>

#include "midi_sounds_list.h"

class MIDIChannelSelector : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIChannelSelector(QWidget *parent = nullptr);
    
private:
    QList<QCheckBox*> list_of_checkboxes;
    
signals:
    
};

#endif // MIDICHANNELSELECTOR_H
