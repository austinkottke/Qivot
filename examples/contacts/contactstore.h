#ifndef CONTACTSTORE_H
#define CONTACTSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>
#include <QStringList>
#include <qilistmodel.h>
#include "contact.h"

/// The controller behind the iOS-style contacts screen.
/**
  Backs the list with a **live** model, QiLiveListModel: it reads the 10,000
  ids once, then the records a batch at a time for the rows on screen. When a
  contact is saved or removed, only that row changes: a rename moves the row,
  an edit refreshes it, a new contact slides in at its place. The list is never
  reloaded, so the view keeps its delegates and its scroll position.

  `activity` says what the last change did and what it cost, so you can watch
  it happen; `simulateChange()` makes changes the way another device syncing
  would.
 */
class ContactStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *contacts READ contacts CONSTANT)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString activity READ activity NOTIFY activityChanged)
    Q_PROPERTY(int cached READ cached NOTIFY activityChanged)
public:
    explicit ContactStore(QObject *parent = nullptr);
    ~ContactStore() override;

    QAbstractItemModel *contacts();
    QString filter() const;
    void    setFilter(const QString &text);
    int     count() const;

    /// What the last change did to the list, and the queries it took
    QString activity() const { return m_activity; }

    /// How many contacts are in memory (the rest are only ids)
    int cached() const { return m_model.cachedRecords(); }

    /// Add a contact: it appears in its section.
    Q_INVOKABLE void add(const QString &firstName, const QString &lastName,
                         const QString &phone);

    /// Edit a contact: its row is refreshed, or moves if the name changed.
    Q_INVOKABLE void update(int id, const QString &firstName, const QString &lastName,
                            const QString &phone);

    /// Delete by row id.
    Q_INVOKABLE void remove(int id);

    /// The row a contact is on now, or -1.
    Q_INVOKABLE int rowOf(int id) const;

    /// Make one change among rows `fromRow`..`toRow` (the ones on screen), as
    /// if another device had synced it: a new phone number, a rename, a new
    /// contact, or a deletion.
    Q_INVOKABLE void simulateChange(int fromRow, int toRow);

    /// One particular synced change to the contact at `row`: "phone", "rename",
    /// "relative" (a new contact with the same surname) or "remove".
    Q_INVOKABLE void simulate(const QString &kind, int row);

    /// The contact at `row`: { id, firstName, lastName, phone }
    Q_INVOKABLE QVariantMap contactAt(int row) const;

    /// First row index whose last name starts with `letter` (for the A–Z index).
    Q_INVOKABLE int indexForLetter(const QString &letter) const;

    /// Section letter (uppercased first initial of last name) for a row —
    /// drives the rail highlight while scrolling.
    Q_INVOKABLE QString sectionForIndex(int row) const;

    /// "First Last" for a row — drives the "current position" HUD so you can
    /// tell exactly which contact is at the top as you scroll.
    Q_INVOKABLE QString nameForIndex(int row) const;

signals:
    void filterChanged();
    void countChanged();
    void activityChanged();

private:
    QiQuery<Contact> buildBase() const;   // ordered + current filter
    void rebuild();                       // point the model at buildBase()
    void note(const QString &event);      // collect what the view was told
    void report();                        // ...and say it, once per change

    QiLiveListModel m_model;
    QString         m_filter;
    QString         m_activity;
    QString         m_source = QStringLiteral("You");   // who made the change: "You" or "Sync"
    QStringList     m_events;
    bool            m_reportPending = false;
    int             m_queriesBefore = 0;  // the model's query count when the write happened
    int             m_hookId = -1;
};

#endif // CONTACTSTORE_H
