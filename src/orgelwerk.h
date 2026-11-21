#ifndef ORGELWERK_H
#define ORGELWERK_H

#include <QObject>
#include <QWidget>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QHash>
#include <QList>
#include <QSet>

#include <functional>

#include "midi_key_selector.h"
#include "midi_pitchwheel.h"
#include "midi_channels_summary.h"
#include "midi_key_shift_widget.h"
#include "midi_mastervolume.h"
#include "keyboard_piano.h"
#include "keyboard_pc.h"
#include "keyboard_notes.h"

#include "enums_structs.h"

#include "preset_model.h"
#include "preset_serializer.h"
#include "midi_engine.h"

/*
 * The view of one keyboard instance. There is one per keyboard, it shows the
 * tab it is bound to (bindTab()). All settings live in the MidiEngine: the
 * widgets here are loaded from it on binding and write every change back.
 *
 * Note events (from the PC keyboard widget) are only emitted with
 * signalMIDIEvent(); MainTabs decides which tabs they are played on.
 */
class Orgelwerk : public QWidget
{
    Q_OBJECT
public:
    explicit Orgelwerk(int keyboard_id, MidiEngine *engine, QWidget *parent = nullptr);

    void bindTab(int tab);
    int boundTab() const;
    // the tabs that sound together with the bound tab (MainTabs: current + checked).
    // The octave hotkeys and the pedals apply to all of them; the widgets of
    // this view (volume slider, key shift buttons) only to the bound tab.
    // The volume hotkeys drive MainTabs' group DCA instead.
    void setSoundingTabs(std::function<QList<int>()> provider);
    // the set of sounding tabs has changed (MainTabs: multiselect)
    void soundingTabsChanged();
    // config.ini: pc-keyboard-sounding-notes
    // the global key shift (Print / Pause) changed: the key shift shown is tab + global
    void refreshKeyShift();
    void setShowSoundingNotes(bool show);
    // shows the bound tab's volume after it was changed elsewhere (group DCA)
    void refreshVolume();
    // shows the engine's current settings of the bound tab (e.g. after a global key shift)
    void reloadFromEngine();
    // writes the pitch wheel position (which is not written back continuously) to the engine
    void storeToEngine();

    void keyDownRaw(int keycode);
    void keyUpRaw(int keycode);
    // resets the note / piano displays (the PC keyboard shows the physical keys and stays)
    void resetNoteDisplays();

    void keySustain(bool pressed);
    void keySostenuto(bool pressed);
    void keySoft(bool pressed);
    void octaveDown();
    void octaveUp();

    void showHideGUIElements(GUIElements elements, bool show);
    
    // the PC keyboard display (keymap, keyboard form)
    KeyboardPC *pcKeyboard() const;

    QPushButton *button_panic = new QPushButton;
    QPushButton *button_stop_all = new QPushButton;
    QPushButton *button_channels_dialog = new QPushButton;
    QPushButton *button_resend_midi = new QPushButton;
    QCheckBox *check_resend_midi_auto = new QCheckBox("auto");

    MIDIPitchWheel *pitch;
    MIDIMasterVolume *volume;

public slots:
    void resendMIDIControls();
    void channelsSummaryUpdate();

private:
    int keyboard_id;
    int tab_id = -1;
    MidiEngine *engine;

    // true while the widgets are filled from the engine: nothing is written back
    bool loading = false;
    
    std::function<QList<int>()> sounding_tabs;
    // the bound tab first, then the other sounding tabs
    QList<int> soundingTabs() const;
    
    // Octave hotkeys: the key shift of the other sounding tabs follows the bound
    // tab as offset. Unclamped values, so a change that hits the limit on one
    // tab returns exactly to where it was.
    QHash<int, int> follow_key_shift;
    void updateKeyShiftLabel();
    void followKeyShift(int delta);
    // true while an octave hotkey changes the key shift (not the buttons of the view)
    bool key_shift_from_hotkey = false;
    
    // tabs a pedal was pressed on: the release goes to them even if the
    // checked tabs changed in between
    QSet<int> sustain_tabs;
    QSet<int> sostenuto_tabs;
    QSet<int> soft_tabs;
    QList<int> pedalTabs(bool pressed, QSet<int> &held) const;

    // the PC keyboard shows what each key sounds as on the sounding tabs;
    // changes are collected and shown once per event loop pass
    bool sounding_notes_pending = false;
    void scheduleSoundingNotes();
    void updateSoundingNotes();

    // Track each source separately so physical keys and MIDI input can overlap.
    QHash<int, int> displayed_source_notes;
    QHash<int, int> displayed_note_counts;

    void drawGUI();
    void drawNotesKeyboard(int grid_row);
    void drawPianoKeyboard(int grid_row);
    void drawPCKeyboard(int grid_row);
    void showChannelsSummary(int grid_row);

    // writes the settings shown here (except the pitch wheel position) to the engine
    void pushMain();
    QMap<QString, QVariant> getParams();

    QGridLayout *grid;
    QGroupBox *group_keys;
    QGroupBox *group_pitch;

    MIDIKeySelector *keys;
    MIDIKeyShiftWidget *key_shift_master;
    QLabel *label_key_shift_master;
    MIDIChannelsSummary *midi_channels_summary;
    KeyboardNotes *notes = nullptr;
    KeyboardPiano *piano = nullptr;
    KeyboardPC *pc = nullptr;

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void panicPressed();
    void stopAllPressed();

    void keyShiftChanged(int value);
    void volumeSliderMoved(int value);
    void pitchWheelMoved(int pitch);

    void engineNoteOn(int tab, int note);
    void engineNoteOff(int tab, int note);
    void engineSourceKey(int tab, int source_key, bool pressed);
    void enginePresetReplaced(int tab);

signals:
    void eventFiltered(QObject *obj, QEvent *ev);
    // a key of the PC keyboard widget, or the pitch wheel (mode PitchBend, value = position)
    void signalMIDIEvent(int value, MIDIMode mode);
    // "Edit Channels" pressed
    void signalEditChannels(int tab_id);
};

#endif // ORGELWERK_H
