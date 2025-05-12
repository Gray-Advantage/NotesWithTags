#ifndef NOTEMANAGER_H
#define NOTEMANAGER_H

#include "note.h"
#include <QList>

class NoteManager
{
public:
    NoteManager();
    QList<Note> searchNotes(const QString &query) const;

private:
    QString getSaveLocation() const;
};

#endif // NOTEMANAGER_H
