#include "keyboard_layout.h"

#include <QGuiApplication>
#include <QInputMethod>
#include <QLocale>

#include "enums_structs.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <Carbon/Carbon.h>
#endif

namespace
{
constexpr int KEY_102ND = 86;       // < > | next to the left Shift (ISO)
constexpr int KEY_RO = 89;          // JIS, next to the right Shift
constexpr int KEY_HENKAN = 92;      // JIS, right of the space bar
constexpr int KEY_KATAKANAHIRAGANA = 93;
constexpr int KEY_MUHENKAN = 94;    // JIS, left of the space bar
constexpr int KEY_YEN = 124;        // JIS, number row

QList<PcKeyDef> numberRow(KeyboardForm form)
{
    const PcKeyRole N = PcKeyRole::Note;
    QList<PcKeyDef> row = {{41, 30, N, 0}, {2, 30, N, 1}, {3, 30, N, 2}, {4, 30, N, 3}, {5, 30, N, 4},
                           {6, 30, N, 5}, {7, 30, N, 6}, {8, 30, N, 7}, {9, 30, N, 8}, {10, 30, N, 9},
                           {11, 30, N, 10}, {12, 30, N, 11}, {13, 30, N, 12}};
    if (form == KeyboardForm::JIS)
    {
        row.append(PcKeyDef{KEY_YEN, 30, N, 13});
        row.append(PcKeyDef{14, 30, N, 14});        // Backspace, narrow on JIS
    }
    else
    {
        row.append(PcKeyDef{14, 60, N, 13});
    }
    return row;
}

QList<PcKeyDef> functionRow(KeyboardForm form)
{
    const PcKeyRole N = PcKeyRole::Note;
    if (form == KeyboardForm::JIS)
    {
        // the Japanese keys around the space bar can be given notes in the keymap editor
        return {{KeysRaw::Control_L, 40, PcKeyRole::OctaveDown, -1}, {KeysRaw::Super_L, 40, PcKeyRole::Soft, -1},
                {KeysRaw::Alt, 40, PcKeyRole::Sostenuto, -1}, {KEY_MUHENKAN, 35, N, -1},
                {KeysRaw::Space, 110, PcKeyRole::Sustain, -1}, {KEY_HENKAN, 35, N, -1},
                {KEY_KATAKANAHIRAGANA, 35, N, -1}, {KeysRaw::AltGr, 40, PcKeyRole::Sostenuto, -1},
                {KeysRaw::Super_R, 40, PcKeyRole::Soft, -1}, {KeysRaw::Menu, 40, PcKeyRole::TabModifier, -1},
                {KeysRaw::Control_R, 40, PcKeyRole::OctaveUp, -1}};
    }
    return {{KeysRaw::Control_L, 40, PcKeyRole::OctaveDown, -1}, {KeysRaw::Super_L, 40, PcKeyRole::Soft, -1},
            {KeysRaw::Alt, 40, PcKeyRole::Sostenuto, -1}, {KeysRaw::Space, 170, PcKeyRole::Sustain, -1},
            {KeysRaw::AltGr, 40, PcKeyRole::Sostenuto, -1}, {KeysRaw::Super_R, 40, PcKeyRole::Soft, -1},
            {KeysRaw::Menu, 40, PcKeyRole::TabModifier, -1}, {KeysRaw::Control_R, 40, PcKeyRole::OctaveUp, -1}};
}
}

QList<QList<PcKeyDef>> KeyboardLayout::keys(KeyboardForm form)
{
    const PcKeyRole N = PcKeyRole::Note;
    QList<QList<PcKeyDef>> rows;
    rows.append(numberRow(form));

    switch (form)
    {
        case KeyboardForm::ISO:
            rows.append(QList<PcKeyDef>{{15, 40, N, 0}, {16, 30, N, 1}, {17, 30, N, 2}, {18, 30, N, 3}, {19, 30, N, 4},
                         {20, 30, N, 5}, {21, 30, N, 6}, {22, 30, N, 7}, {23, 30, N, 8}, {24, 30, N, 9},
                         {25, 30, N, 10}, {26, 30, N, 11}, {27, 30, N, 12}, {28, 50, N, 13}});
            rows.append(QList<PcKeyDef>{{58, 50, N, 0}, {30, 30, N, 1}, {31, 30, N, 2}, {32, 30, N, 3}, {33, 30, N, 4},
                         {34, 30, N, 5}, {35, 30, N, 6}, {36, 30, N, 7}, {37, 30, N, 8}, {38, 30, N, 9},
                         {39, 30, N, 10}, {40, 30, N, 11}, {43, 70, N, 12}});
            rows.append(QList<PcKeyDef>{{42, 40, N, 0}, {KEY_102ND, 30, N, 1}, {44, 30, N, 2}, {45, 30, N, 3}, {46, 30, N, 4},
                         {47, 30, N, 5}, {48, 30, N, 6}, {49, 30, N, 7}, {50, 30, N, 8}, {51, 30, N, 9},
                         {52, 30, N, 10}, {53, 30, N, 11}, {54, 80, N, 12}});
            break;

        case KeyboardForm::ANSI:
            // backslash ends the Q row, Enter ends the A row, no key next to the wide left Shift
            rows.append(QList<PcKeyDef>{{15, 45, N, 0}, {16, 30, N, 1}, {17, 30, N, 2}, {18, 30, N, 3}, {19, 30, N, 4},
                         {20, 30, N, 5}, {21, 30, N, 6}, {22, 30, N, 7}, {23, 30, N, 8}, {24, 30, N, 9},
                         {25, 30, N, 10}, {26, 30, N, 11}, {27, 30, N, 12}, {43, 45, N, 13}});
            rows.append(QList<PcKeyDef>{{58, 52, N, 0}, {30, 30, N, 1}, {31, 30, N, 2}, {32, 30, N, 3}, {33, 30, N, 4},
                         {34, 30, N, 5}, {35, 30, N, 6}, {36, 30, N, 7}, {37, 30, N, 8}, {38, 30, N, 9},
                         {39, 30, N, 10}, {40, 30, N, 11}, {28, 68, N, 12}});
            rows.append(QList<PcKeyDef>{{42, 68, N, 0}, {44, 30, N, 2}, {45, 30, N, 3}, {46, 30, N, 4}, {47, 30, N, 5},
                         {48, 30, N, 6}, {49, 30, N, 7}, {50, 30, N, 8}, {51, 30, N, 9}, {52, 30, N, 10},
                         {53, 30, N, 11}, {54, 82, N, 12}});
            break;

        case KeyboardForm::JIS:
            rows.append(QList<PcKeyDef>{{15, 45, N, 0}, {16, 30, N, 1}, {17, 30, N, 2}, {18, 30, N, 3}, {19, 30, N, 4},
                         {20, 30, N, 5}, {21, 30, N, 6}, {22, 30, N, 7}, {23, 30, N, 8}, {24, 30, N, 9},
                         {25, 30, N, 10}, {26, 30, N, 11}, {27, 30, N, 12}, {28, 45, N, 13}});
            rows.append(QList<PcKeyDef>{{58, 52, N, 0}, {30, 30, N, 1}, {31, 30, N, 2}, {32, 30, N, 3}, {33, 30, N, 4},
                         {34, 30, N, 5}, {35, 30, N, 6}, {36, 30, N, 7}, {37, 30, N, 8}, {38, 30, N, 9},
                         {39, 30, N, 10}, {40, 30, N, 11}, {43, 68, N, 12}});
            rows.append(QList<PcKeyDef>{{42, 68, N, 0}, {44, 30, N, 2}, {45, 30, N, 3}, {46, 30, N, 4}, {47, 30, N, 5},
                         {48, 30, N, 6}, {49, 30, N, 7}, {50, 30, N, 8}, {51, 30, N, 9}, {52, 30, N, 10},
                         {53, 30, N, 11}, {KEY_RO, 30, N, 12}, {54, 52, N, 13}});
            break;
    }

    rows.append(functionRow(form));
    return rows;
}

QString KeyboardLayout::formName(KeyboardForm form)
{
    switch (form)
    {
        case KeyboardForm::ISO:  return "iso";
        case KeyboardForm::ANSI: return "ansi";
        case KeyboardForm::JIS:  return "jis";
    }
    return "iso";
}

QString KeyboardLayout::formTitle(KeyboardForm form)
{
    switch (form)
    {
        case KeyboardForm::ISO:  return "ISO (Europe)";
        case KeyboardForm::ANSI: return "ANSI (US)";
        case KeyboardForm::JIS:  return "JIS (Japan)";
    }
    return QString();
}

bool KeyboardLayout::formFromName(const QString &name, KeyboardForm *form)
{
    const QList<KeyboardForm> forms = allForms();
    for (const KeyboardForm candidate : forms)
    {
        if (formName(candidate) == name.trimmed().toLower())
        {
            *form = candidate;
            return true;
        }
    }
    return false;
}

QList<KeyboardForm> KeyboardLayout::allForms()
{
    return {KeyboardForm::ISO, KeyboardForm::ANSI, KeyboardForm::JIS};
}

KeyboardForm KeyboardLayout::guessSystemForm()
{
#if defined(Q_OS_MACOS)
    // macOS knows the physical type of the keyboard
    switch (KBGetLayoutType(LMGetKbdType()))
    {
        case kKeyboardJIS:  return KeyboardForm::JIS;
        case kKeyboardANSI: return KeyboardForm::ANSI;
        case kKeyboardISO:  return KeyboardForm::ISO;
        default: break;
    }
#elif defined(Q_OS_WIN)
    // type 7: Japanese keyboard
    if (GetKeyboardType(0) == 7)
        return KeyboardForm::JIS;
#endif

    // otherwise a guess from the language of the keyboard (or of the system)
    QLocale locale = QLocale::system();
    if (QGuiApplication::inputMethod() && QGuiApplication::inputMethod()->locale() != QLocale::c())
        locale = QGuiApplication::inputMethod()->locale();

    if (locale.language() == QLocale::Japanese)
        return KeyboardForm::JIS;

    switch (locale.territory())
    {
        case QLocale::UnitedStates:
        case QLocale::Australia:
        case QLocale::NewZealand:
        case QLocale::Philippines:
        case QLocale::China:
        case QLocale::Taiwan:
        case QLocale::HongKong:
        case QLocale::SouthKorea:
        case QLocale::Thailand:
        case QLocale::Malaysia:
        case QLocale::Singapore:
        case QLocale::Indonesia:
        case QLocale::India:
            return KeyboardForm::ANSI;
        case QLocale::Canada:
            return locale.language() == QLocale::English ? KeyboardForm::ANSI : KeyboardForm::ISO;
        default:
            return KeyboardForm::ISO;
    }
}

bool KeyboardLayout::formFromKeycode(int keycode, KeyboardForm *form)
{
    if (keycode == KEY_102ND)
    {
        *form = KeyboardForm::ISO;
        return true;
    }
    if (keycode == KEY_RO || keycode == KEY_HENKAN || keycode == KEY_KATAKANAHIRAGANA
        || keycode == KEY_MUHENKAN || keycode == KEY_YEN)
    {
        *form = KeyboardForm::JIS;
        return true;
    }
    return false;
}
