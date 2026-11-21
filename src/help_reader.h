#ifndef HELPREADER_H
#define HELPREADER_H

#include <QDialog>
#include <QList>
#include <QString>

class QTabWidget;
class QTextBrowser;
class QUrl;

/*
 * The help reader: one window with a tab per help page (docs/*.md, bundled
 * in the resources under :/docs). Not modal, opened once; showing a page
 * again brings the window to the front on that tab. Links between the pages
 * switch the tab, web links open in the browser.
 */
class HelpReader : public QDialog
{
    Q_OBJECT
public:
    // page: file name in docs/, e.g. "layout.md"
    static void showPage(const QString &page, QWidget *parent = nullptr);
    
private:
    explicit HelpReader(QWidget *parent = nullptr);
    
    struct Page
    {
        QString file;
        QString title;
    };
    QList<Page> pages;
    QTabWidget *tabs;
    
    void selectPage(const QString &page);
    void linkClicked(const QUrl &url);
};

#endif // HELPREADER_H
