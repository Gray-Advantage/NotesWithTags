#include "tagsmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDebug>

TagsManager::TagsManager() {
    load();
}

QString TagsManager::tagsFilePath() const {
    return QDir::currentPath() + "/tags.json";
}

void TagsManager::load() {
    QFile file(tagsFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray arr = doc.array();
    for (const auto &v : arr)
        tags << v.toString();

    file.close();
}

QStringList TagsManager::allTags() const {
    return tags;
}

void TagsManager::addTags(const QStringList &newTags) {
    for (const QString &tag : newTags) {
        if (!tags.contains(tag))
            tags << tag;
    }
}

void TagsManager::save() {
    QFile file(tagsFilePath());
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonArray arr;
    for (const QString &tag : tags)
        arr.append(tag);

    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    file.close();
}
