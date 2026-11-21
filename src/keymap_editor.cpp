#include "keymap_editor.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include "keyboard_pc.h"

// ---------------------------------------------------------------------------
// NoteSpinBox

NoteSpinBox::NoteSpinBox(bool allow_silent, QWidget *parent) : QSpinBox(parent)
{
    setRange(allow_silent ? -1 : Keymap::NOTE_MIN, Keymap::NOTE_MAX);
    if (allow_silent)
        setSpecialValueText("silent");
    setAccelerated(true);
}

QString NoteSpinBox::textFromValue(int value) const
{
    return Keymap::noteName(value);
}

int NoteSpinBox::valueFromText(const QString &text) const
{
    int note = minimum();
    Keymap::noteFromName(text, &note);
    return note;
}

QValidator::State NoteSpinBox::validate(QString &text, int &pos) const
{
    Q_UNUSED(pos)
    int note = 0;
    if (Keymap::noteFromName(text, &note))
        return QValidator::Acceptable;
    if (!specialValueText().isEmpty() && text == specialValueText())
        return QValidator::Acceptable;
    // while typing ("C", "C#", "Eb")
    return QValidator::Intermediate;
}

// ---------------------------------------------------------------------------
// KeymapEditor

namespace
{
// the editor shows the keyboard about twice as large, with fonts to match
const char *EDITOR_KEY_STYLE =
    "QPushButton#pc_white { font-size: 15pt; }"
    "QPushButton#pc_black { font-size: 11pt; }"
    "QPushButton#pc_grey, QPushButton#pc_gold, QPushButton#pc_blue { font-size: 10pt; }";
}

KeymapEditor::KeymapEditor(int keyboard_id, const Keymap &keymap, const QString &form_setting,
                           KeyboardForm auto_form, const QString &keymap_dir, QWidget *parent)
    : QWidget(parent, Qt::Window)
{
    this->keyboard_id = keyboard_id;
    this->working = keymap;
    this->auto_form = auto_form;
    this->keymap_dir = keymap_dir;

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QString("Keymap Editor - Keyboard %1").arg(keyboard_id + 1));

    // --- form and pattern ---
    this->combo_form = new QComboBox;
    this->combo_form->addItem("Automatic (now " + KeyboardLayout::formTitle(auto_form) + ")", "auto");
    const QList<KeyboardForm> forms = KeyboardLayout::allForms();
    for (const KeyboardForm form : forms)
        this->combo_form->addItem(KeyboardLayout::formTitle(form), KeyboardLayout::formName(form));
    this->combo_form->setCurrentIndex(qMax(0, this->combo_form->findData(form_setting)));
    this->combo_form->setToolTip("The physical form of this keyboard. Automatic: a guess from the system, "
                                 "corrected as soon as a key is pressed that only one form has.");

    this->combo_pattern = new QComboBox;
    const QList<KeymapPattern> patterns = Keymap::allPatterns();
    for (const KeymapPattern pattern : patterns)
        this->combo_pattern->addItem(Keymap::patternTitle(pattern), Keymap::patternName(pattern));
    this->combo_pattern->setCurrentIndex(qMax(0, this->combo_pattern->findData(Keymap::patternName(keymap.pattern))));
    this->combo_pattern->setToolTip("Fills all keys. Keys set by hand keep their notes.");

    this->spin_start = new NoteSpinBox(false);
    this->spin_start->setValue(keymap.start);
    this->spin_start->setToolTip("The note of the pattern's first key: the key left of 1 "
                                 "(for the piano pattern: Z). C4 is middle C.");

    QPushButton *button_clear = new QPushButton("Clear Keys Set by Hand");
    button_clear->setToolTip("All keys play the pattern again");

    QHBoxLayout *layout_top = new QHBoxLayout;
    layout_top->addWidget(new QLabel("Keyboard form:"));
    layout_top->addWidget(this->combo_form);
    layout_top->addSpacing(20);
    layout_top->addWidget(new QLabel("Pattern:"));
    layout_top->addWidget(this->combo_pattern);
    layout_top->addWidget(new QLabel("First key:"));
    layout_top->addWidget(this->spin_start);
    layout_top->addStretch();
    layout_top->addWidget(button_clear);

    // --- the keyboard ---
    this->keyboard = new KeyboardPC;
    this->keyboard->setStyleSheet(EDITOR_KEY_STYLE);
    this->keyboard->setShowSoundingNotes(false);
    this->keyboard->setScale(1.9);
    this->keyboard->setKeyboardForm(currentForm());
    this->keyboard->setEditMode(true);

    // --- the selected key ---
    this->label_key = new QLabel;
    this->spin_note = new NoteSpinBox(true);
    this->spin_note->setToolTip("The note of the selected key. C4 is middle C (with the default octave box 48).");
    this->button_learn = new QPushButton("Learn from MIDI");
    this->button_learn->setCheckable(true);
    this->button_learn->setToolTip("Play a note on a MIDI keyboard connected to this keyboard section's MIDI input");
    this->button_reset_key = new QPushButton("Back to Pattern");

    QHBoxLayout *layout_key = new QHBoxLayout;
    layout_key->addWidget(this->label_key, 1);
    layout_key->addWidget(new QLabel("Note:"));
    layout_key->addWidget(this->spin_note);
    layout_key->addWidget(this->button_learn);
    layout_key->addWidget(this->button_reset_key);

    QLabel *label_hint = new QLabel("Press a key of this keyboard while this window is active, or click a key. "
                                    "Keys set by hand have an orange corner.");
    label_hint->setWordWrap(true);

    this->check_sounding_notes = new QCheckBox("Show the sounding octaves on the PC keyboards (all keyboards)");
    this->check_sounding_notes->setToolTip("The small line on every key of the main window: the octaves (or notes) "
                                           "it sounds as right now. Also in the View menu.");

    // --- buttons ---
    QPushButton *button_load = new QPushButton("Load...");
    QPushButton *button_save = new QPushButton("Save As...");
    QPushButton *button_cancel = new QPushButton("Cancel");
    QPushButton *button_apply = new QPushButton("Apply");
    QPushButton *button_ok = new QPushButton("OK");
    button_ok->setDefault(true);

    QHBoxLayout *layout_buttons = new QHBoxLayout;
    layout_buttons->addWidget(button_load);
    layout_buttons->addWidget(button_save);
    layout_buttons->addStretch();
    layout_buttons->addWidget(button_cancel);
    layout_buttons->addWidget(button_apply);
    layout_buttons->addWidget(button_ok);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(layout_top);
    layout->addWidget(this->keyboard);
    layout->addLayout(layout_key);
    layout->addWidget(label_hint);
    layout->addWidget(this->check_sounding_notes);
    layout->addLayout(layout_buttons);

    connect(this->combo_form, &QComboBox::currentIndexChanged, this, &KeymapEditor::formChanged);
    connect(this->combo_pattern, &QComboBox::currentIndexChanged, this, &KeymapEditor::patternChanged);
    connect(this->spin_start, &QSpinBox::valueChanged, this, &KeymapEditor::startChanged);
    connect(this->spin_note, &QSpinBox::valueChanged, this, &KeymapEditor::noteChanged);
    connect(this->button_reset_key, &QPushButton::clicked, this, &KeymapEditor::resetKey);
    connect(button_clear, &QPushButton::clicked, this, &KeymapEditor::clearHandSetKeys);
    connect(this->keyboard, &KeyboardPC::keySelected, this, &KeymapEditor::selectKey);
    connect(this->check_sounding_notes, &QCheckBox::toggled, this, &KeymapEditor::showSoundingNotesToggled);
    connect(button_load, &QPushButton::clicked, this, &KeymapEditor::loadFile);
    connect(button_save, &QPushButton::clicked, this, &KeymapEditor::saveFile);
    connect(button_cancel, &QPushButton::clicked, this, &QWidget::close);
    connect(button_apply, &QPushButton::clicked, this, &KeymapEditor::apply);
    connect(button_ok, &QPushButton::clicked, this, [this]{
        apply();
        close();
    });

    showKeymap();
    showSelectedKey();
}

QString KeymapEditor::formSetting() const
{
    return this->combo_form->currentData().toString();
}

KeyboardForm KeymapEditor::currentForm() const
{
    KeyboardForm form = this->auto_form;
    KeyboardLayout::formFromName(formSetting(), &form);
    return form;
}

void KeymapEditor::showKeymap()
{
    this->keyboard->setKeymap(this->working);
    this->keyboard->setSelectedKey(this->selected_keycode);
}

void KeymapEditor::physicalKeyPressed(int keycode)
{
    selectKey(keycode);
}

void KeymapEditor::selectKey(int keycode)
{
    this->selected_keycode = keycode;
    this->keyboard->setSelectedKey(keycode);
    showSelectedKey();
}

void KeymapEditor::showSelectedKey()
{
    const int keycode = this->selected_keycode;
    const bool is_note_key = keycode >= 0 && this->keyboard->isNoteKey(keycode);

    this->spin_note->setEnabled(is_note_key);
    this->button_learn->setEnabled(is_note_key);
    this->button_reset_key->setEnabled(is_note_key && this->working.overrides.contains(keycode));
    if (!is_note_key)
        this->button_learn->setChecked(false);

    if (keycode < 0)
        this->label_key->setText("No key selected");
    else if (!this->keyboard->keycodes().contains(keycode))
        this->label_key->setText(QString("Key %1 is not on this keyboard form").arg(keycode));
    else if (!is_note_key)
        this->label_key->setText(QString("Key %1 has a function (pedal, octave, tab) and plays no note").arg(keycode));
    else
        this->label_key->setText(QString("Key %1: %2").arg(keycode).arg(
            this->working.overrides.contains(keycode) ? "set by hand" : "from the pattern"));

    const QSignalBlocker blocker(this->spin_note);
    this->spin_note->setValue(is_note_key ? this->working.note(keycode, currentForm()) : -1);
}

void KeymapEditor::formChanged()
{
    this->keyboard->setKeyboardForm(currentForm());
    showKeymap();
    showSelectedKey();
}

void KeymapEditor::patternChanged()
{
    KeymapPattern pattern = KeymapPattern::None;
    Keymap::patternFromName(this->combo_pattern->currentData().toString(), &pattern);
    this->working.pattern = pattern;
    this->working.start = Keymap::defaultStart(pattern);
    {
        const QSignalBlocker blocker(this->spin_start);
        this->spin_start->setValue(this->working.start);
    }
    showKeymap();
    showSelectedKey();
}

void KeymapEditor::startChanged(int value)
{
    this->working.start = value;
    showKeymap();
    showSelectedKey();
}

void KeymapEditor::noteChanged(int value)
{
    if (this->selected_keycode < 0 || !this->keyboard->isNoteKey(this->selected_keycode))
        return;
    this->working.overrides.insert(this->selected_keycode, value);
    showKeymap();
    showSelectedKey();
}

void KeymapEditor::resetKey()
{
    this->working.overrides.remove(this->selected_keycode);
    showKeymap();
    showSelectedKey();
}

void KeymapEditor::clearHandSetKeys()
{
    if (this->working.overrides.isEmpty())
        return;
    if (QMessageBox::question(this, "Clear Keys Set by Hand",
                              QString("Let all %1 keys set by hand play the pattern again?").arg(this->working.overrides.size()))
        != QMessageBox::Yes)
        return;
    this->working.overrides.clear();
    showKeymap();
    showSelectedKey();
}

void KeymapEditor::setShowSoundingNotesChecked(bool checked)
{
    const QSignalBlocker blocker(this->check_sounding_notes);
    this->check_sounding_notes->setChecked(checked);
}

bool KeymapEditor::isLearning() const
{
    return this->button_learn->isChecked() && this->button_learn->isEnabled();
}

void KeymapEditor::learnMidiNote(int midi_note)
{
    this->button_learn->setChecked(false);
    const int note = qBound(Keymap::NOTE_MIN, midi_note - Keymap::REFERENCE_OFFSET, Keymap::NOTE_MAX);
    // through the spin box: same path as typing the note
    this->spin_note->setValue(note);
}

void KeymapEditor::loadFile()
{
    const QString path = QFileDialog::getOpenFileName(this, "Load Keymap", this->keymap_dir, "Keymaps (*.json)");
    if (path.isEmpty())
        return;

    Keymap loaded;
    QString error;
    if (!Keymap::loadFile(path, &loaded, &error))
    {
        QMessageBox::warning(this, "Load Keymap", "Could not load " + path + ":\n" + error);
        return;
    }
    this->working = loaded;
    {
        const QSignalBlocker blocker_pattern(this->combo_pattern);
        const QSignalBlocker blocker_start(this->spin_start);
        this->combo_pattern->setCurrentIndex(qMax(0, this->combo_pattern->findData(Keymap::patternName(loaded.pattern))));
        this->spin_start->setValue(loaded.start);
    }
    showKeymap();
    showSelectedKey();
}

void KeymapEditor::saveFile()
{
    QDir().mkpath(this->keymap_dir);
    QString path = QFileDialog::getSaveFileName(this, "Save Keymap", this->keymap_dir, "Keymaps (*.json)");
    if (path.isEmpty())
        return;
    if (!path.endsWith(".json", Qt::CaseInsensitive))
        path += ".json";

    this->working.name = QFileInfo(path).completeBaseName();
    QString error;
    if (!this->working.saveFile(path, &error))
        QMessageBox::warning(this, "Save Keymap", "Could not save " + path + ":\n" + error);
}

void KeymapEditor::apply()
{
    emit applied(this->working, formSetting());
}
