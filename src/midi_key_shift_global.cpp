#include "midi_key_shift_global.h"

MIDIKeyShiftGlobal::MIDIKeyShiftGlobal(QWidget *parent)
    : QWidget{parent}
{
    QGridLayout *grid = new QGridLayout();
    setLayout(grid);
    
    setObjectName("sub_window");
    QFile css_file(":css_allen_heath");
    if (css_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        setStyleSheet(css_file.readAll());
    }
    
    this->key_shift_absolute = new MIDIKeyShiftWidget();
    connect(this->key_shift_absolute, &MIDIKeyShiftWidget::signalValueChanged, this, [this]{ valueChanged(this->key_shift_absolute->value(), false); });
    
    this->key_shift_relative = new MIDIKeyShiftWidget();
    connect(this->key_shift_relative, &MIDIKeyShiftWidget::signalValueChanged, this, [this]{ valueChanged(this->key_shift_relative->value(), true); });
    
    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    
    grid->addWidget(new QLabel("Global Key Shift (absolute)"), 0, 0);
    grid->addWidget(this->key_shift_absolute, 1, 0);
    grid->addWidget(line, 2, 0);
    grid->addWidget(new QLabel("Global Key Shift (relative)"), 3, 0);
    grid->addWidget(this->key_shift_relative, 4, 0);
}

void MIDIKeyShiftGlobal::setGlobalValue(int value)
{
    this->key_shift_absolute->setValueSilently(value);
    this->key_shift_relative->setValueSilently(value);
    this->last_relative_pitch = value;
}

void MIDIKeyShiftGlobal::valueChanged(int value, bool is_relative)
{
    if (! is_relative)
    {
        emit signalKeyShiftChanged(value, is_relative);
    }
    else
    {
        emit signalKeyShiftChanged(-1*(this->last_relative_pitch - value), is_relative);
        
        this->last_relative_pitch = value;
    }
}
