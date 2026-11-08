#ifndef MAIN_SUBTABS_SWITCHER_H
#define MAIN_SUBTABS_SWITCHER_H

#include <QObject>
#include <QWidget>

#include "config.h"

class MainSubTabsSwitcher : public QWidget
{
    Q_OBJECT
public:
    MainSubTabsSwitcher(int keyboard_id = 0, Config *config = nullptr, QWidget *parent = nullptr);
};

#endif // MAIN_SUBTABS_SWITCHER_H
