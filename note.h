#ifndef NOTE_H
#define NOTE_H

#include <QString>
#include <QStringList>

class Note
{
public:
    Note(const QString &title, const QString &text, const QStringList &tags);
    QString title() const;
    QString text() const;
    QStringList tags() const;
    bool save() const;
    bool remove() const;

private:
    QString m_title;
    QString m_text;
    QStringList m_tags;
};

#endif // NOTE_H
