#include "keyboard_pc.h"

KeyboardPC::KeyboardPC(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(qRound(450 * this->button_scale));
    setFixedHeight(qRound(5 * 30 * this->button_scale));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    drawButtons();
    layoutButtons();
}

void KeyboardPC::drawButtons()
{
    QList<QList<int>> sizes = getButtonSizes();
    QList<QList<QString>> colors = getButtonColors();
    QList<QList<QString>> labels = getButtonLabels();
    QList<QList<int>> midicodes = getMIDICodes();
    QList<QList<int>> keycodes = getButtonKeycodesRaw();
    
    for (int row = 0; row < sizes.length(); row++)
    {
        QList<QPushButton*> list_of_buttons_row;
        for (int col = 0; col < sizes.at(row).length(); col++)
        {
            QPushButton *button = new QPushButton(this);
            QString button_text = labels.at(row).at(col);
            button->setText(button_text.replace("|", "\n"));

            int midicode = midicodes.at(row).at(col);
            if (midicode > -1)
            {
                connect(button, &QPushButton::pressed, this, [this, midicode]{ emit MIDIPress(midicode); });
                connect(button, &QPushButton::released, this, [this, midicode]{ emit MIDIRelease(midicode); });
            }
            int keycode = keycodes.at(row).at(col);
            if (keycode == KeysRaw::Control_L)
            {
                connect(button, &QPushButton::clicked, this, [this]{ emit octaveDown(); });
            }
            else if (keycode == KeysRaw::Control_R)
            {
                connect(button, &QPushButton::clicked, this, [this]{ emit octaveUp(); });
            }
            
            button->show();
            
            if (colors.at(row).at(col) == "b")
            {
                button->setObjectName("pc_black");
            }
            else if (colors.at(row).at(col) == "grey")
            {
                button->setObjectName("pc_grey");
            }
            else if (colors.at(row).at(col) == "gold")
            {
                button->setObjectName("pc_gold");
            }
            else if (colors.at(row).at(col) == "blue")
            {
                button->setObjectName("pc_blue");
            }
            else
            {
                button->setObjectName("pc_white");
            }
            
            list_of_buttons_row.append(button);
        }
        this->list_of_buttons.append(list_of_buttons_row);
    }
    //qDebug() << this->list_of_buttons;
}

void KeyboardPC::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutButtons();
}

void KeyboardPC::layoutButtons()
{
    const QList<QList<int>> sizes = getButtonSizes();
    const int row_count = sizes.size();

    for (int row = 0; row < row_count; row++)
    {
        const QList<int> &row_sizes = sizes.at(row);
        int row_width = 0;
        for (int key_width : row_sizes)
            row_width += key_width;

        const int top = qRound(static_cast<qreal>(row) * height() / row_count);
        const int bottom = qRound(static_cast<qreal>(row + 1) * height() / row_count);
        int x = 0;

        for (int col = 0; col < row_sizes.size(); col++)
        {
            const int left = qRound(static_cast<qreal>(x) * width() / row_width);
            x += row_sizes.at(col);
            const int right = qRound(static_cast<qreal>(x) * width() / row_width);
            this->list_of_buttons.at(row).at(col)->setGeometry(left, top, right - left, bottom - top);
        }
    }
}

QList<QList<int>> KeyboardPC::getButtonSizes()
{
    QList<QList<int>> list;
    list = {{30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 60},
            {40, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 50},
            {50, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 70},
            {40, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 80},
            {40, 40, 40, 170, 40, 40, 40, 40}};
    return list;
}

QList<QList<QString>> KeyboardPC::getButtonColors()
{
    QList<QList<QString>> list;
    list = {{"b", "w", "w", "w", "b", "w", "w", "w", "b", "w", "w", "w", "b", "w"},
            {"w", "w", "b", "b", "w", "w", "b", "b", "w", "w", "b", "b", "w", "w"},
            {"b", "b", "w", "w", "b", "b", "w", "w", "b", "b", "w", "w", "b"},
            {"b", "w", "w", "w", "b", "w", "w", "w", "b", "w", "w", "w", "b"},
            {"blue", "gold", "gold", "gold", "gold", "gold", "grey", "blue"}};
    return list;
}

QList<QList<QString>> KeyboardPC::getButtonLabels()
{
    QList<QList<QString>> list;
    list = {{"Ab|G#", "B", "D", "F", "Ab|G#", "B", "D", "F", "Ab|G#", "B", "D", "F", "Ab|Gis", "B"},
            {"A", "C", "Eb|D#", "Gb|F#", "A", "C", "Eb|D#", "Gb|F#", "A", "C", "Eb|D#", "Gb|F#", "A", "C"},
            {"Bb|A#", "Db|C#", "E", "G", "Bb|A#", "Db|C#", "E", "G", "Bb|A#", "Db|C#", "E", "G", "Bb|A#"},
            {"Ab|G#", "B", "D", "F", "Ab|G#", "B", "D", "F", "Ab|G#", "B", "D" ,"F", "Ab|G#"},
            {"<<", "soft", "soste|nuto", "sustain", "soste|nuto", "soft", "", ">>"}};
    return list;
}

QList<QList<int>> KeyboardPC::getButtonKeycodesRaw()
{
    QList<QList<int>> list;
    list = {{41, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
            {15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28},
            {58, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 43},
            {42, 86, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54},
            {KeysRaw::Control_L, 125, 56, 57, 100, -1, -1, KeysRaw::Control_R}};
    return list;
}

QList<QList<int>> KeyboardPC::getMIDICodes()
{
    QList<QList<int>> list;
    list = {{8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47},
            {9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48},
            {10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46},
            {8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44},
            {-1, -1, -1, -1, -1, -1, -1, -1}};
    return list;
}

void KeyboardPC::keyDownRaw(int keycode)
{
    QList<QList<int>> keycodes = getButtonKeycodesRaw();
    for (int row=0; row < keycodes.length(); row++)
    {
        for (int col=0; col < keycodes.at(row).length(); col++)
        {
            if (keycodes.at(row).at(col) == keycode)
            {
                this->list_of_buttons.at(row).at(col)->setDown(true);
                
                QList<QList<int>> midicodes = getMIDICodes();
                if (midicodes.at(row).at(col) > -1)
                {
                    emit MIDIPress(midicodes.at(row).at(col));
                }
            }
        }
    }
}
void KeyboardPC::keyUpRaw(int keycode)
{
    QList<QList<int>> keycodes = getButtonKeycodesRaw();
    for (int row=0; row < keycodes.length(); row++)
    {
        for (int col=0; col < keycodes.at(row).length(); col++)
        {
            if (keycodes.at(row).at(col) == keycode)
            {
                this->list_of_buttons.at(row).at(col)->setDown(false);
                
                QList<QList<int>> midicodes = getMIDICodes();
                if (midicodes.at(row).at(col) > -1)
                {
                    emit MIDIRelease(midicodes.at(row).at(col));
                }
            }
        }
    }
}

void KeyboardPC::allKeysUp()
{
    QList<QList<int>> keycodes = getButtonKeycodesRaw();
    for (int row=0; row < keycodes.length(); row++)
    {
        for (int col=0; col < keycodes.at(row).length(); col++)
        {
            this->list_of_buttons.at(row).at(col)->setDown(false);
        }
    }
}
