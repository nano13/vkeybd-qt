#ifndef KEYBOARDPC_H
#define KEYBOARDPC_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QList>
#include <QDebug>

class KeyboardPC : public QWidget
{
    Q_OBJECT
public:
    explicit KeyboardPC(QWidget *parent = nullptr);
    
    float button_scale = .799;
    
    void keyDown(int keycode);
    void keyUp(int keycode);
    
private:
    QList<QList<int>> getButtonSizes();
    QList<QList<QString>> getButtonColors();
    QList<QList<QString>> getButtonLabels();
    QList<QList<int>> getButtonKeycodes();
    QList<QList<int>> getMIDICodes();
    
    QList<QList<QPushButton*>> list_of_buttons;
    
    void drawButtons();
    float calculateOffset(float row_offset, int row, int col, QList<QList<int>> sizes);
    
signals:
    void MIDIPress(int midicode);
    void MIDIRelease(int midicode);
};

#endif // KEYBOARDPC_H
