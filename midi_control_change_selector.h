#ifndef MIDI_CONTROL_CHANGE_SELECTOR_H
#define MIDI_CONTROL_CHANGE_SELECTOR_H

#include <QWidget>
#include <QVector>

class QTabWidget;
class QGridLayout;
class QSpinBox;
class QPushButton;

class MIDICCTableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MIDICCTableWidget(QWidget *parent = nullptr);
    
private:
    // ------------------ State ------------------
    QGridLayout *grid;
    int rowCount = 0;
    int colCount = 0;
    
    QVector<QSpinBox*> rowHeaderSpinBoxes;
    QVector<QSpinBox*> colHeaderSpinBoxes;
    QVector<QPushButton*> rowDeleteButtons;
    QVector<QPushButton*> colDeleteButtons;
    
    // ------------------ Functions ------------------
    QWidget* createCellWidget();
    void addRow();
    void addColumn();
    void deleteRow(int row);
    void deleteColumn(int col);
};

class MIDIControlChangeSelector : public QWidget
{
    Q_OBJECT
public:
    explicit MIDIControlChangeSelector(QWidget *parent = nullptr);
    
private:
    QTabWidget *tabWidget;
};

#endif // MIDI_CONTROL_CHANGE_SELECTOR_H
