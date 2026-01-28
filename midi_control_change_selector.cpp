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
#include <QVector>
#include <algorithm>

// ------------------- MIDICCTableWidget definition -------------------
MIDICCTableWidget::MIDICCTableWidget(QWidget *parent)
    : QWidget(parent)
{
    // ------------------ Layout ------------------
    QVBoxLayout *tabLayout = new QVBoxLayout(this);
    tabLayout->setContentsMargins(2,2,2,2);
    
    // Buttons
    QPushButton *addRowBtn = new QPushButton("Add CC");
    QPushButton *addColBtn = new QPushButton("Add Channel");
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addRowBtn);
    buttonLayout->addWidget(addColBtn);
    buttonLayout->addStretch();
    tabLayout->addLayout(buttonLayout);
    
    // ------------------ Scrollable Grid ------------------
    QWidget *gridWidget = new QWidget;
    QGridLayout *grid = new QGridLayout(gridWidget);
    grid->setSpacing(8);               // space between both rows and columns
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(12);    // extra space between columns
    grid->setVerticalSpacing(4);       // row spacing
    
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(gridWidget);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    tabLayout->addWidget(scroll);
    
    // ------------------ Minimum Size ------------------
    setMinimumSize(1000, 500);
    
    // ------------------ State Tracking ------------------
    int *rowCount = new int(0);
    int *colCount = new int(0);
    QVector<QSpinBox*> rowHeaderSpinBoxes;
    QVector<QSpinBox*> colHeaderSpinBoxes;
    
    // ------------------ Add Row ------------------
    connect(addRowBtn, &QPushButton::clicked, [=]() mutable {
        int row = (*rowCount)++;
        
        // Row header: "CC" + spinbox
        QWidget *rowHeader = new QWidget;
        QHBoxLayout *headerLayout = new QHBoxLayout(rowHeader);
        headerLayout->setContentsMargins(2,0,2,0); // padding inside cell
        
        QLabel *label = new QLabel("CC");
        QSpinBox *spinCC = new QSpinBox;
        spinCC->setRange(0,127);
        
        if (!rowHeaderSpinBoxes.isEmpty())
            spinCC->setValue(std::min(rowHeaderSpinBoxes.last()->value() + 1, 127));
        else
            spinCC->setValue(0);
        
        rowHeaderSpinBoxes.append(spinCC);
        headerLayout->addWidget(label);
        headerLayout->addWidget(spinCC);
        
        grid->addWidget(rowHeader, row + 1, 0); // column 0 = row headers
        
        // Add cells for each column
        for (int c = 0; c < *colCount; ++c) {
            QWidget *cell = new QWidget;
            QHBoxLayout *cellLayout = new QHBoxLayout(cell);
            cellLayout->setContentsMargins(2,0,2,0); // padding inside cell
            
            QSlider *slider = new QSlider(Qt::Horizontal);
            slider->setRange(0,127);
            QSpinBox *spinVal = new QSpinBox;
            spinVal->setRange(0,127);
            
            connect(slider, &QSlider::valueChanged, spinVal, &QSpinBox::setValue);
            connect(spinVal, &QSpinBox::valueChanged, slider, &QSlider::setValue);
            
            cellLayout->addWidget(slider);
            cellLayout->addWidget(spinVal);
            
            grid->addWidget(cell, row + 1, c + 1);
        }
    });
    
    // ------------------ Add Column ------------------
    connect(addColBtn, &QPushButton::clicked, [=]() mutable {
        int col = (*colCount)++;
        
        // Column header: "Channel" + spinbox
        QWidget *headerCell = new QWidget;
        QHBoxLayout *headerLayout = new QHBoxLayout(headerCell);
        headerLayout->setContentsMargins(2,0,2,0); // padding
        
        QLabel *label = new QLabel("Channel");
        QSpinBox *spin = new QSpinBox;
        spin->setRange(1,16);
        
        if (!colHeaderSpinBoxes.isEmpty())
            spin->setValue(std::min(colHeaderSpinBoxes.last()->value() + 1, 16));
        else
            spin->setValue(1);
        
        colHeaderSpinBoxes.append(spin);
        headerLayout->addWidget(label);
        headerLayout->addWidget(spin);
        
        grid->addWidget(headerCell, 0, col + 1); // row 0 = header
        
        // Add cells for existing rows
        for (int r = 0; r < *rowCount; ++r) {
            QWidget *cell = new QWidget;
            QHBoxLayout *cellLayout = new QHBoxLayout(cell);
            cellLayout->setContentsMargins(2,0,2,0); // padding
            
            QSlider *slider = new QSlider(Qt::Horizontal);
            slider->setRange(0,127);
            QSpinBox *spinVal = new QSpinBox;
            spinVal->setRange(0,127);
            
            connect(slider, &QSlider::valueChanged, spinVal, &QSpinBox::setValue);
            connect(spinVal, &QSpinBox::valueChanged, slider, &QSlider::setValue);
            
            cellLayout->addWidget(slider);
            cellLayout->addWidget(spinVal);
            
            grid->addWidget(cell, r + 1, col + 1);
        }
    });
}


// ------------------- MIDIControlChangeSelector -------------------
MIDIControlChangeSelector::MIDIControlChangeSelector(QWidget *parent)
    : QWidget(parent)
{
    tabWidget = new QTabWidget(this);
    
    for (int i = 0; i < 12; ++i)
        tabWidget->addTab(new MIDICCTableWidget, QString("F%1").arg(i + 1));
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(tabWidget);
    setLayout(layout);
}
