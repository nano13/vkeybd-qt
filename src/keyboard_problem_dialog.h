#ifndef KEYBOARDPROBLEMDIALOG_H
#define KEYBOARDPROBLEMDIALOG_H

#include <QString>

#include "input_keyboard_raw.h"

class QWidget;

/*
 * Explains why a keyboard could not be opened or locked, and what to do
 * about it (e.g. install the Interception driver on Windows), with links and
 * buttons. Not modal: the other keyboards keep playing while it is open.
 *
 * keyboard: name of the keyboard as shown in the selector
 * listening: the keyboard is used without lock now
 */
void showKeyboardProblemDialog(QWidget *parent, const QString &keyboard, const QString &message, RawKeyboardProblem problem, bool listening);

#endif // KEYBOARDPROBLEMDIALOG_H
