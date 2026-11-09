#include "keyboard_problem_dialog.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>

namespace
{
const char *INTERCEPTION_RELEASES = "https://github.com/oblitum/Interception/releases";
const char *INTERCEPTION_PROJECT = "https://github.com/oblitum/Interception";
const char *MACOS_INPUT_MONITORING = "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent";

QString interceptionSteps()
{
    return QString(
        "<ol>"
        "<li>Download <b>Interception.zip</b> from the <a href=\"%1\">Interception releases</a>.</li>"
        "<li>Open a command prompt <b>as administrator</b> in its folder <i>command line installer</i> and run<br>"
        "<code>install-interception.exe /install</code></li>"
        "<li><b>Reboot</b> Windows.</li>"
        "<li>Copy <b>interception.dll</b> from <i>library\\x64</i> (64-bit vkeybd-qt) or <i>library\\x86</i> "
        "(32-bit) next to <b>vkeybd-qt.exe</b>.</li>"
        "<li>Start vkeybd-qt again and lock the keyboard.</li>"
        "</ol>"
        "<p>Interception is a signed keyboard filter driver by Francisco Lopes: free for non-commercial use, "
        "see the <a href=\"%2\">project page</a> for the licence.<br>"
        "Known limitation: every unplug/replug of a keyboard uses up one of its 10 keyboard slots; "
        "when they are used up, the keyboard stops working until the next reboot. "
        "Plug in all keyboards before the performance.</p>")
        .arg(QString::fromLatin1(INTERCEPTION_RELEASES), QString::fromLatin1(INTERCEPTION_PROJECT));
}
}

void showKeyboardProblemDialog(QWidget *parent, const QString &keyboard, const QString &message, RawKeyboardProblem problem, bool listening)
{
    QString title = "Keyboard not available";
    QString headline;
    QString details;
    QString button_text;
    QString button_url;
    
    switch (problem)
    {
        case RawKeyboardProblem::LockNeedsInterception:
            title = "Keyboard lock not available";
            headline = "Locking a keyboard on Windows needs the free <b>Interception</b> driver.";
            details = "<p>Without it, vkeybd-qt can only <i>listen</i> to a keyboard: its keys also reach Windows "
                      "and the focused application (shortcuts, the Start menu, ...).</p>" + interceptionSteps();
            button_text = "Open download page";
            button_url = QString::fromLatin1(INTERCEPTION_RELEASES);
            break;
            
        case RawKeyboardProblem::InterceptionNotRunning:
            title = "Keyboard lock not available";
            headline = "interception.dll was found, but the <b>Interception driver</b> is not running.";
            details = "<p>Did you reboot after installing the driver? If it is not installed yet:</p>" + interceptionSteps();
            button_text = "Open download page";
            button_url = QString::fromLatin1(INTERCEPTION_RELEASES);
            break;
            
        case RawKeyboardProblem::LockBusy:
            title = "Keyboard lock not available";
            headline = "Another program has locked (grabbed) this keyboard already.";
            details = "<p>Only one program can lock a keyboard. Typical candidates: a second vkeybd-qt instance, "
                      "<code>evtest --grab</code>, key remappers like keyd, kmonad, kanata or input-remapper.</p>"
                      "<p>Close that program, then lock the keyboard again.</p>";
            break;
            
        case RawKeyboardProblem::PermissionDenied:
            headline = "vkeybd-qt is not allowed to read this keyboard.";
            details = "<p>Raw keyboards are read from <code>/dev/input/event*</code>, which usually needs membership "
                      "in the group <b>input</b>:</p>"
                      "<p><code>sudo usermod -aG input $USER</code></p>"
                      "<p>Then log out and in again (or reboot).</p>"
                      "<p>Note: every program of your user can then read all input devices.</p>";
            break;
            
        case RawKeyboardProblem::LockUnsupported:
            title = "Keyboard lock not available";
            headline = "Locking keyboards is not supported on macOS.";
            details = "<p>macOS only lets programs running as root take exclusive control of a keyboard, "
                      "so vkeybd-qt can only <i>listen</i>: the keys also reach macOS and the focused application.</p>"
                      "<p>For a locked live setup, use Linux or Windows (with the Interception driver).</p>";
            break;
            
        case RawKeyboardProblem::InputMonitoring:
            headline = "vkeybd-qt needs the <b>Input Monitoring</b> permission to read keyboards.";
            details = "<p>Allow vkeybd-qt in <i>System Settings &gt; Privacy &amp; Security &gt; Input Monitoring</i>, "
                      "then quit and start vkeybd-qt again.</p>";
            button_text = "Open Input Monitoring settings";
            button_url = QString::fromLatin1(MACOS_INPUT_MONITORING);
            break;
            
        case RawKeyboardProblem::Disconnected:
            headline = "The keyboard was disconnected.";
            details = "<p>Plug it in again, press the rescan button next to the selector and select it again.</p>";
            break;
            
        case RawKeyboardProblem::Unavailable:
            headline = "The keyboard could not be opened.";
            break;
    }
    
    if (listening)
        details += "<p><b>The keyboard is used without lock now.</b></p>";
    if (!message.isEmpty())
        details += "<p><small>" + message.toHtmlEscaped() + "</small></p>";
    
    QString name = keyboard;
    name.replace("\n", " ");
    
    QMessageBox *box = new QMessageBox(parent);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setIcon(QMessageBox::Warning);
    box->setWindowTitle(title);
    box->setTextFormat(Qt::RichText);
    box->setText("<b>" + name.toHtmlEscaped() + "</b><br>" + headline);
    box->setInformativeText(details);
    // links clickable, commands selectable for copy & paste
    box->setTextInteractionFlags(Qt::TextBrowserInteraction);
    box->setStandardButtons(QMessageBox::Close);
    
    if (!button_text.isEmpty())
    {
        // ActionRole: does not close the box, so the steps stay visible
        QPushButton *button = box->addButton(button_text, QMessageBox::ActionRole);
        const QUrl url(button_url);
        QObject::connect(button, &QPushButton::clicked, box, [url]() { QDesktopServices::openUrl(url); });
    }
    
    // not modal: the other keyboards keep playing
    box->open();
}
