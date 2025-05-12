#ifndef TAGSMANAGER_H
#define TAGSMANAGER_H

#include <QStringList>
#include <QSet>

class TagsManager
{
public:
    TagsManager();
    QStringList allTags() const;
    void addTags(const QStringList &tags);
    void clearTags();
    void save();
    void load();

private:
    QSet<QString> tags;
};

#endif // TAGSMANAGER_H
