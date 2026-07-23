#ifndef CONTACTSTORE_H
#define CONTACTSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>
#include <qilistmodel.h>
#include "contact.h"

/// The controller behind the iOS-style contacts screen.
/**
  Backs the list with a **windowed** model: it knows the full 10,000-row count
  up front (one `count(*)`) but only ever fetches the pages you scroll to, so the
  whole table never lives in memory. The A–Z jump is a `count(*)` too (rows that
  sort before a letter = its offset), so it works without loading everything.
  Search re-queries; adding a contact just save()s and the list re-counts.
 */
class ContactStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *contacts READ contacts CONSTANT)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit ContactStore(QObject *parent = nullptr);

    QAbstractItemModel *contacts();
    QString filter() const;
    void    setFilter(const QString &text);
    int     count() const;

    /// Add a contact (reactively appears in its section).
    Q_INVOKABLE void add(const QString &firstName, const QString &lastName,
                         const QString &phone);

    /// Delete by row id.
    Q_INVOKABLE void remove(int id);

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

private:
    QiQuery<Contact> buildBase() const;   // ordered + current filter
    void rebuild();                       // re-point the window at buildBase()

    QiWindowedListModel m_model;
    QString             m_filter;
    int                 m_hookId = -1;
};

#endif // CONTACTSTORE_H
