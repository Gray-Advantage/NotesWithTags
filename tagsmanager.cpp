#include "tagsmanager.h"
#include "settingsmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QDir>

TagsManager::TagsManager()
{
    load();
}

QStringList TagsManager::allTags() const
{
    return QStringList(tags.begin(), tags.end());
}

void TagsManager::addTags(const QStringList &newTags)
{
    tags.unite(QSet<QString>(newTags.begin(), newTags.end()));
}

void TagsManager::clearTags()
{
    tags.clear();
    save();
}

void TagsManager::save()
{
    QString saveLocation = SettingsManager::instance().saveLocation();
    if (saveLocation.isEmpty()) {
        qDebug() << "Save location is empty, cannot save tags";
        return;
    }

    QDir dir(saveLocation);
    if (!dir.exists()) {
        dir.mkpath(saveLocation);
    }

    QFile file(dir.filePath("tags.json"));
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open tags.json for writing";
        return;
    }

    QJsonArray tagsArray;
    for (const QString &tag : tags) {
        tagsArray.append(tag);
    }

    QJsonDocument doc(tagsArray);
    file.write(doc.toJson());
    file.close();
}

void TagsManager::load()
{
    QString saveLocation = SettingsManager::instance().saveLocation();
    if (saveLocation.isEmpty()) {
        qDebug() << "Save location is empty, cannot load tags";
        return;
    }

    QFile file(QDir(saveLocation).filePath("tags.json"));
    if (!file.exists()) return;

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open tags.json for reading";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isArray()) {
        QJsonArray tagsArray = doc.array();
        for (const QJsonValue &value : tagsArray) {
            tags.insert(value.toString());
        }
    }
}
