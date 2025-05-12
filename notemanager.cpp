#include "notemanager.h"
#include "settingsmanager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

NoteManager::NoteManager()
{
}

QString NoteManager::getSaveLocation() const
{
    return SettingsManager::instance().saveLocation();
}

QList<Note> NoteManager::searchNotes(const QString &query) const
{
    QList<Note> results;
    QString saveLocation = getSaveLocation();
    if (saveLocation.isEmpty()) {
        qDebug() << "Save location is empty";
        return results;
    }

    QDir dir(saveLocation);
    dir.setNameFilters(QStringList() << "*.json");
    QString lowerQuery = query.toLower();

    for (const QString &fileName : dir.entryList(QDir::Files)) {
        QFile file(dir.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open" << fileName;
            continue;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!doc.isObject()) {
            qDebug() << "Invalid JSON in" << fileName;
            continue;
        }

        QJsonObject obj = doc.object();
        QString title = obj["title"].toString();
        QString text = obj["text"].toString();
        QStringList tags;
        QJsonArray tagsArray = obj["tags"].toArray();
        for (const QJsonValue &tag : tagsArray) {
            tags << tag.toString();
        }

        bool matches = title.toLower().contains(lowerQuery) ||
                       text.toLower().contains(lowerQuery) ||
                       tags.contains(query, Qt::CaseInsensitive);

        if (matches) {
            results.append(Note(title, text, tags));
        }
    }

    return results;
}
