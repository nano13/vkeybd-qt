#include "help_reader.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QPointer>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
QPointer<HelpReader> reader;
}

void HelpReader::showPage(const QString &page, QWidget *parent)
{
    if (!reader)
        reader = new HelpReader(parent);
    
    reader->selectPage(page);
    reader->show();
    reader->raise();
    reader->activateWindow();
}

HelpReader::HelpReader(QWidget *parent) : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("vkeybd-qt help");
    setMinimumSize(500, 500);
    resize(700, 760);
    
    this->pages = {
        {"layout.md", "Keyboard Layout"},
        {"keyboards.md", "Keyboard Recommendations"},
        {"interception.md", "Windows Lock Driver"},
    };
    
    this->tabs = new QTabWidget;
    for (const Page &page : this->pages)
    {
        QTextBrowser *browser = new QTextBrowser;
        // links are handled here: other pages switch the tab, web links open the browser
        browser->setOpenLinks(false);
        connect(browser, &QTextBrowser::anchorClicked, this, &HelpReader::linkClicked);
        browser->setSource(QUrl("qrc:/docs/" + page.file), QTextDocument::MarkdownResource);
        this->tabs->addTab(browser, page.title);
    }
    
    QPushButton *button_close = new QPushButton("Close");
    connect(button_close, &QPushButton::clicked, this, &QDialog::close);
    
    QHBoxLayout *layout_buttons = new QHBoxLayout;
    layout_buttons->addStretch(1);
    layout_buttons->addWidget(button_close);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(this->tabs);
    layout->addLayout(layout_buttons);
    setLayout(layout);
}

void HelpReader::selectPage(const QString &page)
{
    for (int i = 0; i < this->pages.length(); i++)
    {
        if (this->pages.at(i).file == page)
        {
            this->tabs->setCurrentIndex(i);
            return;
        }
    }
}

void HelpReader::linkClicked(const QUrl &url)
{
    // a link to another help page: "layout.md", resolved to qrc:/docs/layout.md
    const QString file = url.fileName();
    if (url.scheme().isEmpty() || url.scheme() == "qrc")
    {
        for (const Page &page : this->pages)
        {
            if (page.file == file)
            {
                selectPage(file);
                return;
            }
        }
        // a doc that is not bundled (e.g. keyboard-input.md): nothing to show here
        return;
    }
    
    QDesktopServices::openUrl(url);
}
