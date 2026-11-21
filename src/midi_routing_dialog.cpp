#include "midi_routing_dialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSet>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{
// combo box entries of a matrix cell
constexpr int MODE_NONE = 0;
constexpr int MODE_WHEN_SELECTED = 1;
constexpr int MODE_ALWAYS = 2;
}

QString MidiInputPort::functionName(ControlFunction function)
{
    return function == ControlFunction::Swell ? "swell" : "velocity";
}

bool MidiInputPort::functionFromName(const QString &name, ControlFunction *function)
{
    if (name == "velocity")
        *function = ControlFunction::Velocity;
    else if (name == "swell")
        *function = ControlFunction::Swell;
    else
        return false;
    return true;
}

MidiRoutingDialog::MidiRoutingDialog(int keyboard_id, const QList<MidiInputPort> &ports, const QStringList &tab_titles,
                                     const QList<QMap<int, MidiInputMode>> &routes, QWidget *parent)
    : QWidget(parent, Qt::Window)
{
    this->keyboard_id = keyboard_id;
    this->ports = ports;
    this->tab_titles = tab_titles;
    this->routes = routes;
    while (this->routes.size() < this->tab_titles.size())
        this->routes.append(QMap<int, MidiInputMode>());

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QString("MIDI Routing - Keyboard %1").arg(keyboard_id + 1));

    // --- ports ---
    this->list_ports = new QListWidget;
    this->list_ports->setMaximumHeight(110);
    this->list_ports->setToolTip("The MIDI input ports of this keyboard as they appear in the patchbay. "
                                 "Double-click to rename.");
    QPushButton *button_add = new QPushButton("Add Port");
    this->button_remove = new QPushButton("Remove Port");
    this->button_remove->setToolTip("The first port always exists");

    QVBoxLayout *layout_port_buttons = new QVBoxLayout;
    layout_port_buttons->addWidget(button_add);
    layout_port_buttons->addWidget(this->button_remove);
    layout_port_buttons->addStretch();
    QHBoxLayout *layout_ports = new QHBoxLayout;
    layout_ports->addWidget(this->list_ports, 1);
    layout_ports->addLayout(layout_port_buttons);

    // --- controller functions of the selected port ---
    this->table_controls = new QTableWidget(0, 2);
    this->table_controls->setHorizontalHeaderLabels({"Controller (CC)", "Function"});
    this->table_controls->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->table_controls->verticalHeader()->hide();
    this->table_controls->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->table_controls->setMaximumHeight(110);
    this->button_learn = new QPushButton("Learn Pedal");
    this->button_learn->setCheckable(true);
    this->button_learn->setToolTip("Move the pedal (or controller) connected to the selected port");
    QPushButton *button_add_control = new QPushButton("Add Controller");
    this->button_remove_control = new QPushButton("Remove Controller");
    QVBoxLayout *layout_control_buttons = new QVBoxLayout;
    layout_control_buttons->addWidget(this->button_learn);
    layout_control_buttons->addWidget(button_add_control);
    layout_control_buttons->addWidget(this->button_remove_control);
    layout_control_buttons->addStretch();
    QHBoxLayout *layout_controls = new QHBoxLayout;
    layout_controls->addWidget(this->table_controls, 1);
    layout_controls->addLayout(layout_control_buttons);

    QLabel *label_controls_help = new QLabel(
        "<b>velocity</b>: the pedal sets the velocity of the notes played on the PC keyboard "
        "(touch dynamics with the foot).<br>"
        "<b>swell</b>: the pedal sets the volume of the whole keyboard, on top of all channel and tab "
        "volumes &mdash; like the swell pedal of an organ.<br>"
        "Controllers with a function are not forwarded to the tabs; all others are.");
    label_controls_help->setWordWrap(true);

    // --- matrix ---
    this->table = new QTableWidget;
    this->table->setSelectionMode(QAbstractItemView::NoSelection);
    this->table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->table->setMinimumHeight(300);

    this->combo_fill_port = new QComboBox;
    this->combo_fill_mode = modeCombo();
    QPushButton *button_fill = new QPushButton("Set for All Tabs");
    QHBoxLayout *layout_fill = new QHBoxLayout;
    layout_fill->addWidget(new QLabel("Port"));
    layout_fill->addWidget(this->combo_fill_port);
    layout_fill->addWidget(this->combo_fill_mode);
    layout_fill->addWidget(button_fill);
    layout_fill->addStretch();

    QLabel *label_help = new QLabel(
        "<b>when selected</b>: the port plays the tab while it sounds (the current tab and the tabs "
        "selected with Menu+F) &mdash; it follows your stops, like the PC keyboard.<br>"
        "<b>always</b>: the port plays the tab whatever is selected &mdash; e.g. a sequencer "
        "on a fixed instrument.<br>"
        "Several tabs on one port sound together.");
    label_help->setWordWrap(true);

    // --- buttons ---
    QPushButton *button_cancel = new QPushButton("Cancel");
    QPushButton *button_apply = new QPushButton("Apply");
    QPushButton *button_ok = new QPushButton("OK");
    button_ok->setDefault(true);
    QHBoxLayout *layout_buttons = new QHBoxLayout;
    layout_buttons->addStretch();
    layout_buttons->addWidget(button_cancel);
    layout_buttons->addWidget(button_apply);
    layout_buttons->addWidget(button_ok);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<b>Input ports</b>"));
    layout->addLayout(layout_ports);
    layout->addWidget(new QLabel("<b>Pedals and controllers of the selected port</b>"));
    layout->addWidget(label_controls_help);
    layout->addLayout(layout_controls);
    layout->addWidget(new QLabel("<b>Which port plays which tab</b>"));
    layout->addWidget(label_help);
    layout->addWidget(this->table, 1);
    layout->addLayout(layout_fill);
    layout->addLayout(layout_buttons);

    connect(button_add, &QPushButton::clicked, this, &MidiRoutingDialog::addPort);
    connect(this->button_remove, &QPushButton::clicked, this, &MidiRoutingDialog::removePort);
    connect(this->list_ports, &QListWidget::currentRowChanged, this, [this](int row){
        this->button_remove->setEnabled(row > 0);
        showControls();
    });
    connect(button_add_control, &QPushButton::clicked, this, [this]{ addControl(-1); });
    connect(this->button_remove_control, &QPushButton::clicked, this, &MidiRoutingDialog::removeControl);
    connect(this->list_ports, &QListWidget::itemChanged, this, [this]{
        // a renamed port: new column header
        readMatrix();
        readPorts();
        showMatrix();
    });
    connect(button_fill, &QPushButton::clicked, this, &MidiRoutingDialog::fillColumn);
    connect(button_cancel, &QPushButton::clicked, this, &QWidget::close);
    connect(button_apply, &QPushButton::clicked, this, &MidiRoutingDialog::apply);
    connect(button_ok, &QPushButton::clicked, this, [this]{
        if (!validate())
            return;
        apply();
        close();
    });

    showPorts();
    this->list_ports->setCurrentRow(0);
    showMatrix();
    showControls();
    resize(720, 760);
}

QComboBox *MidiRoutingDialog::modeCombo()
{
    QComboBox *combo = new QComboBox;
    combo->addItem("–");
    combo->addItem("when selected");
    combo->addItem("always");
    return combo;
}

int MidiRoutingDialog::modeIndex(const QMap<int, MidiInputMode> &route, int port_id)
{
    if (!route.contains(port_id))
        return MODE_NONE;
    return route.value(port_id) == MidiInputMode::Always ? MODE_ALWAYS : MODE_WHEN_SELECTED;
}

void MidiRoutingDialog::showPorts()
{
    const QSignalBlocker blocker(this->list_ports);
    this->list_ports->clear();
    for (const MidiInputPort &port : this->ports)
    {
        QListWidgetItem *item = new QListWidgetItem(port.name);
        item->setData(Qt::UserRole, port.id);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        this->list_ports->addItem(item);
    }
    this->button_remove->setEnabled(this->list_ports->currentRow() > 0);
    showControls();
}

void MidiRoutingDialog::readPorts()
{
    QList<MidiInputPort> result;
    for (int row = 0; row < this->list_ports->count(); ++row)
    {
        MidiInputPort port;
        port.id = this->list_ports->item(row)->data(Qt::UserRole).toInt();
        port.name = this->list_ports->item(row)->text().trimmed();
        // the controls are edited in table_controls (readControls)
        for (const MidiInputPort &old_port : this->ports)
        {
            if (old_port.id == port.id)
                port.controls = old_port.controls;
        }
        result.append(port);
    }
    this->ports = result;
}

int MidiRoutingDialog::selectedPortIndex() const
{
    const int row = this->list_ports->currentRow();
    if (row < 0 || row >= this->ports.size())
        return -1;
    return row;
}

void MidiRoutingDialog::showControls()
{
    // the table shows the controls of the selected port; table_controls carries
    // the port id, so edits always go to the right port
    this->table_controls->setRowCount(0);
    const int index = selectedPortIndex();
    this->table_controls->setProperty("port_id", index >= 0 ? this->ports.at(index).id : -1);
    this->table_controls->setEnabled(index >= 0);
    this->button_learn->setEnabled(index >= 0);
    this->button_remove_control->setEnabled(index >= 0);
    if (index < 0)
        return;

    const QMap<int, ControlFunction> controls = this->ports.at(index).controls;
    for (QMap<int, ControlFunction>::const_iterator it = controls.cbegin(); it != controls.cend(); ++it)
    {
        const int row = this->table_controls->rowCount();
        this->table_controls->insertRow(row);

        QSpinBox *spin_cc = new QSpinBox;
        spin_cc->setRange(0, 127);
        spin_cc->setValue(it.key());
        spin_cc->setToolTip("11: expression pedal, 4: foot controller, 7: volume");
        connect(spin_cc, &QSpinBox::valueChanged, this, &MidiRoutingDialog::readControls);
        this->table_controls->setCellWidget(row, 0, spin_cc);

        QComboBox *combo_function = new QComboBox;
        combo_function->addItem("velocity", MidiInputPort::functionName(ControlFunction::Velocity));
        combo_function->addItem("swell", MidiInputPort::functionName(ControlFunction::Swell));
        combo_function->setCurrentIndex(it.value() == ControlFunction::Swell ? 1 : 0);
        connect(combo_function, &QComboBox::currentIndexChanged, this, &MidiRoutingDialog::readControls);
        this->table_controls->setCellWidget(row, 1, combo_function);
    }
}

void MidiRoutingDialog::readControls()
{
    const int port_id = this->table_controls->property("port_id").toInt();
    for (MidiInputPort &port : this->ports)
    {
        if (port.id != port_id)
            continue;
        port.controls.clear();
        for (int row = 0; row < this->table_controls->rowCount(); ++row)
        {
            QSpinBox *spin_cc = qobject_cast<QSpinBox*>(this->table_controls->cellWidget(row, 0));
            QComboBox *combo_function = qobject_cast<QComboBox*>(this->table_controls->cellWidget(row, 1));
            if (!spin_cc || !combo_function)
                continue;
            ControlFunction function = ControlFunction::Velocity;
            MidiInputPort::functionFromName(combo_function->currentData().toString(), &function);
            port.controls.insert(spin_cc->value(), function);
        }
    }
}

void MidiRoutingDialog::addControl(int cc)
{
    const int index = selectedPortIndex();
    if (index < 0)
        return;
    readControls();
    MidiInputPort &port = this->ports[index];
    if (cc < 0)
    {
        // the first common pedal controller that is still free
        for (const int candidate : {11, 4, 7, 1, 2})
        {
            if (!port.controls.contains(candidate))
            {
                cc = candidate;
                break;
            }
        }
        if (cc < 0)
            return;
    }
    if (!port.controls.contains(cc))
        port.controls.insert(cc, ControlFunction::Swell);
    showControls();
}

void MidiRoutingDialog::removeControl()
{
    const int row = this->table_controls->currentRow();
    if (row < 0)
        return;
    this->table_controls->removeRow(row);
    readControls();
    showControls();
}

bool MidiRoutingDialog::isLearning() const
{
    return this->button_learn->isChecked() && this->button_learn->isEnabled();
}

void MidiRoutingDialog::learnControl(int input, int cc)
{
    const int index = selectedPortIndex();
    // only a pedal on the selected port counts
    if (!isLearning() || index < 0 || this->ports.at(index).id != input)
        return;
    this->button_learn->setChecked(false);
    addControl(cc);
}

void MidiRoutingDialog::showMatrix()
{
    this->table->clear();
    this->table->setRowCount(this->tab_titles.size());
    this->table->setColumnCount(this->ports.size());
    this->table->setVerticalHeaderLabels(this->tab_titles);

    QStringList headers;
    for (const MidiInputPort &port : this->ports)
        headers.append(port.name);
    this->table->setHorizontalHeaderLabels(headers);

    for (int row = 0; row < this->tab_titles.size(); ++row)
    {
        for (int col = 0; col < this->ports.size(); ++col)
        {
            QComboBox *combo = modeCombo();
            combo->setCurrentIndex(modeIndex(this->routes.at(row), this->ports.at(col).id));
            this->table->setCellWidget(row, col, combo);
        }
    }

    const QString fill_port = this->combo_fill_port->currentText();
    this->combo_fill_port->clear();
    this->combo_fill_port->addItems(headers);
    this->combo_fill_port->setCurrentIndex(qMax(0, headers.indexOf(fill_port)));
}

void MidiRoutingDialog::readMatrix()
{
    for (int row = 0; row < this->table->rowCount() && row < this->routes.size(); ++row)
    {
        QMap<int, MidiInputMode> route;
        for (int col = 0; col < this->table->columnCount() && col < this->ports.size(); ++col)
        {
            QComboBox *combo = qobject_cast<QComboBox*>(this->table->cellWidget(row, col));
            if (!combo)
                continue;
            if (combo->currentIndex() == MODE_WHEN_SELECTED)
                route.insert(this->ports.at(col).id, MidiInputMode::WhenSelected);
            else if (combo->currentIndex() == MODE_ALWAYS)
                route.insert(this->ports.at(col).id, MidiInputMode::Always);
        }
        this->routes[row] = route;
    }
}

void MidiRoutingDialog::addPort()
{
    readControls();
    readMatrix();
    readPorts();

    int id = 0;
    QSet<QString> names;
    for (const MidiInputPort &port : this->ports)
    {
        id = qMax(id, port.id + 1);
        names.insert(port.name);
    }
    MidiInputPort port;
    port.id = id;
    int number = this->ports.size() + 1;
    do
    {
        port.name = QString("%1-MIDI-in-%2").arg(this->keyboard_id + 1).arg(number++);
    }
    while (names.contains(port.name));
    this->ports.append(port);

    showPorts();
    showMatrix();
    this->list_ports->setCurrentRow(this->ports.size() - 1);
    this->list_ports->editItem(this->list_ports->currentItem());
}

void MidiRoutingDialog::removePort()
{
    const int row = this->list_ports->currentRow();
    if (row <= 0)
        return;
    readControls();
    readMatrix();
    readPorts();
    const int id = this->ports.at(row).id;
    this->ports.removeAt(row);
    for (QMap<int, MidiInputMode> &route : this->routes)
        route.remove(id);
    showPorts();
    showMatrix();
}

void MidiRoutingDialog::fillColumn()
{
    const int col = this->combo_fill_port->currentIndex();
    if (col < 0 || col >= this->table->columnCount())
        return;
    for (int row = 0; row < this->table->rowCount(); ++row)
    {
        QComboBox *combo = qobject_cast<QComboBox*>(this->table->cellWidget(row, col));
        if (combo)
            combo->setCurrentIndex(this->combo_fill_mode->currentIndex());
    }
}

bool MidiRoutingDialog::validate()
{
    readPorts();
    QSet<QString> names;
    for (const MidiInputPort &port : this->ports)
    {
        if (port.name.isEmpty() || port.name.contains(':'))
        {
            QMessageBox::warning(this, "MIDI Routing", "Port names must not be empty or contain \":\".");
            return false;
        }
        if (names.contains(port.name))
        {
            QMessageBox::warning(this, "MIDI Routing", "Two ports are called \"" + port.name + "\".");
            return false;
        }
        names.insert(port.name);
    }
    return true;
}

void MidiRoutingDialog::apply()
{
    readControls();
    if (!validate())
        return;
    readMatrix();
    emit applied(this->ports, this->routes);
}
