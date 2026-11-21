#ifndef HELPMESSAGE_H
#define HELPMESSAGE_H

#include <QDialog>
#include <QString>

// kept for the files that got them through this header so far
#include <QFile>
#include <QPushButton>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

/*
 * Shows a text file from the resources in its own window, not modal (the
 * keyboards keep playing). Files ending in ".md" are rendered as Markdown,
 * others as plain text or HTML. Links open in the web browser.
 * Deletes itself when closed.
 */
class HelpMessage : public QDialog
{
    Q_OBJECT
public:
    explicit HelpMessage(QString filename, QString title = "KeyDeskMIDI - help", QWidget *parent = nullptr);
};

#endif // HELPMESSAGE_H
