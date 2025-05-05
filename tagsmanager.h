// tagsmanager.h
#ifndef TAGSMANAGER_H
#define TAGSMANAGER_H

#include <QStringList>

class TagsManager {
public:
    TagsManager();

    QStringList allTags() const;
    void addTags(const QStringList &tags);
    void save();

private:
    void load();
    QString tagsFilePath() const;

    QStringList tags;
};

#endif // TAGSMANAGER_H
