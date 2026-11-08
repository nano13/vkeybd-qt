#include "preset_serializer.h"

TabPreset PresetSerializer::fromGuiMaps(const QList<QMap<QString, QVariant>> &channels, const QMap<QString, QVariant> &main_params, bool resend_midi_auto)
{
    TabPreset preset;
    QList<CCSetting> cc_entries;

    for (int i = 0; i < channels.length(); i++)
    {
        const QMap<QString, QVariant> &map = channels.at(i);
        int channel = intValue(map, "channel", i);
        if (channel < 0 || channel >= TabPreset::NUMBER_OF_CHANNELS)
            continue;

        preset.channels[channel] = channelFromMap(map, preset.channels[channel], channel, cc_entries);
    }
    preset.cc_entries = cc_entries;

    mainFromMap(main_params, preset);
    preset.resend_midi_auto = resend_midi_auto;

    return preset;
}

TabPreset PresetSerializer::fromRestoreData(const QMap<QString, QVariant> &data)
{
    TabPreset preset;
    QList<CCSetting> cc_entries;

    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        QString key = QString::number(c);
        if (data.contains(key))
        {
            preset.channels[c] = channelFromMap(data.value(key).toMap(), preset.channels[c], c, cc_entries);
        }
    }
    preset.cc_entries = cc_entries;

    if (data.contains("main"))
    {
        mainFromMap(data.value("main").toMap(), preset);
    }

    return preset;
}

QMap<QString, QVariant> PresetSerializer::toRestoreData(const TabPreset &preset)
{
    QMap<QString, QVariant> result;

    int cc_index = 0;
    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        result[QString::number(c)] = channelToMap(preset.channels.at(c), c, preset.cc_entries, cc_index);
    }
    result["main"] = mainToMap(preset);

    return result;
}

QList<QMap<QString, QVariant>> PresetSerializer::toChannelMaps(const TabPreset &preset, bool only_activated)
{
    QList<QMap<QString, QVariant>> result;
    
    int cc_index = 0;
    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        QMap<QString, QVariant> map = channelToMap(preset.channels.at(c), c, preset.cc_entries, cc_index);
        if (!only_activated || preset.channels.at(c).activated)
        {
            result.append(map);
        }
    }
    
    return result;
}

void PresetSerializer::save(QSettings *settings, int keyboard_id, const QString &label, const TabPreset &preset)
{
    if (!settings)
        return;

    QString base = QString::number(keyboard_id) + "/" + label + "/";

    int cc_index = 0;
    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        QMap<QString, QVariant> map = channelToMap(preset.channels.at(c), c, preset.cc_entries, cc_index);
        QList<QString> keys = map.keys();
        for (const QString &key : keys)
        {
            settings->setValue(base + pad3(c) + "/" + key, map.value(key));
        }
    }

    QMap<QString, QVariant> main = mainToMap(preset);
    QList<QString> keys = main.keys();
    for (const QString &key : keys)
    {
        settings->setValue(base + "main/" + key, main.value(key));
    }
}

TabPreset PresetSerializer::load(QSettings *settings, int keyboard_id, const QString &label)
{
    if (!settings)
        return TabPreset();

    QMap<QString, QVariant> data;

    settings->beginGroup(QString::number(keyboard_id) + "/" + label);
    QList<QString> groups = settings->childGroups();
    for (const QString &group : groups)
    {
        settings->beginGroup(group);
        QList<QString> keys = settings->childKeys();
        QMap<QString, QVariant> values;
        for (const QString &key : keys)
        {
            values[key] = settings->value(key);
        }
        settings->endGroup();

        // "000" -> "0", "main" stays "main" (same normalisation as Config::loadChannelSettings)
        bool ok = false;
        int number = group.toInt(&ok);
        data[ok ? QString::number(number) : group] = values;
    }
    settings->endGroup();

    return fromRestoreData(data);
}

ChannelSettings PresetSerializer::channelFromMap(const QMap<QString, QVariant> &map, const ChannelSettings &defaults, int channel, QList<CCSetting> &cc_out)
{
    ChannelSettings result = defaults;

    if (map.contains("activated"))
        result.activated = map.value("activated").toBool();

    result.volume = intValue(map, "volume", result.volume);
    result.pan = intValue(map, "pan", result.pan);
    result.key_shift = intValue(map, "key_shift", result.key_shift);
    result.key_min = intValue(map, "key_min", result.key_min);
    result.key_max = intValue(map, "key_max", result.key_max);
    result.instrument_msb = intValue(map, "instrument_msb", result.instrument_msb);
    result.instrument_lsb = intValue(map, "instrument_lsb", result.instrument_lsb);
    if (map.contains("instrument_bank"))
        result.instrument_bank = map.value("instrument_bank").toString();
    if (map.contains("instrument_name"))
        result.instrument_name = map.value("instrument_name").toString();
    result.velocity = intValue(map, "velocity", result.velocity);
    result.pitch = intValue(map, "pitch", result.pitch);
    result.portamento_time = intValue(map, "portamento_time", result.portamento_time);
    result.attack = intValue(map, "attack", result.attack);
    result.release = intValue(map, "release", result.release);
    result.tremolo = intValue(map, "tremolo", result.tremolo);

    // CC entries: cc_<NNN>_<field>, grouped by NNN, kept in NNN order
    QMap<int, CCSetting> cc_by_id;
    QList<QString> keys = map.keys();
    for (const QString &key : keys)
    {
        if (!key.startsWith("cc_"))
            continue;

        bool ok = false;
        int id = key.section('_', 1, 1).toInt(&ok);
        if (!ok)
            continue;

        QString field = key.section('_', 2);
        QVariant value = map.value(key);

        CCSetting &cc = cc_by_id[id];
        cc.channel = channel;

        if (field == "active")
            cc.active = value.toBool();     // stored as "true" / "false"
        else if (field == "label")
            cc.label = value.toString();
        else if (field == "key")
            cc.cc = value.toInt();
        else if (field == "value")
            cc.value = value.toInt();
        else if (field == "delay")
            cc.delay_ms = value.toInt();
    }

    QList<int> ids = cc_by_id.keys();
    for (int id : ids)
    {
        cc_out.append(cc_by_id.value(id));
    }

    return result;
}

QMap<QString, QVariant> PresetSerializer::channelToMap(const ChannelSettings &settings, int channel, const QList<CCSetting> &cc_entries, int &cc_index)
{
    QMap<QString, QVariant> map;

    map["channel"] = channel;
    map["activated"] = settings.activated;
    map["volume"] = settings.volume;
    map["pan"] = settings.pan;
    map["key_shift"] = settings.key_shift;
    map["key_min"] = settings.key_min;
    map["key_max"] = settings.key_max;
    map["instrument_msb"] = settings.instrument_msb;
    map["instrument_lsb"] = settings.instrument_lsb;
    map["instrument_bank"] = settings.instrument_bank;
    map["instrument_name"] = settings.instrument_name;
    map["velocity"] = settings.velocity;
    map["pitch"] = settings.pitch;
    map["portamento_time"] = settings.portamento_time;
    map["attack"] = settings.attack;
    map["release"] = settings.release;
    map["tremolo"] = settings.tremolo;

    // same value types as the old MIDIChannelSelector::listOfChannels
    for (const CCSetting &cc : cc_entries)
    {
        if (cc.channel != channel)
            continue;

        QString prefix = "cc_" + pad3(cc_index) + "_";
        map[prefix + "active"] = cc.active ? "true" : "false";
        map[prefix + "label"] = cc.label;
        map[prefix + "key"] = QString::number(cc.cc);
        map[prefix + "value"] = cc.value;
        map[prefix + "delay"] = cc.delay_ms;

        cc_index++;
    }

    return map;
}

void PresetSerializer::mainFromMap(const QMap<QString, QVariant> &map, TabPreset &preset)
{
    preset.master_key_shift = intValue(map, "key_shift", preset.master_key_shift);
    preset.master_volume = intValue(map, "volume", preset.master_volume);
    if (map.contains("keys"))
        preset.octave_mask = map.value("keys").toString();
    preset.pitchwheel_tether = intValue(map, "pitchwheel_tether", preset.pitchwheel_tether);
    preset.pitchwheel_pitch = intValue(map, "pitchwheel_pitch", preset.pitchwheel_pitch);
    if (map.contains("resend_midi_auto"))
        preset.resend_midi_auto = map.value("resend_midi_auto").toBool();
}

QMap<QString, QVariant> PresetSerializer::mainToMap(const TabPreset &preset)
{
    QMap<QString, QVariant> map;

    map["key_shift"] = preset.master_key_shift;
    map["volume"] = preset.master_volume;
    map["keys"] = preset.octave_mask;
    map["pitchwheel_tether"] = preset.pitchwheel_tether;
    map["pitchwheel_pitch"] = preset.pitchwheel_pitch;
    map["resend_midi_auto"] = preset.resend_midi_auto;

    return map;
}

int PresetSerializer::intValue(const QMap<QString, QVariant> &map, const QString &key, int fallback)
{
    if (!map.contains(key))
        return fallback;

    bool ok = false;
    int value = map.value(key).toInt(&ok);

    return ok ? value : fallback;
}

QString PresetSerializer::pad3(int n)
{
    return QString("%1").arg(n, 3, 10, QLatin1Char('0'));
}
