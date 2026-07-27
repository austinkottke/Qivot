#include "notestore.h"
#include "note.h"

#include <QDateTime>

static const QStringList kColors = {
    "#FDE68A", "#BBF7D0", "#BFDBFE", "#FBCFE8", "#DDD6FE", "#FED7AA", "#A7F3D0", "#FECACA" };

NoteStore::NoteStore(QObject *parent) : QObject(parent) {
    m_notes.setParent(this);
    // Live model: re-run this query whenever the `note` table changes.
    m_notes.setLive<Note>(QiConnection::defaultConnection(),
                          [] { return QiQuery<Note>().orderBy("id desc").all(); });
}

void NoteStore::add(const QString &text) {
    if (text.trimmed().isEmpty()) return;
    Note n;
    n.text = text.trimmed();
    n.color = kColors.at(Note::objects().count() % kColors.size());
    n.createdAt = QDateTime::currentDateTime().toString("MMM d · h:mm ap");
    n.save();                       // fires the change hook → the live model refreshes
}

void NoteStore::remove(int id) {
    (void) QiQuery<Note>().filter(QiWhere("id = ", id)).remove();
}

void NoteStore::clear() {
    (void) QiQuery<Note>().filter(QiWhere("id > ", 0)).remove();
}
