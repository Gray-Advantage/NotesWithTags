#include "settingsmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>

SettingsManager::SettingsManager() {
    load();
}

SettingsManager &SettingsManager::instance() {
    static SettingsManager instance;
    return instance;
}

QString SettingsManager::settingsPath() const {
    return QDir::currentPath() + "/settings.json";
}

void SettingsManager::load() {
    QFile file(settingsPath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isObject())
        settings = doc.object();
    file.close();
}

void SettingsManager::save() {
    QFile file(settingsPath());
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonDocument doc(settings);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

QString SettingsManager::theme() const {
    return settings.value("theme").toString("light");
}

void SettingsManager::setTheme(const QString &theme) {
    settings["theme"] = theme;
    save();
}

QString SettingsManager::syncUrl() const {
    return settings.value("sync_url").toString();
}

void SettingsManager::setSyncUrl(const QString &url) {
    settings["sync_url"] = url;
    save();
}

QString SettingsManager::login() const {
    return settings.value("login").toString();
}

void SettingsManager::setLogin(const QString &login) {
    settings["login"] = login;
    save();
}

QString SettingsManager::password() const {
    return settings.value("password").toString();
}

void SettingsManager::setPassword(const QString &password) {
    settings["password"] = password;
    save();
}

QString SettingsManager::saveLocation() const {
    return settings.value("save_location").toString(QDir::currentPath());
}

void SettingsManager::setSaveLocation(const QString &path) {
    settings["save_location"] = path;
    save();
}
