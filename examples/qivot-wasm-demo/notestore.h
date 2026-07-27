#ifndef NOTESTORE_H
#define NOTESTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>        // QML_ELEMENT
#include <qilistmodel.h>

/// Exposes a *live* list of notes to QML. `notes` re-runs its query whenever the
/// note table changes, so adding/removing updates the view with no manual reload.
class NoteStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *notes READ notes CONSTANT)
public:
    explicit NoteStore(QObject *parent = nullptr);
    QAbstractItemModel *notes() { return &m_notes; }

    Q_INVOKABLE void add(const QString &text);   // insert a note
    Q_INVOKABLE void remove(int id);             // delete one
    Q_INVOKABLE void clear();                    // delete all

private:
    QiListModel m_notes;
};

#endif // NOTESTORE_H
