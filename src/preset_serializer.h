#ifndef PRESET_SERIALIZER_H
#define PRESET_SERIALIZER_H

#include <QString>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QSettings>

#include "preset_model.h"

/*
 * Converts TabPreset from/to the existing representations:
 *
 *  - the GUI maps (channel maps + Orgelwerk::getParams())
 *  - the "restore data" emitted by Config::restoreParams
 *    (keys "0".."15" and "main", each holding a QMap<QString,QVariant>)
 *  - the INI layout of the save files:
 *      <keyboard>/<tab-label>/<000..015>/<key>
 *      <keyboard>/<tab-label>/main/<key>
 */
class PresetSerializer
{
public:
    static TabPreset fromGuiMaps(const QList<QMap<QString, QVariant>> &channels, const QMap<QString, QVariant> &main_params, bool resend_midi_auto);

    static TabPreset fromRestoreData(const QMap<QString, QVariant> &data);
    static QMap<QString, QVariant> toRestoreData(const TabPreset &preset);
    
    // one map per channel, same keys as the INI channel groups (e.g. for MIDIChannelsSummary)
    static QList<QMap<QString, QVariant>> toChannelMaps(const TabPreset &preset, bool only_activated);

    static void save(QSettings *settings, int keyboard_id, const QString &label, const TabPreset &preset);
    static TabPreset load(QSettings *settings, int keyboard_id, const QString &label);

private:
    static ChannelSettings channelFromMap(const QMap<QString, QVariant> &map, const ChannelSettings &defaults, int channel, QList<CCSetting> &cc_out);
    static QMap<QString, QVariant> channelToMap(const ChannelSettings &settings, int channel, const QList<CCSetting> &cc_entries, int &cc_index);

    static void mainFromMap(const QMap<QString, QVariant> &map, TabPreset &preset);
    static QMap<QString, QVariant> mainToMap(const TabPreset &preset);

    static int intValue(const QMap<QString, QVariant> &map, const QString &key, int fallback);
    static QString pad3(int n);
};

#endif // PRESET_SERIALIZER_H
