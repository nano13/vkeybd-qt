#include "midi_control_change_selector.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <algorithm>

// ------------------- MIDICCTableWidget -------------------
MIDICCTableWidget::MIDICCTableWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(1000, 500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2,2,2,2);
    
    // Scrollable grid
    QWidget *gridWidget = new QWidget;
    grid = new QGridLayout(gridWidget);
    grid->setSpacing(8);
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(4);
    
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(gridWidget);
    mainLayout->addWidget(scroll);
    
    // Buttons to add rows/columns
    QPushButton *addRowBtn = new QPushButton("Add CC");
    QPushButton *addColBtn = new QPushButton("Add Channel");
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addRowBtn);
    buttonLayout->addWidget(addColBtn);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(addRowBtn, &QPushButton::clicked, this, &MIDICCTableWidget::addRow);
    connect(addColBtn, &QPushButton::clicked, this, &MIDICCTableWidget::addColumn);
}

// ------------------- Cell Creation -------------------
QWidget* MIDICCTableWidget::createCellWidget()
{
    QWidget *cell = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(cell);
    layout->setContentsMargins(2,0,2,0);
    
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0,127);
    QSpinBox *spinVal = new QSpinBox;
    spinVal->setRange(0,127);
    
    connect(slider, &QSlider::valueChanged, spinVal, &QSpinBox::setValue);
    connect(spinVal, &QSpinBox::valueChanged, slider, &QSlider::setValue);
    
    layout->addWidget(slider);
    layout->addWidget(spinVal);
    return cell;
}

// ------------------- Add Row -------------------
void MIDICCTableWidget::addRow()
{
    int row = rowCount++;
    
    // Delete button
    QPushButton *deleteRowBtn = new QPushButton("X");
    rowDeleteButtons.append(deleteRowBtn);
    grid->addWidget(deleteRowBtn, row + 2, 0);
    connect(deleteRowBtn, &QPushButton::clicked, [this, row](){ deleteRow(row); });
    
    // Row header
    QWidget *rowHeader = new QWidget;
    QHBoxLayout *headerLayout = new QHBoxLayout(rowHeader);
    headerLayout->setContentsMargins(2,0,2,0);
    
    QLabel *label = new QLabel("CC");
    QSpinBox *spinCC = new QSpinBox;
    spinCC->setRange(0,127);
    spinCC->setValue(rowHeaderSpinBoxes.isEmpty() ? 0 : std::min(rowHeaderSpinBoxes.last()->value() + 1,127));
    rowHeaderSpinBoxes.append(spinCC);
    
    headerLayout->addWidget(label);
    headerLayout->addWidget(spinCC);
    grid->addWidget(rowHeader, row + 2, 1);
    
    // Add cells for each column
    for(int c=0; c<colCount; ++c)
        grid->addWidget(createCellWidget(), row + 2, c + 2);
}

// ------------------- Add Column -------------------
void MIDICCTableWidget::addColumn()
{
    int col = colCount++;
    
    // Column header
    QWidget *headerCell = new QWidget;
    QVBoxLayout *headerLayout = new QVBoxLayout(headerCell);
    headerLayout->setContentsMargins(2,0,2,0);
    
    QPushButton *deleteColBtn = new QPushButton("X");
    colDeleteButtons.append(deleteColBtn);
    headerLayout->addWidget(deleteColBtn, 0, Qt::AlignHCenter);
    
    QHBoxLayout *labelLayout = new QHBoxLayout;
    QLabel *label = new QLabel("Channel");
    QSpinBox *spin = new QSpinBox;
    spin->setRange(1,16);
    spin->setValue(colHeaderSpinBoxes.isEmpty() ? 1 : std::min(colHeaderSpinBoxes.last()->value()+1,16));
    colHeaderSpinBoxes.append(spin);
    
    labelLayout->addWidget(label);
    labelLayout->addWidget(spin);
    headerLayout->addLayout(labelLayout);
    
    grid->addWidget(headerCell, 0, col + 2);
    
    connect(deleteColBtn, &QPushButton::clicked, [this, col](){ deleteColumn(col); });
    
    // Add cells for existing rows
    for(int r=0; r<rowCount; ++r)
        grid->addWidget(createCellWidget(), r + 2, col + 2);
}

// ------------------- Delete Row -------------------
void MIDICCTableWidget::deleteRow(int row)
{
    for(int c=0; c<=colCount; ++c){
        if(QLayoutItem *item = grid->itemAtPosition(row + 2, c))
            if(QWidget *w = item->widget()){ w->hide(); w->deleteLater(); }
    }
    rowHeaderSpinBoxes.remove(row);
    rowDeleteButtons.remove(row);
    rowCount--;
}

// ------------------- Delete Column -------------------
void MIDICCTableWidget::deleteColumn(int col)
{
    for(int r=0; r<=rowCount + 1; ++r){
        if(QLayoutItem *item = grid->itemAtPosition(r, col + 2))
            if(QWidget *w = item->widget()){ w->hide(); w->deleteLater(); }
    }
    colHeaderSpinBoxes.remove(col);
    colDeleteButtons.remove(col);
    colCount--;
}

// ------------------- MIDIControlChangeSelector -------------------
MIDIControlChangeSelector::MIDIControlChangeSelector(QWidget *parent)
    : QWidget(parent)
{
    tabWidget = new QTabWidget(this);
    for(int i=0; i<12; ++i)
        tabWidget->addTab(new MIDICCTableWidget, QString("F%1").arg(i+1));
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(tabWidget);
}
