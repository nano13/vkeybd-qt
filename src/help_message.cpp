#include "help_message.h"

#include <QHBoxLayout>
#include <QTextBrowser>
#include <QUrl>

HelpMessage::HelpMessage(QString filename, QString title, QWidget *parent) : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(title);
    setMinimumSize(500, 500);
    resize(640, 720);
    
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    
    QTextBrowser *text = new QTextBrowser;
    // web links open in the browser; links to other bundled docs open here
    text->setOpenExternalLinks(true);
    layout->addWidget(text);
    
    if (filename.endsWith(".md"))
    {
        // loaded as a source, so relative links (other .md files) resolve
        const QUrl url = filename.startsWith(":") ? QUrl("qrc" + filename) : QUrl::fromLocalFile(filename);
        text->setSource(url, QTextDocument::MarkdownResource);
    }
    else
    {
        QString content;
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            content = QString::fromUtf8(file.readAll());
        text->setText(content);
    }
    
    QHBoxLayout *layout_buttons = new QHBoxLayout;
    
    QPushButton *button_back = new QPushButton("Back");
    button_back->setEnabled(false);
    connect(button_back, &QPushButton::clicked, text, &QTextBrowser::backward);
    connect(text, &QTextBrowser::backwardAvailable, button_back, &QPushButton::setEnabled);
    // the window title follows the page
    connect(text, &QTextBrowser::sourceChanged, this, [this, text]{
        const QString page_title = text->documentTitle();
        if (!page_title.isEmpty())
            setWindowTitle(page_title);
    });
    
    QPushButton *button_close = new QPushButton("Close");
    connect(button_close, &QPushButton::clicked, this, &QDialog::close);
    
    layout_buttons->addWidget(button_back);
    layout_buttons->addStretch(1);
    layout_buttons->addWidget(button_close);
    layout->addLayout(layout_buttons);
    
    // not modal: playing goes on while it is open
    show();
}
