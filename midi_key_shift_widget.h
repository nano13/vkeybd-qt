#ifndef MIDIKEYSHIFTWIDGET_H
#define MIDIKEYSHIFTWIDGET_H

#include <QObject>
#include <QWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QHBoxLayout>

class MIDIKeyShiftWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIKeyShiftWidget(QWidget *parent = nullptr);
    
    int value();
    
private:
    QSpinBox *spin_key;
    
signals:
    
private slots:
    void lowerShiftKeyPressed();
    void higherShiftKeyPressed();
};

#endif // MIDIKEYSHIFTWIDGET_H
