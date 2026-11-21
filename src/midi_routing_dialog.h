#ifndef MIDI_ROUTING_DIALOG_H
#define MIDI_ROUTING_DIALOG_H

#include <QWidget>
#include <QComboBox>
#include <QList>
#include <QListWidget>
#include <QMap>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>

#include "preset_model.h"

// what a controller (CC) of an input port does instead of being forwarded to the tabs
enum class ControlFunction
{
    Velocity,   // velocity pedal: the velocity of the PC keys of the section
    Swell       // swell pedal: the volume of the whole section (on top of all other volumes)
};

// one MIDI input port of a keyboard section; the id stays when the port is renamed
struct MidiInputPort
{
    int id = 0;
    QString name;
    // CC number -> function; other controllers go to the tabs of the port
    QMap<int, ControlFunction> controls;

    bool operator==(const MidiInputPort &other) const
    {
        return this->id == other.id && this->name == other.name && this->controls == other.controls;
    }

    static QString functionName(ControlFunction function);      // "velocity", "swell"
    static bool functionFromName(const QString &name, ControlFunction *function);
};

/*
 * MIDI routing of one keyboard section: its input ports (add, rename, remove;
 * the first one always exists) and which port plays which tab, as a matrix:
 * per tab and port "-", "when selected" or "always". Works on a copy; Apply / OK
 * hand it to the section.
 */
class MidiRoutingDialog : public QWidget
{
    Q_OBJECT
public:
    MidiRoutingDialog(int keyboard_id, const QList<MidiInputPort> &ports, const QStringList &tab_titles,
                      const QList<QMap<int, MidiInputMode>> &routes, QWidget *parent = nullptr);

    // "Learn" is on: the next controller of the selected port is taken
    bool isLearning() const;
    // a controller arrived on an input port (MainTabs forwards them while learning)
    void learnControl(int input, int cc);

signals:
    void applied(const QList<MidiInputPort> &ports, const QList<QMap<int, MidiInputMode>> &routes);

private:
    int keyboard_id;
    QList<MidiInputPort> ports;
    QStringList tab_titles;
    QList<QMap<int, MidiInputMode>> routes;

    QListWidget *list_ports;
    QPushButton *button_remove;
    QTableWidget *table;
    QComboBox *combo_fill_port;
    QComboBox *combo_fill_mode;
    QTableWidget *table_controls;
    QPushButton *button_learn;
    QPushButton *button_remove_control;

    void showPorts();
    int selectedPortIndex() const;
    void showControls();
    void readControls();
    void addControl(int cc);
    void removeControl();
    void readPorts();
    void showMatrix();
    void readMatrix();
    void addPort();
    void removePort();
    void fillColumn();
    bool validate();
    void apply();

    static QComboBox *modeCombo();
    static int modeIndex(const QMap<int, MidiInputMode> &route, int port_id);
};

#endif // MIDI_ROUTING_DIALOG_H
