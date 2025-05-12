#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QString>
#include <QJsonObject>

class SettingsManager
{
public:
    static SettingsManager& instance();
    void load();
    void save();

    QString theme() const;
    void setTheme(const QString &theme);

    QString syncUrl() const;
    void setSyncUrl(const QString &url);

    QString login() const;
    void setLogin(const QString &login);

    QString password() const;
    void setPassword(const QString &password);

    QString saveLocation() const;
    void setSaveLocation(const QString &path);

private:
    SettingsManager();
    QString settingsPath() const;

    QJsonObject settings;
};

#endif // SETTINGSMANAGER_H
