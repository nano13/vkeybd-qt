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
#include <QWidget>
#include <QVector>
#include <algorithm>

MIDIControlChangeSelector::MIDIControlChangeSelector(QWidget *parent)
    : QWidget(parent)
{
    tabWidget = new QTabWidget(this);
    
    // Create 12 tabs
    for (int i = 0; i < 12; ++i) {
        setupDynamicTab(i);
    }
    
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(tabWidget);
    setLayout(layout);
}

void MIDIControlChangeSelector::setupDynamicTab(int tabIndex)
{
    // Tab container
    QWidget *tabContent = new QWidget;
    QVBoxLayout *tabLayout = new QVBoxLayout(tabContent);
    tabLayout->setContentsMargins(2,2,2,2);
    
    // Buttons to add rows/columns
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *addRowBtn = new QPushButton("Add CC");
    QPushButton *addColBtn = new QPushButton("Add Channel");
    buttonLayout->addWidget(addRowBtn);
    buttonLayout->addWidget(addColBtn);
    buttonLayout->addStretch();
    tabLayout->addLayout(buttonLayout);
    
    // Scrollable grid area
    QWidget *gridWidget = new QWidget;
    QGridLayout *grid = new QGridLayout(gridWidget);
    grid->setSpacing(4);
    grid->setContentsMargins(0,0,0,0);
    
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(gridWidget);
    tabLayout->addWidget(scroll);
    
    tabWidget->addTab(tabContent, QString("F%1").arg(tabIndex + 1));
    
    // Track rows/columns
    int *rowCount = new int(0);   // number of CC rows
    int *colCount = new int(0);   // number of channel columns
    
    // Column headers: store spin boxes for auto-increment
    QVector<QSpinBox*> colHeaderSpinBoxes;
    
    // Row headers: store spin boxes for auto-increment
    QVector<QSpinBox*> rowHeaderSpinBoxes;
    
    // ---------------- Add Row ----------------
    connect(addRowBtn, &QPushButton::clicked, [=]() mutable {
        int row = (*rowCount)++;
        
        // --- Row header: "CC" + spinbox ---
        QWidget *rowHeader = new QWidget;
        QHBoxLayout *headerLayout = new QHBoxLayout(rowHeader);
        headerLayout->setContentsMargins(0,0,0,0);
        
        QLabel *label = new QLabel("CC");
        QSpinBox *spinCC = new QSpinBox;
        spinCC->setRange(0,127);
        
        // Auto-increment from previous row
        if (!rowHeaderSpinBoxes.isEmpty()) {
            int prev = rowHeaderSpinBoxes.last()->value();
            spinCC->setValue(std::min(prev + 1, 127));
        } else {
            spinCC->setValue(0);
        }
        
        rowHeaderSpinBoxes.append(spinCC);
        
        headerLayout->addWidget(label);
        headerLayout->addWidget(spinCC);
        
        grid->addWidget(rowHeader, row + 1, 0); // column 0 reserved for row headers
        
        // --- Add cells for existing columns ---
        for (int c = 0; c < *colCount; ++c) {
            QWidget *cell = new QWidget;
            QHBoxLayout *cellLayout = new QHBoxLayout(cell);
            cellLayout->setContentsMargins(0,0,0,0);
            
            QSlider *slider = new QSlider(Qt::Horizontal);
            slider->setRange(0,127);
            QSpinBox *spinVal = new QSpinBox;
            spinVal->setRange(0,127);
            
            connect(slider, &QSlider::valueChanged, spinVal, &QSpinBox::setValue);
            connect(spinVal, &QSpinBox::valueChanged, slider, &QSlider::setValue);
            
            cellLayout->addWidget(slider);
            cellLayout->addWidget(spinVal);
            
            grid->addWidget(cell, row + 1, c + 1); // +1 for header column
        }
    });
    
    // ---------------- Add Column ----------------
    connect(addColBtn, &QPushButton::clicked, [=]() mutable {
        int col = (*colCount)++;
        
        // --- Column header: "Channel" + spinbox ---
        QWidget *headerCell = new QWidget;
        QHBoxLayout *headerLayout = new QHBoxLayout(headerCell);
        headerLayout->setContentsMargins(0,0,0,0);
        
        QLabel *label = new QLabel("Channel");
        QSpinBox *spin = new QSpinBox;
        spin->setRange(1,16);
        
        // Default value = previous column +1
        if (!colHeaderSpinBoxes.isEmpty()) {
            int prev = colHeaderSpinBoxes.last()->value();
            spin->setValue(std::min(prev + 1, 16));
        } else {
            spin->setValue(1);
        }
        
        colHeaderSpinBoxes.append(spin);
        headerLayout->addWidget(label);
        headerLayout->addWidget(spin);
        
        grid->addWidget(headerCell, 0, col + 1); // row 0, column 0 reserved for row headers
        
        // --- Add cells for existing rows ---
        for (int r = 0; r < *rowCount; ++r) {
            QWidget *cell = new QWidget;
            QHBoxLayout *cellLayout = new QHBoxLayout(cell);
            cellLayout->setContentsMargins(0,0,0,0);
            
            QSlider *slider = new QSlider(Qt::Horizontal);
            slider->setRange(0,127);
            QSpinBox *spinVal = new QSpinBox;
            spinVal->setRange(0,127);
            
            connect(slider, &QSlider::valueChanged, spinVal, &QSpinBox::setValue);
            connect(spinVal, &QSpinBox::valueChanged, slider, &QSlider::setValue);
            
            cellLayout->addWidget(slider);
            cellLayout->addWidget(spinVal);
            
            grid->addWidget(cell, r + 1, col + 1); // +1 for header/labels
        }
    });
}

