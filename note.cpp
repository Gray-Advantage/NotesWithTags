#include "note.h"
#include "settingsmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QDebug>

Note::Note(const QString &title, const QString &text, const QStringList &tags)
    : m_title(title), m_text(text), m_tags(tags)
{
}

QString Note::title() const
{
    return m_title;
}

QString Note::text() const
{
    return m_text;
}

QStringList Note::tags() const
{
    return m_tags;
}

bool Note::save() const
{
    QString saveLocation = SettingsManager::instance().saveLocation();
    if (saveLocation.isEmpty()) {
        qDebug() << "Save location is empty";
        return false;
    }

    QDir dir(saveLocation);
    if (!dir.exists()) {
        dir.mkpath(saveLocation);
    }

    QString fileName = QString("%1.json").arg(m_title);
    QFile file(dir.filePath(fileName));
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open" << fileName << "for writing";
        return false;
    }

    QJsonObject obj;
    obj["title"] = m_title;
    obj["text"] = m_text;
    QJsonArray tagsArray;
    for (const QString &tag : m_tags) {
        tagsArray.append(tag);
    }
    obj["tags"] = tagsArray;

    QJsonDocument doc(obj);
    file.write(doc.toJson());
    file.close();
    return true;
}

bool Note::remove() const
{
    QString saveLocation = SettingsManager::instance().saveLocation();
    if (saveLocation.isEmpty()) {
        return false;
    }

    QString fileName = QString("%1.json").arg(m_title);
    QFile file(QDir(saveLocation).filePath(fileName));
    return file.exists() && file.remove();
}
