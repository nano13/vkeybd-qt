#ifndef MAINTABS_H
#define MAINTABS_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QSettings>
#include <QKeyEvent>

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QCoreApplication>
#include <QScrollArea>

#include "orgelwerk.h"
#include "config.h"
#include "main_tabs_switcher.h"
#include "enums_structs.h"

#include "input_keyboard_select.h"

#include "interface_audio.h"
#include "interface_alsa.h"
#include "interface_jack.h"
#ifdef VKEYBD_HAS_LIBREMIDI
#include "interface_libremidi.h"
#endif

#include "midi_engine.h"
#include "midi_channel_selector.h"
#include "midi_mastervolume.h"

#include <QHash>
#include <QPointer>
#include <QSet>

#include "keymap.h"
#include "keymap_editor.h"
#include "midi_routing_dialog.h"

/*
 * One keyboard instance: its MIDI engine, its tabs (presets, one MIDI port each)
 * and one view (Orgelwerk) that shows the current tab.
 *
 * Tabs are not widgets any more: the "current index" only selects which preset
 * the view is bound to and which tab the keys are played on.
 */
class MainTabs : public QWidget
{
    Q_OBJECT
public:
    explicit MainTabs(QList<QString> labels, int id, Config* config, MainTabsSwitcher* switcher, OutputSystem output, InputKeyboardSelect *input_keyboard_select, QLineEdit *line_udp_ip, QSpinBox *spin_port, QWidget *parent = nullptr);

    int count() const;
    int currentIndex() const;
    void setCurrentIndex(int index);

    void saveParams(QSettings *settings);
    void restoreParams(QString tab, QMap<QString,QVariant> data);

    void showHideGUIElements(GUIElements elements, bool show);

    void rawKeyPressed(int keycode);
    void rawKeyReleased(int keycode);
    void MIDISignal(MIDISignalTypes type);

    // the global key shift, the same absolute value for all sections (MainWindow)
    void setGlobalKeyShift(int value);
    // the key shift of the current tab (without the global key shift)
    int globalKeyShiftGet();
    // label of the current tab, e.g. "F03-1"
    QString currentLabel() const;
    void globalResendMIDISettings();

    void listOfCheckTabsChanged(QList<int> list_of_tab_ids);

    // lets an open channel editor follow the current tab
    void syncChannelEditorToCurrentTab();
    
    // the keymap editor of this keyboard (one window, raised if open already)
    void openKeymapEditor();
    // the MIDI input ports of this keyboard and which tabs they play
    void openMidiRouting();
    // the sounding-notes line of the PC keyboard (a global setting, see MainWindow)
    void setShowSoundingNotes(bool show);

private:
    InterfaceAudio *interface_audio = nullptr;
    MidiEngine *midi_engine = nullptr;
    Orgelwerk *view = nullptr;

    int current_index = 0;

    // one channel editor for all tabs of this keyboard, created on first use
    MIDIChannelSelector *channel_editor = nullptr;
    QScrollArea *scroll_channel_editor = nullptr;
    void showChannelEditor(int tab_id);
    void bindChannelEditor(int tab_id);
    void channelEditorClosed();

    QList<int> list_function_keys_raw;
    QList<QString> list_labels;

    // we want to allow multiple tabs to be checked at once. they all shall sound simultaneously.
    QList<int> list_of_checked_tabs;
    // the current tab plus all additionally checked ones
    QList<int> soundingTabs() const;
    
    // Group DCA: shown below the tab rows while more than one tab sounds.
    // Scales the volumes of all sounding tabs by the same factor, so their
    // ratios (= dB differences) stay. The volume hotkeys drive it then.
    QWidget *group_dca = nullptr;
    MIDIMasterVolume *group_volume = nullptr;
    // volume of each sounding tab at DCA 100 %
    QHash<int, double> group_anchors;
    int group_dca_value = 100;
    void updateGroupDCA();
    void groupDCAMoved(int value);
    void updateGroupDCALabel();

    int id;
    Config *config;
    MainTabsSwitcher *main_tabs_switcher;
    QLineEdit *line_udp_ip;
    QSpinBox *spin_port;

    QUdpSocket *socket;
    static constexpr int MAX_UDP_MESSAGE_SIZE = 64;
    void rebindSocketIP(QString ip);
    void rebindSocket(int value);
    bool send_udp = false;
    void sendUDPMessage(QString message);
    void receiveUDPMessage();

    void initializeTabs();

    // note / pitch events of the view
    void routeMIDIEvent(int value, MIDIMode mode);
    
    // the MIDI input of this keyboard (omni)
    // input: the id of the input port (see midi_input_ports)
    void midiInputNoteOn(int input, int note, int velocity);
    void midiInputNoteOff(int input, int note);
    void midiInputControlChange(int input, int cc, int value);
    void midiInputPitchBend(int input, int value);

    bool is_escape_pressed = false;
    bool is_menu_pressed = false;
    // Esc was pressed while Menu was held: Menu + Esc (reset) unless Esc + F-key switched the row
    bool escape_with_menu = false;
    bool escape_used_for_row = false;
    bool is_space_pressed = false;
    
    // What the keys play and the form of the keyboard, kept in
    // <data dir>/keyboard-<n>.json. Form setting "auto": the guess of the
    // system, or the form a key proved (detected_form).
    Keymap keymap;
    QString keyboard_form_setting = "auto";
    QString detected_form;
    QString keyboardStatePath() const;
    void loadKeyboardState();
    void saveKeyboardState();
    void applyKeyboardState();
    KeyboardForm autoKeyboardForm() const;
    void keymapApplied(const Keymap &keymap, const QString &form_setting);
    
    // while the editor window is active, the keys of this keyboard select keys there
    QPointer<KeymapEditor> keymap_editor;
    
    // MIDI input ports of this section (stored with the keyboard state; the first
    // one always exists). Which tabs a port plays is part of each tab's preset.
    QList<MidiInputPort> midi_input_ports;
    QPointer<MidiRoutingDialog> midi_routing_dialog;
    QString defaultMidiInputName() const;
    void openMidiInputPorts();
    // the tabs an input plays right now: "always" tabs, and "when selected" tabs that sound
    QList<int> tabsForInput(int input) const;
    QString tabTitle(int tab) const;
    void midiRoutingApplied(const QList<MidiInputPort> &ports, const QList<QMap<int, MidiInputMode>> &routes);
    
    // pedals on the input ports (controller functions)
    // velocity pedal: the velocity of the PC keys; -1: none (the channel settings)
    int pc_velocity = -1;
    // swell pedal: collected and applied at most every SWELL_INTERVAL_MS, as a
    // pedal sends many values and every value re-sends the volumes of the section
    static constexpr int SWELL_INTERVAL_MS = 15;
    int pending_swell = -1;
    void applySwell();
    // returns true if the controller has a function on this input (and was used)
    bool controlFunction(int input, int cc, int value);
    // tabs played "always" by an input get a mark in the tab switcher
    void updateFixedInputMarks();
    QSet<int> keys_to_editor;

private slots:
    void deviceNotAvailable(QString message);

signals:
    void currentChanged(int index);
    void signalKeyShiftChanged(int value, bool is_relative);
    // tab or key shift of this section changed (for the status notification)
    void signalStatusChanged();
    // the checkbox in the keymap editor was toggled (applies to all keyboards)
    void signalShowSoundingNotes(bool show);
};

#endif // MAINTABS_H
