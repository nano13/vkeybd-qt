#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QSettings>
#include <QGuiApplication>

#include <QDebug>

#include "enums_structs.h"

class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject *parent = nullptr);
    
    // One-time move of the data folder used under the old program name
    // (e.g. ~/.local/share/vkeybd-qt) to the current one; stored paths that
    // point into the old folder are rewritten. Does nothing once the new
    // folder exists. Call after QApplication::setApplicationName().
    static void migrateLegacyDataDir();
    
    //void setValue(QString key, QVariant value);
    void loadChannelSettings(QSettings *settings);
    void saveParams(QSettings *settings, int id, QString label, QString channel, QMap<QString,QVariant> params);
    
    // nullptr if no autoload file is configured or it does not exist
    QSettings *openAutoloadFile();
    // for writing: the returned settings are cleared
    QSettings *openQuicksaveFile(int number_of_keyboards);
    QSettings *openSaveFile(QString filepath);
    // for reading: nothing is cleared
    QSettings *openQuickloadFile(int number_of_keyboards);
    QSettings *openLoadFile(QString filepath);
    void saveLastSavePath(QString path);
    QString getLastSavePath();
    
    int getNumberOfKeyboards();
    void setNumberOfKeyboards(int number);
    
    int getNumberOfLayers();
    void setNumberOfLayers(int number);
    
    NotifyBackend getNotifyBackend();
    
    // the small line on the PC keyboard keys with the notes that sound
    bool getPCKeyboardSoundingNotes();
    void setPCKeyboardSoundingNotes(bool show);
    
    QString getOutputSystem();
    void setOutputSystem(QString output);
    
    QString getKeyboardConfig();
    
    // the folder of config.ini (keyboard states, keymaps)
    QString dataDir() const;
    void setKeyboardConfig(QString file);
    
private:
    QDir *config_dir;
    
    // this is the main config.ini
    QSettings *config;
    // this is the file we use to save/restore our saved presets
    //QSettings *settings;
    QString quicksavePath(int number_of_keyboards);
    QString pad3(int n);
    QString pad3(const QString &str);
    
signals:
    void restoreParams(int maintab, QString tab, QMap<QString, QVariant> data);
    void restoreGeneral(int maintab, QMap<QString, QVariant> data);
};

#endif // CONFIG_H
