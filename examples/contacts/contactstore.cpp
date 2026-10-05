#include "contactstore.h"
#include "contact.h"
#include <QLocale>
#include <QRandomGenerator>
#include <QTimer>
#include <qiabstractmodel.h>

namespace {

const QStringList kFirsts = { "Ada", "Grace", "Alan", "Linus", "Margaret", "Dennis",
                              "Barbara", "Ken", "Frances", "Tim", "Hedy", "Edsger" };
const QStringList kLasts  = { "Abbott", "Bishop", "Carver", "Dalton", "Ellison", "Fischer",
                              "Garner", "Holt", "Ingram", "Jensen", "Keller", "Lambert",
                              "Mercer", "Novak", "Osborne", "Pryor", "Quinn", "Rhodes",
                              "Sutton", "Tate", "Underwood", "Vance", "Whitaker", "Young" };

QString randomPhone() {
    QRandomGenerator *r = QRandomGenerator::global();
    return QString("(%1) %2-%3").arg(r->bounded(200, 999)).arg(r->bounded(100, 999))
                                .arg(r->bounded(1000, 9999));
}

QString rowText(int row) { return QLocale().toString(row + 1); }

} // namespace

ContactStore::ContactStore(QObject *parent) : QObject(parent) {
    rebuild();                                   // the ids, and nothing else
    connect(&m_model, &QiLiveListModel::countChanged, this, &ContactStore::countChanged);

    // Say what each change did to the list. The model emits only these row
    // signals: there is no reset to watch for, because there isn't one.
    connect(&m_model, &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex &, int first, int last) {
        note(first == last ? QString("Inserted at row %1").arg(rowText(first))
                           : QString("Inserted %1 rows at %2").arg(last - first + 1).arg(rowText(first)));
    });
    connect(&m_model, &QAbstractItemModel::rowsRemoved, this,
            [this](const QModelIndex &, int first, int last) {
        note(first == last ? QString("Removed row %1").arg(rowText(first))
                           : QString("Removed %1 rows at %2").arg(last - first + 1).arg(rowText(first)));
    });
    connect(&m_model, &QAbstractItemModel::rowsMoved, this,
            [this](const QModelIndex &, int start, int, const QModelIndex &, int dest) {
        const int to = dest > start ? dest - 1 : dest;    // Qt counts dest before the move
        note(QString("Moved row %1 → %2").arg(rowText(start), rowText(to)));
    });
    connect(&m_model, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &from, const QModelIndex &to) {
        if (m_events.size() > 1)
            return;                              // the row that just moved or arrived: said already
        note(from.row() == to.row() ? QString("Refreshed row %1").arg(rowText(from.row()))
                                    : QString("Refreshed the rows on screen"));
    });
    connect(&m_model, &QAbstractItemModel::modelReset, this, [this] { note("Reloaded the list"); });

    // The moment a write happens, before the model works out what it means:
    // the queries from here to the row signal are what the change cost.
    m_hookId = QiConnection::defaultConnection().addRowChangeHook([this](const QiChange &) {
        if (!m_reportPending && m_events.isEmpty())
            m_queriesBefore = m_model.queryCount();
    });
}

ContactStore::~ContactStore() {
    QiConnection::defaultConnection().removeChangeHook(m_hookId);
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
    m_model.setQuery<Contact>(buildBase(), 60);  // 60 records per batch
}

void ContactStore::setFilter(const QString &text) {
    if (text == m_filter) return;
    m_filter = text;
    emit filterChanged();
    rebuild();                                   // a new list: read its ids
    m_activity = m_filter.trimmed().isEmpty()
        ? QString()
        : QString("%1 matches · 1 query for the ids").arg(QLocale().toString(m_model.count()));
    emit activityChanged();
}

void ContactStore::note(const QString &event) {
    if (m_events.isEmpty()) {
        // Everything the model ran before its first signal placed the change.
        m_events << QString::number(m_model.queryCount() - m_queriesBefore);
    }
    m_events << event;
    if (m_reportPending)
        return;
    m_reportPending = true;
    QTimer::singleShot(0, this, [this] { report(); });   // after the view has redrawn
}

void ContactStore::report() {
    m_reportPending = false;
    if (m_events.size() < 2)
        return;
    const int placing = m_events.takeFirst().toInt();
    const int reading = m_model.queryCount() - m_queriesBefore - placing;

    QString cost = placing == 0 ? QString("no query")
                 : placing == 1 ? QString("1 query") : QString("%1 queries").arg(placing);
    if (reading > 0)
        cost += reading == 1 ? QString(" + 1 to read the row") : QString(" + %1 to read rows").arg(reading);

    m_activity = QString("%1 · %2\n%3 · %4 of %5 in memory")
                     .arg(m_source, m_events.join(", "), cost,
                          QLocale().toString(m_model.cachedRecords()),
                          QLocale().toString(m_model.count()));
    m_events.clear();
    m_source = QStringLiteral("You");
    m_queriesBefore = m_model.queryCount();
    emit activityChanged();
}

void ContactStore::add(const QString &firstName, const QString &lastName,
                       const QString &phone) {
    if (firstName.trimmed().isEmpty() && lastName.trimmed().isEmpty())
        return;
    Contact c;
    c.firstName = firstName.trimmed();
    c.lastName  = lastName.trimmed();
    c.phone     = phone.trimmed();
    c.save();                                    // the list inserts the one row
}

void ContactStore::update(int id, const QString &firstName, const QString &lastName,
                          const QString &phone) {
    Contact c;
    if (!c.load(Contact::col().id == id))
        return;
    c.firstName = firstName.trimmed();
    c.lastName  = lastName.trimmed();
    c.phone     = phone.trimmed();
    c.save();                                    // the list moves or refreshes the one row
}

void ContactStore::remove(int id) {
    Contact c;
    if (c.load(Contact::col().id == id))
        c.remove();                              // the list removes the one row
}

int ContactStore::rowOf(int id) const {
    return m_model.indexOf(id);
}

void ContactStore::simulateChange(int fromRow, int toRow) {
    const int n = m_model.count();
    if (n == 0) return;
    fromRow = qBound(0, fromRow, n - 1);
    toRow   = qBound(fromRow, toRow, n - 1);
    QRandomGenerator *r = QRandomGenerator::global();
    const int row = fromRow + r->bounded(toRow - fromRow + 1);
    const int dice = r->bounded(100);
    simulate(dice < 40 ? "phone" : dice < 70 ? "rename" : dice < 88 ? "relative" : "remove", row);
}

void ContactStore::simulate(const QString &kind, int row) {
    Contact c;
    if (!c.load(Contact::col().id == m_model.idAt(row)))
        return;
    QRandomGenerator *r = QRandomGenerator::global();
    m_source = QStringLiteral("Sync");
    if (kind == "phone") {                       // a new number: refreshed in place
        c.phone = randomPhone();
        c.save();
    } else if (kind == "rename") {               // a new surname: the row moves away
        c.lastName = kLasts.at(r->bounded(kLasts.size()));
        c.save();
    } else if (kind == "relative") {             // a relative: appears right here
        // A first name with the same initial, so they sort side by side
        const QString first = c.firstName.get().toString();
        QStringList near;
        for (const QString &name : kFirsts) {
            if (!first.isEmpty() && name.startsWith(first.at(0)) && name != first)
                near << name;
        }
        if (near.isEmpty())
            near = kFirsts;
        Contact relative;
        relative.firstName = near.at(r->bounded(near.size()));
        relative.lastName  = c.lastName.get().toString();
        relative.phone     = randomPhone();
        relative.save();
    } else if (kind == "remove") {               // gone
        c.remove();
    }
}

QVariantMap ContactStore::contactAt(int row) const {
    QVariantMap c;
    if (row < 0 || row >= m_model.count())
        return c;
    c["id"]        = m_model.idAt(row);
    c["firstName"] = m_model.valueAt(row, "firstName");
    c["lastName"]  = m_model.valueAt(row, "lastName");
    c["phone"]     = m_model.valueAt(row, "phone");
    return c;
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
