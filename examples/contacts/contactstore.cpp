#include "contactstore.h"
#include "contact.h"
#include <qiabstractmodel.h>

ContactStore::ContactStore(QObject *parent) : QObject(parent) {
    rebuild();                                   // count + first page
    connect(&m_model, &QiWindowedListModel::countChanged,
            this, &ContactStore::countChanged);

    // Reactive: when the contact table changes anywhere, re-count / re-window
    // so adds and removes show up. (Coalesced by QiConnection's hook dispatch.)
    m_hookId = QiConnection::defaultConnection().addChangeHook(
        [this](const QString &table) {
            if (table == Contact::TableName())
                m_model.refresh();
        });
}

QAbstractItemModel *ContactStore::contacts() { return &m_model; }
QString ContactStore::filter() const { return m_filter; }
int ContactStore::count() const { return m_model.count(); }

QiQuery<Contact> ContactStore::buildBase() const {
    QiQuery<Contact> q = Contact::objects();
    const QString needle = m_filter.trimmed();
    if (!needle.isEmpty()) {
        const QString like = "%" + needle + "%";
        q = q.filter( Contact::col().lastName.expr("like", like)
                   || Contact::col().firstName.expr("like", like) );
    }
    return q.orderBy(QStringList() << "lastName asc" << "firstName asc");
}

void ContactStore::rebuild() {
    m_model.setQuery<Contact>(buildBase(), 60);  // 60 rows per fetched page
    m_model.refresh();                           // count(*) + load page 1
}

void ContactStore::setFilter(const QString &text) {
    if (text == m_filter) return;
    m_filter = text;
    emit filterChanged();
    rebuild();                                   // re-window with the new filter
    emit countChanged();
}

void ContactStore::add(const QString &firstName, const QString &lastName,
                       const QString &phone) {
    if (firstName.trimmed().isEmpty() && lastName.trimmed().isEmpty())
        return;
    Contact c;
    c.firstName = firstName.trimmed();
    c.lastName  = lastName.trimmed();
    c.phone     = phone.trimmed();
    c.save();                                    // change hook re-windows the list
}

void ContactStore::remove(int id) {
    Contact c;
    if (c.load(Contact::col().id == id))
        c.remove();
}

int ContactStore::indexForLetter(const QString &letter) const {
    if (letter.isEmpty()) return -1;
    const QChar target = letter.at(0).toUpper();
    if (target == QChar('#')) return 0;          // non-letters sort first

    // The first row of a letter sits at offset == (# of rows that sort before it).
    // That's a count(*), so the jump never loads the whole table.
    QiWhere where = Contact::col().lastName.expr("<", QString(target));
    const QString needle = m_filter.trimmed();
    if (!needle.isEmpty()) {
        const QString like = "%" + needle + "%";
        where = where && ( Contact::col().lastName.expr("like", like)
                        || Contact::col().firstName.expr("like", like) );
    }
    int offset = Contact::objects().filter(where).count();
    const int total = m_model.count();
    if (total <= 0) return -1;
    if (offset >= total) offset = total - 1;
    return offset;
}

QString ContactStore::sectionForIndex(int row) const {
    const QString ln = m_model.valueAt(row, "lastName").toString();
    if (ln.isEmpty()) return QString();
    const QChar c = ln.at(0).toUpper();
    return c.isLetter() ? QString(c) : QStringLiteral("#");
}

QString ContactStore::nameForIndex(int row) const {
    const QString first = m_model.valueAt(row, "firstName").toString();
    const QString last  = m_model.valueAt(row, "lastName").toString();
    return (first + " " + last).trimmed();
}
