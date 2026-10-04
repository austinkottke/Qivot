#include <QCoreApplication>
#include <QJsonArray>
#include <QLocale>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QTimeZone>
#include <QUuid>
#include <cmath>
#include "qivalidation.h"
#include "qiconnection.h"
#include "qimodelmetainfo.h"
#include "qisql.h"

// ============================================================================
// QiRule
// ============================================================================

QiRule::QiRule(const QString &name_, const QString &message, Check check_)
    : name(name_), text(message), check(std::move(check_)) {
}

QiRule QiRule::message(const QString &t) const { QiRule r = *this; r.text = t; return r; }
QiRule QiRule::warning() const { QiRule r = *this; r.severity = Warning; return r; }
QiRule QiRule::on(const QStringList &c) const { QiRule r = *this; r.contexts = c; return r; }
QiRule QiRule::when(Condition c) const {
    QiRule r = *this;
    const Condition before = r.condition;
    r.condition = before ? Condition([before, c](const QiRuleContext &x) { return before(x) && c(x); }) : c;
    return r;
}
QiRule QiRule::inZone(const QByteArray &zone) const { QiRule r = *this; r.params.insert(QStringLiteral("zone"), QString::fromLatin1(zone)); return r; }
QiRule QiRule::enforced() const { QiRule r = *this; r.enforce = true; return r; }
QiRule QiRule::checkingNull() const { QiRule r = *this; r.skipNull = false; return r; }

// ============================================================================
// QiRuleContext
// ============================================================================

QVariant QiRuleContext::value() const { return info ? info->value(model, field) : QVariant(); }
QVariant QiRuleContext::valueOf(const QString &f) const { return info ? info->value(model, f) : QVariant(); }
void QiRuleContext::setValue(const QVariant &v) const { if (info) info->setValue(model, field, v); }
QString QiRuleContext::table() const { return info ? info->name() : QString(); }
QString QiRuleContext::idColumn() const {
    if (!info)
        return QStringLiteral("id");
    const QString pk = info->primaryKeyName();
    return pk.isEmpty() ? QStringLiteral("id") : pk;
}
QVariant QiRuleContext::id() const { return info ? info->value(model, idColumn()) : QVariant(); }

// ============================================================================
// QiValidation
// ============================================================================

bool QiValidation::isValid() const {
    for (const QiFieldError &e : m_items)
        if (!e.warning) return false;
    return true;
}
bool QiValidation::hasWarnings() const {
    for (const QiFieldError &e : m_items)
        if (e.warning) return true;
    return false;
}
QList<QiFieldError> QiValidation::errors() const {
    QList<QiFieldError> out;
    for (const QiFieldError &e : m_items) if (!e.warning) out << e;
    return out;
}
QList<QiFieldError> QiValidation::warnings() const {
    QList<QiFieldError> out;
    for (const QiFieldError &e : m_items) if (e.warning) out << e;
    return out;
}
QString QiValidation::error(const QString &field) const {
    for (const QiFieldError &e : m_items) if (!e.warning && e.field == field) return e.message;
    return QString();
}
QStringList QiValidation::errorsFor(const QString &field) const {
    QStringList out;
    for (const QiFieldError &e : m_items) if (!e.warning && e.field == field) out << e.message;
    return out;
}
QString QiValidation::warning(const QString &field) const {
    for (const QiFieldError &e : m_items) if (e.warning && e.field == field) return e.message;
    return QString();
}
bool QiValidation::hasError(const QString &field) const { return !error(field).isEmpty(); }
QStringList QiValidation::fields() const {
    QStringList out;
    for (const QiFieldError &e : m_items)
        if (!e.warning && !out.contains(e.field)) out << e.field;
    return out;
}
void QiValidation::add(const QiFieldError &e) { m_items << e; }
void QiValidation::addError(const QString &field, const QString &message, const QString &rule) {
    m_items << QiFieldError{ field, rule, message, false };
}
void QiValidation::addWarning(const QString &field, const QString &message, const QString &rule) {
    m_items << QiFieldError{ field, rule, message, true };
}
void QiValidation::merge(const QiValidation &other, const QString &prefix) {
    for (QiFieldError e : other.m_items) {
        e.field = prefix + e.field;
        m_items << e;
    }
    for (auto it = other.m_labels.constBegin(); it != other.m_labels.constEnd(); ++it)
        m_labels.insert(prefix + it.key(), it.value());
}
void QiValidation::clear(const QString &field) {
    for (int i = m_items.size() - 1; i >= 0; --i) {
        const QString &f = m_items.at(i).field;
        if (f == field || f.startsWith(field + QLatin1Char('.')) || f.startsWith(field + QLatin1Char('[')))
            m_items.removeAt(i);
    }
}
QString QiValidation::summary() const {
    QStringList lines;
    for (const QiFieldError &e : m_items) {
        if (e.warning) continue;
        if (e.field.isEmpty()) { lines << e.message; continue; }
        // "lines[2].quantity": the last part's label, with where it is.
        const QString label = m_labels.value(e.field, qiHumanize(e.field.section(QLatin1Char('.'), -1)));
        const QString where = e.field.contains(QLatin1Char('.')) ? e.field.section(QLatin1Char('.'), 0, -2) + QLatin1Char(' ') : QString();
        lines << where + label + QStringLiteral(": ") + e.message;
    }
    return lines.join(QLatin1Char('\n'));
}
QVariantMap QiValidation::errorMap() const {
    QVariantMap m;
    for (const QiFieldError &e : m_items)
        if (!e.warning && !m.contains(e.field)) m.insert(e.field, e.message);
    return m;
}
QVariantMap QiValidation::warningMap() const {
    QVariantMap m;
    for (const QiFieldError &e : m_items)
        if (e.warning && !m.contains(e.field)) m.insert(e.field, e.message);
    return m;
}
QJsonObject QiValidation::toJson() const {
    QJsonObject errs, warns;
    for (const QiFieldError &e : m_items) {
        QJsonObject &target = e.warning ? warns : errs;
        QJsonArray list = target.value(e.field).toArray();
        list << e.message;
        target.insert(e.field, list);
    }
    return QJsonObject{ { "valid", isValid() }, { "errors", errs }, { "warnings", warns } };
}

QString qiHumanize(const QString &field) {
    QString out;
    for (int i = 0; i < field.size(); ++i) {
        const QChar c = field.at(i);
        if (c == QLatin1Char('_') || c == QLatin1Char('-')) { out += QLatin1Char(' '); continue; }
        if (c.isUpper() && i > 0 && field.at(i - 1).isLower()) out += QLatin1Char(' ');   // camelCase
        out += out.isEmpty() ? c.toUpper() : c.toLower();
    }
    out = out.simplified();
    if (out.endsWith(QLatin1String(" id")))           // author_id -> Author
        out.chop(3);
    return out;
}

// ============================================================================
// Dates
// ============================================================================

namespace {

QTimeZone qivZone(const QByteArray &id) {
    if (!id.isEmpty()) {
        QTimeZone z(id);
        if (z.isValid()) return z;
    }
    return QTimeZone::systemTimeZone();
}

QDateTime qivNow(const QByteArray &zone) {
    return QDateTime::currentDateTime().toTimeZone(qivZone(zone));
}

QDate qivToday(const QByteArray &zone) {
    return qivNow(zone).date();
}

QByteArray qivZoneOf(const QVariantMap &p) {
    return p.value(QStringLiteral("zone")).toString().toLatin1();
}

} // namespace

QDateTime qiDateTimeOf(const QVariant &value, const QByteArray &zoneId) {
    if (value.isNull())
        return QDateTime();
    const QTimeZone zone = qivZone(zoneId);
    switch (value.userType()) {
    case QMetaType::QDate:     return value.toDate().isValid() ? QDateTime(value.toDate(), QTime(0, 0), zone) : QDateTime();
    case QMetaType::QDateTime: return value.toDateTime();
    case QMetaType::QTime:     return value.toTime().isValid() ? QDateTime(qivToday(zoneId), value.toTime(), zone) : QDateTime();
    default: break;
    }
    QString s = value.toString().trimmed();
    if (s.isEmpty())
        return QDateTime();
    static const QRegularExpression qivSpacedIso(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}) (\\d)"));
    s.replace(qivSpacedIso, QStringLiteral("\\1T\\2"));            // "2026-10-03 12:00" -> ISO
    QDateTime dt = QDateTime::fromString(s, Qt::ISODateWithMs);
    if (dt.isValid() && s.contains(QLatin1Char('T')))
        return dt.timeSpec() == Qt::LocalTime ? QDateTime(dt.date(), dt.time(), zone) : dt;
    const QDate d = QDate::fromString(s, Qt::ISODate);
    if (d.isValid() && s.size() == 10)
        return QDateTime(d, QTime(0, 0), zone);
    const QTime t = QTime::fromString(s, Qt::ISODateWithMs);
    if (t.isValid() && !s.contains(QLatin1Char('-')))
        return QDateTime(qivToday(zoneId), t, zone);
    return QDateTime();
}

QDateTime qiResolveDate(const QVariant &spec, const QByteArray &zoneId) {
    if (spec.userType() != QMetaType::QString)
        return qiDateTimeOf(spec, zoneId);
    const QString s = spec.toString().trimmed().toLower();
    static const QRegularExpression qivRelative(QStringLiteral("^(now|today|tomorrow|yesterday)?\\s*((?:[+-]\\s*\\d+\\s*(?:s|sec|min|h|d|w|m|mo|y)\\s*)*)$"));
    const QRegularExpressionMatch m = qivRelative.match(s);
    if (!m.hasMatch() || (m.captured(1).isEmpty() && m.captured(2).trimmed().isEmpty()))
        return qiDateTimeOf(spec, zoneId);

    const QString base = m.captured(1);
    static const QRegularExpression qivRelativePart(QStringLiteral("([+-])\\s*(\\d+)\\s*(min|sec|mo|s|h|d|w|m|y)"));
    bool timeUnits = false;
    QRegularExpressionMatchIterator it = qivRelativePart.globalMatch(m.captured(2));
    QList<QRegularExpressionMatch> parts;
    while (it.hasNext()) {
        parts << it.next();
        const QString u = parts.last().captured(3);
        timeUnits = timeUnits || u == QLatin1String("s") || u == QLatin1String("sec") || u == QLatin1String("min") || u == QLatin1String("h");
    }
    const QTimeZone zone = qivZone(zoneId);
    QDateTime at;
    if (base == QLatin1String("now") || (base.isEmpty() && timeUnits))
        at = qivNow(zoneId);
    else
        at = QDateTime(qivToday(zoneId), QTime(0, 0), zone);
    if (base == QLatin1String("tomorrow")) at = at.addDays(1);
    if (base == QLatin1String("yesterday")) at = at.addDays(-1);
    for (const QRegularExpressionMatch &p : parts) {
        const int n = (p.captured(1) == QLatin1String("-") ? -1 : 1) * p.captured(2).toInt();
        const QString u = p.captured(3);
        if (u == QLatin1String("s") || u == QLatin1String("sec")) at = at.addSecs(n);
        else if (u == QLatin1String("min")) at = at.addSecs(qint64(n) * 60);
        else if (u == QLatin1String("h")) at = at.addSecs(qint64(n) * 3600);
        else if (u == QLatin1String("d")) at = at.addDays(n);
        else if (u == QLatin1String("w")) at = at.addDays(qint64(n) * 7);
        else if (u == QLatin1String("m") || u == QLatin1String("mo")) at = at.addMonths(n);
        else if (u == QLatin1String("y")) at = at.addYears(n);
    }
    return at;
}

// ============================================================================
// Rule helpers
// ============================================================================

namespace {

bool qivBlank(const QVariant &v) {
    if (v.isNull() || !v.isValid())
        return true;
    if (v.userType() == QMetaType::QString)
        return v.toString().trimmed().isEmpty();
    return false;
}

QString qivText(const QVariant &v) {
    switch (v.userType()) {
    case QMetaType::QDate:     return v.toDate().toString(Qt::ISODate);
    case QMetaType::QDateTime: {
        const QDateTime dt = v.toDateTime();
        return dt.time() == QTime(0, 0) ? dt.date().toString(Qt::ISODate) : dt.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    }
    case QMetaType::QTime:     return v.toTime().toString(QStringLiteral("HH:mm"));
    case QMetaType::Double:
    case QMetaType::Float:     return QString::number(v.toDouble(), 'g', 15);
    case QMetaType::QVariantList: {
        QStringList parts;
        for (const QVariant &x : v.toList()) parts << qivText(x);
        return parts.join(QStringLiteral(", "));
    }
    default:                   return v.toString();
    }
}

QString qivFill(const QString &text, const QVariantMap &params) {
    QString out = QCoreApplication::translate("QiValidation", text.toUtf8().constData());
    for (auto it = params.constBegin(); it != params.constEnd(); ++it)
        if (!it.key().startsWith(QLatin1Char('_')))
            out.replace(QLatin1Char('{') + it.key() + QLatin1Char('}'), qivText(it.value()));
    return out;
}

bool qivNumber(const QVariant &v, double &out) {
    bool ok = false;
    out = v.toDouble(&ok);
    return ok && std::isfinite(out);
}

// a < b: -1, equal: 0, a > b: 1; false if they can't be compared.
bool qivCompare(const QVariant &a, const QVariant &b, int &result, const QByteArray &zone) {
    const auto isDate = [](const QVariant &v) {
        const int t = v.userType();
        return t == QMetaType::QDate || t == QMetaType::QDateTime || t == QMetaType::QTime;
    };
    if (isDate(a) || isDate(b)) {
        const QDateTime x = qiDateTimeOf(a, zone), y = qiDateTimeOf(b, zone);
        if (!x.isValid() || !y.isValid()) return false;
        result = x < y ? -1 : x > y ? 1 : 0;
        return true;
    }
    double x, y;
    if (qivNumber(a, x) && qivNumber(b, y)) {
        result = x < y ? -1 : x > y ? 1 : 0;
        return true;
    }
    if (a.userType() == QMetaType::QString && b.userType() == QMetaType::QString) {
        const QDateTime dx = qiDateTimeOf(a, zone), dy = qiDateTimeOf(b, zone);
        if (dx.isValid() && dy.isValid()) {
            result = dx < dy ? -1 : dx > dy ? 1 : 0;
            return true;
        }
        result = QString::compare(a.toString(), b.toString());
        result = result < 0 ? -1 : result > 0 ? 1 : 0;
        return true;
    }
    return false;
}

QiRule qivRule(const QString &name, const QString &message, QiRule::Check check) {
    return QiRule(name, message, std::move(check));
}

// A rule on a date: the value parsed (in the rule's zone), or "is not a valid date".
QiRule qivDateRule(const QString &name, const QString &message,
                   std::function<bool(const QDateTime &, const QiRuleContext &, QVariantMap &)> check) {
    return qivRule(name, message, [check](const QiRuleContext &c, QVariantMap &p) {
        const QByteArray zone = qivZoneOf(p);
        QDateTime dt = qiDateTimeOf(c.value(), zone);
        if (!dt.isValid()) {
            p.insert(QStringLiteral("_message"), QStringLiteral("is not a valid date"));
            return false;
        }
        if (!zone.isEmpty() && dt.timeSpec() != Qt::LocalTime)
            dt = dt.toTimeZone(qivZone(zone));
        return check(dt, c, p);
    });
}

QString qivDayName(int day) {
    return QLocale(QLocale::English).dayName(day);
}

// A value as SQL, quoted by the database's own driver (bound strings are mangled by
// some ODBC builds; see QiMigrator), with SQLite dates as Qt stores them.
QString qivLiteral(QSqlDatabase db, const QVariant &value) {
    if (value.isNull())
        return QStringLiteral("NULL");
    const bool sqlite = db.driverName() == QLatin1String("QSQLITE");
    if (sqlite && value.userType() == QMetaType::QDateTime)
        return QLatin1Char('\'') + value.toDateTime().toString(Qt::ISODateWithMs) + QLatin1Char('\'');
    if (sqlite && value.userType() == QMetaType::QDate)
        return QLatin1Char('\'') + value.toDate().toString(Qt::ISODate) + QLatin1Char('\'');
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QSqlField field(QStringLiteral("v"), value.metaType());
#else
    QSqlField field(QStringLiteral("v"), QVariant::Type(value.userType()));
#endif
    field.setValue(value);
    const QString text = db.driver()->formatValue(field);
    return db.driver()->dbmsType() == QSqlDriver::MSSqlServer && value.userType() == QMetaType::QString
               ? QLatin1Char('N') + text : text;
}

// The value as the database stores it (QiField conversions applied).
QVariant qivStored(const QiRuleContext &c, const QString &field) {
    return c.info->value(c.model, field, true);
}

// "= x", or "IS NULL".
QString qivEquals(QSqlDatabase db, const QVariant &value) {
    return value.isNull() ? QStringLiteral(" IS NULL") : QStringLiteral(" = ") + qivLiteral(db, value);
}

bool qivCount(const QiRuleContext &c, const QString &where, qint64 &count) {
    QSqlQuery q = c.connection->query();
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM %1 WHERE %2").arg(c.table(), where)) || !q.next())
        return false;
    count = q.value(0).toLongLong();
    return true;
}

// "AND scope1 = .. AND id <> self"
QString qivScopeAndSelf(const QiRuleContext &c, const QStringList &scope) {
    QSqlDatabase db = c.connection->sql().database();
    QString where;
    for (const QString &s : scope)
        where += QStringLiteral(" AND ") + s + qivEquals(db, qivStored(c, s));
    const QVariant self = c.id();
    if (!self.isNull() && !c.creating)
        where += QStringLiteral(" AND ") + c.idColumn() + QStringLiteral(" <> ") + qivLiteral(db, self);
    return where;
}

// `dialect`: Studio's ("mysql", "sqlserver"), Qt's driver ("QMYSQL", "QODBC") or the
// statement writer's ("MYSQL", "MSSQL"). Characters, not bytes, everywhere.
QString qivLengthSql(const QString &column, const QString &dialect) {
    const QString d = dialect.toUpper();
    if (d.contains(QLatin1String("MYSQL")) || d.contains(QLatin1String("MARIADB")))
        return QStringLiteral("CHAR_LENGTH(%1)").arg(column);
    if (d.contains(QLatin1String("SQLSERVER")) || d.contains(QLatin1String("MSSQL")) || d == QLatin1String("QODBC"))
        return QStringLiteral("LEN(%1)").arg(column);
    return QStringLiteral("length(%1)").arg(column);
}

QString qivSqlValue(const QVariant &v) {
    double d;
    if (v.userType() != QMetaType::QString && qivNumber(v, d))
        return QString::number(d, 'g', 15);
    return QLatin1Char('\'') + v.toString().replace(QLatin1Char('\''), QLatin1String("''")) + QLatin1Char('\'');
}

} // namespace

// ============================================================================
// Presence
// ============================================================================

QiRule QiRequired() {
    return qivRule(QStringLiteral("required"), QStringLiteral("is required"),
                   [](const QiRuleContext &c, QVariantMap &) { return !qivBlank(c.value()); }).checkingNull();
}

QiRule QiRequiredIf(const QString &other, const QVariant &value) {
    QiRule r = QiRequired().when([other, value](const QiRuleContext &c) { return c.valueOf(other) == value; });
    r.params.insert(QStringLiteral("other"), other);
    return r;
}

QiRule QiRequiredIf(std::function<bool(const QiRuleContext &)> condition) {
    return QiRequired().when(std::move(condition));
}

QiRule QiRequiredUnless(const QString &other, const QVariant &value) {
    QiRule r = QiRequired().when([other, value](const QiRuleContext &c) { return c.valueOf(other) != value; });
    r.params.insert(QStringLiteral("other"), other);
    return r;
}

QiRule QiProhibitedIf(const QString &other, const QVariant &value) {
    QiRule r = qivRule(QStringLiteral("prohibited"), QStringLiteral("must be empty"),
                       [](const QiRuleContext &c, QVariantMap &) { return qivBlank(c.value()); })
                   .when([other, value](const QiRuleContext &c) { return c.valueOf(other) == value; });
    r.params.insert(QStringLiteral("other"), other);
    return r;
}

// ============================================================================
// Text
// ============================================================================

QiRule QiLength(int min, int max) {
    const QString message = min > 0 && max >= 0 ? QStringLiteral("must be {min} to {max} characters")
                          : min > 0              ? QStringLiteral("must be at least {min} characters")
                                                 : QStringLiteral("must be at most {max} characters");
    QiRule r = qivRule(QStringLiteral("length"), message, [min, max](const QiRuleContext &c, QVariantMap &p) {
        const int n = c.value().toString().size();
        p.insert(QStringLiteral("length"), n);
        return n >= qMax(0, min) && (max < 0 || n <= max);
    });
    r.params.insert(QStringLiteral("min"), min);
    r.params.insert(QStringLiteral("max"), max);
    r.constraint = [min, max](const QString &column, const QString &dialect) {
        QStringList parts;
        if (min > 0) parts << QStringLiteral("%1 >= %2").arg(qivLengthSql(column, dialect)).arg(min);
        if (max >= 0) parts << QStringLiteral("%1 <= %2").arg(qivLengthSql(column, dialect)).arg(max);
        return parts.join(QStringLiteral(" AND "));
    };
    return r;
}

QiRule QiMinLength(int min) { return QiLength(min, -1); }
QiRule QiMaxLength(int max) { return QiLength(0, max); }

QiRule QiPattern(const QString &regex, const QString &message) {
    const QRegularExpression rx(regex);
    QiRule r = qivRule(QStringLiteral("pattern"), message.isEmpty() ? QStringLiteral("is not in the right format") : message,
                       [rx](const QiRuleContext &c, QVariantMap &) { return rx.match(c.value().toString()).hasMatch(); });
    r.params.insert(QStringLiteral("pattern"), regex);
    return r;
}

QiRule QiEmail() {
    // Pragmatic: one @, a dot in the domain, no spaces; not RFC 5322 in full.
    QiRule r = QiPattern(QStringLiteral("^[A-Za-z0-9.!#$%&'*+/=?^_`{|}~-]+@[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?"
                                        "(?:\\.[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?)+$"),
                         QStringLiteral("is not a valid email address"));
    r.name = QStringLiteral("email");
    return r;
}

QiRule QiUrl() {
    QiRule r = QiPattern(QStringLiteral("^(https?|ftp)://[^\\s/$.?#][^\\s]*$"), QStringLiteral("is not a valid web address"));
    r.name = QStringLiteral("url");
    return r;
}

QiRule QiPhone() {
    return qivRule(QStringLiteral("phone"), QStringLiteral("is not a valid phone number"), [](const QiRuleContext &c, QVariantMap &) {
        const QString s = c.value().toString().trimmed();
        static const QRegularExpression qivPhoneChars(QStringLiteral("^\\+?[0-9 ().-]+$"));
        if (!qivPhoneChars.match(s).hasMatch()) return false;
        int digits = 0;
        for (const QChar ch : s) digits += ch.isDigit();
        return digits >= 7 && digits <= 15;
    });
}

QiRule QiUuid() {
    return qivRule(QStringLiteral("uuid"), QStringLiteral("is not a valid UUID"), [](const QiRuleContext &c, QVariantMap &) {
        static const QRegularExpression qivUuidRx(QStringLiteral("^\\{?[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\\}?$"));
        return qivUuidRx.match(c.value().toString()).hasMatch();
    });
}

QiRule QiLuhn() {
    return qivRule(QStringLiteral("luhn"), QStringLiteral("is not a valid card number"), [](const QiRuleContext &c, QVariantMap &) {
        QString digits;
        for (const QChar ch : c.value().toString()) {
            if (ch.isDigit()) digits += ch;
            else if (ch != QLatin1Char(' ') && ch != QLatin1Char('-')) return false;
        }
        if (digits.size() < 12 || digits.size() > 19) return false;
        int sum = 0;
        bool twice = false;
        for (int i = digits.size() - 1; i >= 0; --i) {
            int d = digits.at(i).digitValue();
            if (twice) { d *= 2; if (d > 9) d -= 9; }
            sum += d;
            twice = !twice;
        }
        return sum % 10 == 0;
    });
}

QiRule QiCharset(const QString &allowed) {
    const QRegularExpression rx(QStringLiteral("^[%1]*$").arg(allowed));
    QiRule r = qivRule(QStringLiteral("charset"), QStringLiteral("may only contain {allowed}"),
                       [rx](const QiRuleContext &c, QVariantMap &) { return rx.match(c.value().toString()).hasMatch(); });
    r.params.insert(QStringLiteral("allowed"), allowed);
    return r;
}

QiRule QiStrongPassword(int minLength, bool upper, bool lower, bool digit, bool symbol) {
    QStringList needs;
    if (upper) needs << QStringLiteral("a capital letter");
    if (lower) needs << QStringLiteral("a small letter");
    if (digit) needs << QStringLiteral("a digit");
    if (symbol) needs << QStringLiteral("a symbol");
    QString list = needs.join(QStringLiteral(", "));
    const int last = list.lastIndexOf(QStringLiteral(", "));
    if (last >= 0) list.replace(last, 2, QStringLiteral(" and "));
    QiRule r = qivRule(QStringLiteral("password"),
                       needs.isEmpty() ? QStringLiteral("must be at least {min} characters")
                                       : QStringLiteral("must be at least {min} characters, with {needs}"),
                       [=](const QiRuleContext &c, QVariantMap &) {
        const QString s = c.value().toString();
        bool u = false, l = false, d = false, y = false;
        for (const QChar ch : s) {
            u = u || ch.isUpper(); l = l || ch.isLower(); d = d || ch.isDigit();
            y = y || (!ch.isLetterOrNumber() && !ch.isSpace());
        }
        return s.size() >= minLength && (!upper || u) && (!lower || l) && (!digit || d) && (!symbol || y);
    });
    r.params.insert(QStringLiteral("min"), minLength);
    r.params.insert(QStringLiteral("needs"), list);
    return r;
}

// ============================================================================
// Numbers
// ============================================================================

namespace {
QiRule qivBounds(const QString &name, const QString &message, double min, double max, bool hasMin, bool hasMax, bool strictMin = false) {
    QiRule r = qivRule(name, message, [=](const QiRuleContext &c, QVariantMap &p) {
        double v;
        if (!qivNumber(c.value(), v)) { p.insert(QStringLiteral("_message"), QStringLiteral("must be a number")); return false; }
        if (hasMin && (strictMin ? v <= min : v < min)) return false;
        if (hasMax && v > max) return false;
        return true;
    });
    if (hasMin) r.params.insert(QStringLiteral("min"), min);
    if (hasMax) r.params.insert(QStringLiteral("max"), max);
    r.constraint = [=](const QString &column, const QString &) {
        QStringList parts;
        if (hasMin) parts << QStringLiteral("%1 %2 %3").arg(column, strictMin ? QStringLiteral(">") : QStringLiteral(">="), QString::number(min, 'g', 15));
        if (hasMax) parts << QStringLiteral("%1 <= %2").arg(column, QString::number(max, 'g', 15));
        return parts.join(QStringLiteral(" AND "));
    };
    return r;
}
} // namespace

QiRule QiRange(double min, double max) { return qivBounds(QStringLiteral("range"), QStringLiteral("must be between {min} and {max}"), min, max, true, true); }
QiRule QiMin(double min) { return qivBounds(QStringLiteral("min"), QStringLiteral("must be at least {min}"), min, 0, true, false); }
QiRule QiMax(double max) { return qivBounds(QStringLiteral("max"), QStringLiteral("must be at most {max}"), 0, max, false, true); }
QiRule QiPositive() { return qivBounds(QStringLiteral("positive"), QStringLiteral("must be more than 0"), 0, 0, true, false, true); }
QiRule QiNonNegative() { return qivBounds(QStringLiteral("nonNegative"), QStringLiteral("can't be negative"), 0, 0, true, false); }

QiRule QiDecimals(int places) {
    QiRule r = qivRule(QStringLiteral("decimals"), places == 0 ? QStringLiteral("must be a whole number")
                                                               : QStringLiteral("can have at most {places} decimal places"),
                       [places](const QiRuleContext &c, QVariantMap &) {
        const QVariant v = c.value();
        QString s = v.userType() == QMetaType::QString ? v.toString().trimmed() : QString::number(v.toDouble(), 'f', 10);
        if (v.userType() != QMetaType::QString)
            while (s.contains(QLatin1Char('.')) && (s.endsWith(QLatin1Char('0')) || s.endsWith(QLatin1Char('.')))) s.chop(1);
        const int dot = s.indexOf(QLatin1Char('.'));
        return dot < 0 || s.size() - dot - 1 <= places;
    });
    r.params.insert(QStringLiteral("places"), places);
    return r;
}

QiRule QiMultipleOf(double step) {
    QiRule r = qivRule(QStringLiteral("multipleOf"), QStringLiteral("must be a multiple of {step}"), [step](const QiRuleContext &c, QVariantMap &) {
        double v;
        if (!qivNumber(c.value(), v) || step == 0) return false;
        const double n = v / step;
        return std::fabs(n - std::round(n)) < 1e-9;
    });
    r.params.insert(QStringLiteral("step"), step);
    return r;
}

// ============================================================================
// Choices
// ============================================================================

QiRule QiOneOf(const QVariantList &values) {
    QiRule r = qivRule(QStringLiteral("oneOf"), QStringLiteral("must be one of {choices}"), [values](const QiRuleContext &c, QVariantMap &) {
        const QVariant v = c.value();
        for (const QVariant &x : values)
            if (x == v || x.toString() == v.toString()) return true;
        return false;
    });
    r.params.insert(QStringLiteral("choices"), values);
    r.constraint = [values](const QString &column, const QString &) {
        QStringList parts;
        for (const QVariant &v : values) parts << qivSqlValue(v);
        return QStringLiteral("%1 IN (%2)").arg(column, parts.join(QStringLiteral(", ")));
    };
    return r;
}

QiRule QiNotOneOf(const QVariantList &values) {
    QiRule r = qivRule(QStringLiteral("notOneOf"), QStringLiteral("can't be {value}"), [values](const QiRuleContext &c, QVariantMap &) {
        const QVariant v = c.value();
        for (const QVariant &x : values)
            if (x == v || x.toString().compare(v.toString(), Qt::CaseInsensitive) == 0) return false;
        return true;
    });
    r.params.insert(QStringLiteral("choices"), values);
    return r;
}

// ============================================================================
// Comparing with another field
// ============================================================================

QiRule QiSameAs(const QString &other) {
    QiRule r = qivRule(QStringLiteral("sameAs"), QStringLiteral("doesn't match {otherLabel}"),
                       [other](const QiRuleContext &c, QVariantMap &) { return c.value().toString() == c.valueOf(other).toString(); });
    r.params.insert(QStringLiteral("other"), other);
    return r;
}

QiRule QiDifferentFrom(const QString &other) {
    QiRule r = qivRule(QStringLiteral("differentFrom"), QStringLiteral("must be different from {otherLabel}"),
                       [other](const QiRuleContext &c, QVariantMap &) { return c.value().toString() != c.valueOf(other).toString(); });
    r.params.insert(QStringLiteral("other"), other);
    return r;
}

namespace {
QiRule qivOrdered(const QString &name, const QString &message, const QString &other, bool orEqual, int want) {
    QiRule r = qivRule(name, message, [=](const QiRuleContext &c, QVariantMap &p) {
        const QVariant o = c.valueOf(other);
        if (qivBlank(o)) return true;            // nothing to compare with yet
        int cmp = 0;
        if (!qivCompare(c.value(), o, cmp, qivZoneOf(p))) return true;
        return cmp == want || (orEqual && cmp == 0);
    });
    r.params.insert(QStringLiteral("other"), other);
    return r;
}
} // namespace

QiRule QiGreaterThan(const QString &other, bool orEqual) {
    return qivOrdered(QStringLiteral("greaterThan"), orEqual ? QStringLiteral("must be at least {otherLabel}")
                                                             : QStringLiteral("must be greater than {otherLabel}"), other, orEqual, 1);
}
QiRule QiLessThan(const QString &other, bool orEqual) {
    return qivOrdered(QStringLiteral("lessThan"), orEqual ? QStringLiteral("can't be more than {otherLabel}")
                                                          : QStringLiteral("must be less than {otherLabel}"), other, orEqual, -1);
}

// ============================================================================
// The database
// ============================================================================

QiRule QiUniqueIn(const QStringList &scope) {
    QiRule r = qivRule(QStringLiteral("unique"), QStringLiteral("is already taken"), [scope](const QiRuleContext &c, QVariantMap &) {
        QSqlDatabase db = c.connection->sql().database();
        qint64 n = 0;
        if (!qivCount(c, c.field + qivEquals(db, qivStored(c, c.field)) + qivScopeAndSelf(c, scope), n))
            return true;                          // can't tell: leave it to the database
        return n == 0;
    });
    r.needsDatabase = true;
    r.params.insert(QStringLiteral("scope"), scope);
    return r;
}

QiRule QiExists(const QString &table, const QString &column) {
    QiRule r = qivRule(QStringLiteral("exists"), QStringLiteral("doesn't exist"), [table, column](const QiRuleContext &c, QVariantMap &) {
        QSqlDatabase db = c.connection->sql().database();
        QSqlQuery q = c.connection->query();
        if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM %1 WHERE %2%3").arg(table, column, qivEquals(db, qivStored(c, c.field)))) || !q.next())
            return true;
        return q.value(0).toLongLong() > 0;
    });
    r.needsDatabase = true;
    r.params.insert(QStringLiteral("table"), table);
    return r;
}

QiRule QiNoOverlap(const QString &startField, const QStringList &scope) {
    QiRule r = qivRule(QStringLiteral("overlap"), QStringLiteral("overlaps another entry"), [startField, scope](const QiRuleContext &c, QVariantMap &) {
        const QVariant start = qivStored(c, startField), end = qivStored(c, c.field);
        if (start.isNull())
            return true;
        QSqlDatabase db = c.connection->sql().database();
        // Another row overlaps when it starts before this ends and ends after this starts.
        const QString where = QStringLiteral("%1 < %2 AND %3 > %4").arg(startField, qivLiteral(db, end), c.field, qivLiteral(db, start))
                            + qivScopeAndSelf(c, scope);
        qint64 n = 0;
        if (!qivCount(c, where, n))
            return true;
        return n == 0;
    });
    r.needsDatabase = true;
    r.params.insert(QStringLiteral("other"), startField);
    return r;
}

// ============================================================================
// Dates and times
// ============================================================================

QiRule QiDate() {
    return qivDateRule(QStringLiteral("date"), QStringLiteral("is not a valid date"),
                       [](const QDateTime &, const QiRuleContext &, QVariantMap &) { return true; });
}

QiRule QiDateMin(const QVariant &min) {
    QiRule r = qivDateRule(QStringLiteral("dateMin"), QStringLiteral("must be on or after {min}"),
                           [min](const QDateTime &dt, const QiRuleContext &, QVariantMap &p) {
        const QDateTime m = qiResolveDate(min, qivZoneOf(p));
        p.insert(QStringLiteral("min"), m);
        return !m.isValid() || dt >= m;
    });
    return r;
}

QiRule QiDateMax(const QVariant &max) {
    return qivDateRule(QStringLiteral("dateMax"), QStringLiteral("must be on or before {max}"),
                       [max](const QDateTime &dt, const QiRuleContext &, QVariantMap &p) {
        QDateTime m = qiResolveDate(max, qivZoneOf(p));
        // A date limit includes the whole day.
        if (max.userType() == QMetaType::QDate || (max.userType() == QMetaType::QString && m.time() == QTime(0, 0) && dt.time() != QTime(0, 0)))
            m = m.addDays(1).addMSecs(-1);
        p.insert(QStringLiteral("max"), qiResolveDate(max, qivZoneOf(p)));
        return !m.isValid() || dt <= m;
    });
}

QiRule QiDateBetween(const QVariant &min, const QVariant &max) {
    QiRule lo = QiDateMin(min), hi = QiDateMax(max);
    QiRule r = qivDateRule(QStringLiteral("dateBetween"), QStringLiteral("must be between {min} and {max}"),
                           [lo, hi](const QDateTime &, const QiRuleContext &c, QVariantMap &p) {
        QVariantMap a = p, b = p;
        const bool ok = lo.check(c, a) && hi.check(c, b);
        p.insert(QStringLiteral("min"), a.value(QStringLiteral("min")));
        p.insert(QStringLiteral("max"), b.value(QStringLiteral("max")));
        return ok;
    });
    return r;
}

QiRule QiPast() {
    return qivDateRule(QStringLiteral("past"), QStringLiteral("must be in the past"),
                       [](const QDateTime &dt, const QiRuleContext &, QVariantMap &p) { return dt < qivNow(qivZoneOf(p)); });
}
QiRule QiFuture() {
    return qivDateRule(QStringLiteral("future"), QStringLiteral("must be in the future"),
                       [](const QDateTime &dt, const QiRuleContext &, QVariantMap &p) { return dt > qivNow(qivZoneOf(p)); });
}
QiRule QiTodayOrLater() {
    return qivDateRule(QStringLiteral("todayOrLater"), QStringLiteral("can't be in the past"),
                       [](const QDateTime &dt, const QiRuleContext &, QVariantMap &p) { return dt.date() >= qivToday(qivZoneOf(p)); });
}
QiRule QiTodayOrEarlier() {
    return qivDateRule(QStringLiteral("todayOrEarlier"), QStringLiteral("can't be in the future"),
                       [](const QDateTime &dt, const QiRuleContext &, QVariantMap &p) { return dt.date() <= qivToday(qivZoneOf(p)); });
}

namespace {
int qivAge(const QDate &born, const QDate &today) {
    int age = today.year() - born.year();
    if (born.addYears(age) > today) --age;     // QDate::addYears takes 29 Feb to 28 Feb
    return age;
}
} // namespace

QiRule QiMinAge(int years) {
    QiRule r = qivDateRule(QStringLiteral("minAge"), QStringLiteral("must be at least {years} years ago"),
                           [years](const QDateTime &dt, const QiRuleContext &, QVariantMap &p) {
        return qivAge(dt.date(), qivToday(qivZoneOf(p))) >= years;
    });
    r.params.insert(QStringLiteral("years"), years);
    return r;
}
QiRule QiMaxAge(int years) {
    QiRule r = qivDateRule(QStringLiteral("maxAge"), QStringLiteral("must be at most {years} years ago"),
                           [years](const QDateTime &dt, const QiRuleContext &, QVariantMap &p) {
        return qivAge(dt.date(), qivToday(qivZoneOf(p))) <= years;
    });
    r.params.insert(QStringLiteral("years"), years);
    return r;
}

QiRule QiWeekday() {
    return qivDateRule(QStringLiteral("weekday"), QStringLiteral("must be a weekday"),
                       [](const QDateTime &dt, const QiRuleContext &, QVariantMap &) { return dt.date().dayOfWeek() <= 5; });
}
QiRule QiWeekend() {
    return qivDateRule(QStringLiteral("weekend"), QStringLiteral("must be on a weekend"),
                       [](const QDateTime &dt, const QiRuleContext &, QVariantMap &) { return dt.date().dayOfWeek() >= 6; });
}
QiRule QiOnDays(const QList<int> &days) {
    QStringList names;
    for (int d : days) names << qivDayName(d);
    QiRule r = qivDateRule(QStringLiteral("onDays"), QStringLiteral("must be on a {days}"),
                           [days](const QDateTime &dt, const QiRuleContext &, QVariantMap &) { return days.contains(dt.date().dayOfWeek()); });
    QString list = names.join(QStringLiteral(", "));
    const int last = list.lastIndexOf(QStringLiteral(", "));
    if (last >= 0) list.replace(last, 2, QStringLiteral(" or "));
    r.params.insert(QStringLiteral("days"), list);
    return r;
}
QiRule QiNotOn(const QList<QDate> &dates) {
    return qivDateRule(QStringLiteral("notOn"), QStringLiteral("isn't available on that date"),
                       [dates](const QDateTime &dt, const QiRuleContext &, QVariantMap &) { return !dates.contains(dt.date()); });
}
QiRule QiBusinessDay(std::function<bool(const QDate &)> isHoliday) {
    return qivDateRule(QStringLiteral("businessDay"), QStringLiteral("must be a business day"),
                       [isHoliday](const QDateTime &dt, const QiRuleContext &, QVariantMap &) {
        return dt.date().dayOfWeek() <= 5 && !(isHoliday && isHoliday(dt.date()));
    });
}

QiRule QiTimeBetween(const QTime &from, const QTime &to) {
    QiRule r = qivDateRule(QStringLiteral("timeBetween"), QStringLiteral("must be between {from} and {to}"),
                           [from, to](const QDateTime &dt, const QiRuleContext &, QVariantMap &) {
        const QTime t = dt.time();
        return from <= to ? (t >= from && t <= to) : (t >= from || t <= to);   // across midnight
    });
    r.params.insert(QStringLiteral("from"), from);
    r.params.insert(QStringLiteral("to"), to);
    return r;
}

QiRule QiTimeStep(int minutes) {
    QiRule r = qivDateRule(QStringLiteral("timeStep"), QStringLiteral("must be on a {minutes}-minute mark"),
                           [minutes](const QDateTime &dt, const QiRuleContext &, QVariantMap &) {
        const QTime t = dt.time();
        return minutes > 0 && t.second() == 0 && t.msec() == 0 && (t.hour() * 60 + t.minute()) % minutes == 0;
    });
    r.params.insert(QStringLiteral("minutes"), minutes);
    return r;
}

QiRule QiAfter(const QString &other, bool orEqual) {
    QiRule r = qivDateRule(QStringLiteral("after"), orEqual ? QStringLiteral("can't be before {otherLabel}")
                                                            : QStringLiteral("must be after {otherLabel}"),
                           [other, orEqual](const QDateTime &dt, const QiRuleContext &c, QVariantMap &p) {
        const QDateTime o = qiDateTimeOf(c.valueOf(other), qivZoneOf(p));
        return !o.isValid() || dt > o || (orEqual && dt == o);
    });
    r.params.insert(QStringLiteral("other"), other);
    return r;
}

QiRule QiBefore(const QString &other, bool orEqual) {
    QiRule r = qivDateRule(QStringLiteral("before"), orEqual ? QStringLiteral("can't be after {otherLabel}")
                                                             : QStringLiteral("must be before {otherLabel}"),
                           [other, orEqual](const QDateTime &dt, const QiRuleContext &c, QVariantMap &p) {
        const QDateTime o = qiDateTimeOf(c.valueOf(other), qivZoneOf(p));
        return !o.isValid() || dt < o || (orEqual && dt == o);
    });
    r.params.insert(QStringLiteral("other"), other);
    return r;
}

namespace {
QiRule qivSpan(const QString &name, const QString &unit, const QString &other, int minimum, int maximum,
               std::function<qint64(const QDateTime &from, const QDateTime &to)> distance) {
    const QString message = minimum > 0 && maximum >= 0 ? QStringLiteral("must be {min} to {max} %1 after {otherLabel}").arg(unit)
                          : maximum >= 0                  ? QStringLiteral("must be at most {max} %1 after {otherLabel}").arg(unit)
                                                          : QStringLiteral("must be at least {min} %1 after {otherLabel}").arg(unit);
    QiRule r = qivDateRule(name, message, [=](const QDateTime &dt, const QiRuleContext &c, QVariantMap &p) {
        const QDateTime o = qiDateTimeOf(c.valueOf(other), qivZoneOf(p));
        if (!o.isValid()) return true;
        const qint64 n = distance(o, dt);
        p.insert(QStringLiteral("distance"), n);
        return n >= minimum && (maximum < 0 || n <= maximum);
    });
    r.params.insert(QStringLiteral("other"), other);
    r.params.insert(QStringLiteral("min"), minimum);
    r.params.insert(QStringLiteral("max"), maximum);
    return r;
}
} // namespace

QiRule QiDaysAfter(const QString &other, int minDays, int maxDays) {
    return qivSpan(QStringLiteral("daysAfter"), QStringLiteral("days"), other, minDays, maxDays,
                   [](const QDateTime &a, const QDateTime &b) { return a.date().daysTo(b.date()); });
}

QiRule QiMinutesAfter(const QString &other, int minMinutes, int maxMinutes) {
    return qivSpan(QStringLiteral("minutesAfter"), QStringLiteral("minutes"), other, minMinutes, maxMinutes,
                   [](const QDateTime &a, const QDateTime &b) { return a.secsTo(b) / 60; });
}

QiRule QiNotExpired() {
    return qivRule(QStringLiteral("notExpired"), QStringLiteral("has expired"), [](const QiRuleContext &c, QVariantMap &p) {
        const QVariant v = c.value();
        QDate month;
        if (v.userType() == QMetaType::QDate || v.userType() == QMetaType::QDateTime) {
            month = v.toDate();
        } else {
            static const QRegularExpression qivExpiryRx(QStringLiteral("^\\s*(\\d{1,2})\\s*[/-]\\s*(\\d{2}|\\d{4})\\s*$"));
            const QRegularExpressionMatch m = qivExpiryRx.match(v.toString());
            const int mm = m.captured(1).toInt();
            int yy = m.captured(2).toInt();
            if (m.captured(2).size() == 2) yy += 2000;
            if (m.hasMatch() && mm >= 1 && mm <= 12) month = QDate(yy, mm, 1);
        }
        if (!month.isValid()) {
            p.insert(QStringLiteral("_message"), QStringLiteral("is not a valid expiry date"));
            return false;
        }
        const QDate lastDay(month.year(), month.month(), month.daysInMonth());   // valid through the month's end
        return lastDay >= qivToday(qivZoneOf(p));
    });
}

QiRule QiExistingLocalTime(const QByteArray &zoneId) {
    QiRule r = qivRule(QStringLiteral("existingTime"), QStringLiteral("doesn't exist in {zone}: the clocks change then"),
                       [zoneId](const QiRuleContext &c, QVariantMap &p) {
        // The wall-clock date and time as written, wherever the value thinks it is.
        const QVariant v = c.value();
        QDate d; QTime t;
        if (v.userType() == QMetaType::QDateTime) { d = v.toDateTime().date(); t = v.toDateTime().time(); }
        else {
            QString s = v.toString().trimmed();
            static const QRegularExpression qivSpacedLocal(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}) (\\d)"));
            s.replace(qivSpacedLocal, QStringLiteral("\\1T\\2"));
            const QDateTime dt = QDateTime::fromString(s.left(19), QStringLiteral("yyyy-MM-ddTHH:mm:ss")).isValid()
                                     ? QDateTime::fromString(s.left(19), QStringLiteral("yyyy-MM-ddTHH:mm:ss"))
                                     : QDateTime::fromString(s.left(16), QStringLiteral("yyyy-MM-ddTHH:mm"));
            d = dt.date(); t = dt.time();
        }
        if (!d.isValid() || !t.isValid()) {
            p.insert(QStringLiteral("_message"), QStringLiteral("is not a valid date"));
            return false;
        }
        const QTimeZone zone(zoneId);
        if (!zone.isValid()) return true;
        const QDateTime there(d, t, zone);
        return there.isValid() && there.date() == d && there.time() == t;     // a gap moves it
    });
    r.params.insert(QStringLiteral("zone"), QString::fromLatin1(zoneId));
    return r;
}

// ============================================================================
// Clean-up
// ============================================================================

QiRule QiNormalize(std::function<QVariant(const QVariant &)> transform) {
    QiRule r(QStringLiteral("normalize"), QString(), QiRule::Check());
    r.normalize = std::move(transform);
    return r;
}

QiRule QiTrim() {
    QiRule r = QiNormalize([](const QVariant &v) { return v.userType() == QMetaType::QString ? QVariant(v.toString().trimmed()) : v; });
    r.name = QStringLiteral("trim");
    return r;
}
QiRule QiLower() {
    QiRule r = QiNormalize([](const QVariant &v) { return v.userType() == QMetaType::QString ? QVariant(v.toString().toLower()) : v; });
    r.name = QStringLiteral("lower");
    return r;
}
QiRule QiUpper() {
    QiRule r = QiNormalize([](const QVariant &v) { return v.userType() == QMetaType::QString ? QVariant(v.toString().toUpper()) : v; });
    r.name = QStringLiteral("upper");
    return r;
}
QiRule QiCollapseSpaces() {
    QiRule r = QiNormalize([](const QVariant &v) { return v.userType() == QMetaType::QString ? QVariant(v.toString().simplified()) : v; });
    r.name = QStringLiteral("collapseSpaces");
    return r;
}

// ============================================================================
// Your own
// ============================================================================

QiRule QiCustom(const QString &name, const QString &message, std::function<bool(const QVariant &)> check) {
    return qivRule(name, message, [check](const QiRuleContext &c, QVariantMap &) { return check(c.value()); });
}

QiRule QiCustom(const QString &name, const QString &message, std::function<bool(const QiRuleContext &)> check) {
    return qivRule(name, message, [check](const QiRuleContext &c, QVariantMap &) { return check(c); });
}

// ============================================================================
// The validator
// ============================================================================

namespace {

QString qivLabel(QiModelMetaInfo *info, const QString &field) {
    for (int i = 0; i < info->size(); ++i) {
        QiModelMetaInfoField *f = const_cast<QiModelMetaInfoField *>(info->at(i));
        if (f->name == field)
            return f->clause.testFlag(QiClause::LABEL) ? f->clause.flag(QiClause::LABEL).toString() : qiHumanize(field);
    }
    return qiHumanize(field);
}

bool qivApplies(const QiRule &r, const QString &context) {
    return r.contexts.isEmpty() || r.contexts.contains(context);
}

} // namespace

QiValidation QiValidator::validate(QiAbstractModel *model, QiModelMetaInfo *info, QiConnection *connection, const Options &o) {
    QiValidation v;
    if (!model || !info)
        return v;

    QiRuleContext ctx;
    ctx.model = model;
    ctx.info = info;
    ctx.connection = connection && connection->isOpen() ? connection : nullptr;
    const QVariant id = ctx.id();
    ctx.creating = id.isNull();
    ctx.context = o.context.isEmpty() ? (ctx.creating ? QStringLiteral("create") : QStringLiteral("update")) : o.context;

    for (int i = 0; i < info->size(); ++i) {
        QiModelMetaInfoField *f = const_cast<QiModelMetaInfoField *>(info->at(i));
        if (!o.fields.isEmpty() && !o.fields.contains(f->name))
            continue;
        ctx.field = f->name;
        const QString label = qivLabel(info, f->name);
        v.setLabel(f->name, label);

        QiRuleList rules;
        // What the column already says.
        const bool key = f->clause.testFlag(QiClause::PRIMARY_KEY) || f->name == QLatin1String("id");
        if (f->clause.testFlag(QiClause::NOT_NULL) && !key && !f->clause.testFlag(QiClause::DEFAULT)) {
            QiRule notNull = qivRule(QStringLiteral("required"), QStringLiteral("is required"),
                                     [](const QiRuleContext &c, QVariantMap &) { return !c.value().isNull(); }).checkingNull();
            rules << notNull;
        }
        rules += f->clause.rules();
        if (o.uniqueKeys && f->clause.testFlag(QiClause::UNIQUE) && !key)
            rules << QiUniqueIn();

        if (o.normalize)
            for (const QiRule &r : rules)
                if (r.normalize && qivApplies(r, ctx.context) && (!r.condition || r.condition(ctx)))
                    ctx.setValue(r.normalize(ctx.value()));

        bool failed = false;
        for (const QiRule &r : rules) {
            if (!r.check || !qivApplies(r, ctx.context))
                continue;
            if (r.needsDatabase && (!o.database || !ctx.connection || failed))
                continue;                          // no point asking about a value that's already wrong
            if (r.condition && !r.condition(ctx))
                continue;
            const QVariant value = ctx.value();
            if (r.skipNull && qivBlank(value))
                continue;
            QVariantMap p = r.params;
            p.insert(QStringLiteral("label"), label);
            p.insert(QStringLiteral("value"), value);
            if (p.contains(QStringLiteral("other")))
                p.insert(QStringLiteral("otherLabel"), qivLabel(info, p.value(QStringLiteral("other")).toString()));
            if (r.check(ctx, p))
                continue;
            const QString message = qivFill(p.value(QStringLiteral("_message"), r.text).toString(), p);
            if (r.severity == QiRule::Warning) {
                v.addWarning(f->name, message, r.name);
            } else {
                v.addError(f->name, message, r.name);
                failed = true;
            }
        }
    }
    return v;
}

QStringList QiValidator::constraints(const QString &column, const QiRuleList &rules, const QString &dialect) {
    QStringList out;
    for (const QiRule &r : rules) {
        if (!r.enforce || !r.constraint)
            continue;
        const QString sql = r.constraint(column, dialect);
        if (!sql.isEmpty())
            out << QStringLiteral("(%1)").arg(sql);
    }
    return out;
}

QiFieldError QiValidator::fromDatabaseError(const QString &error, QiModelMetaInfo *info) {
    QiFieldError e;
    if (!info)
        return e;
    const QStringList fields = info->fieldNameList();
    auto field = [&](const QString &name) { return fields.contains(name, Qt::CaseInsensitive) ? name : QString(); };
    QRegularExpressionMatch m;

    // SQLite: "UNIQUE constraint failed: user.email", "NOT NULL constraint failed: user.email"
    static const QRegularExpression sqliteRx(QStringLiteral("(UNIQUE|NOT NULL) constraint failed: \\w+\\.(\\w+)"));
    // PostgreSQL: Key (email)=(x) already exists / null value in column "email"
    static const QRegularExpression pgUnique(QStringLiteral("Key \\((\\w+)\\)=\\(.*\\) already exists"));
    static const QRegularExpression pgNull(QStringLiteral("null value in column \"(\\w+)\""));
    // MySQL: Duplicate entry 'x' for key 'user.email' / Column 'email' cannot be null
    static const QRegularExpression myUnique(QStringLiteral("Duplicate entry '.*' for key '(?:\\w+\\.)?(\\w+)'"));
    static const QRegularExpression myNull(QStringLiteral("Column '(\\w+)' cannot be null"));
    // SQL Server: Cannot insert the value NULL into column 'email'
    static const QRegularExpression msNull(QStringLiteral("Cannot insert the value NULL into column '(\\w+)'"));

    if ((m = sqliteRx.match(error)).hasMatch()) {
        e.field = field(m.captured(2));
        e.rule = m.captured(1) == QLatin1String("UNIQUE") ? QStringLiteral("unique") : QStringLiteral("required");
    } else if ((m = pgUnique.match(error)).hasMatch() || (m = myUnique.match(error)).hasMatch()) {
        e.field = field(m.captured(1));
        e.rule = QStringLiteral("unique");
    } else if ((m = pgNull.match(error)).hasMatch() || (m = myNull.match(error)).hasMatch() || (m = msNull.match(error)).hasMatch()) {
        e.field = field(m.captured(1));
        e.rule = QStringLiteral("required");
    }
    if (!e.field.isEmpty())
        e.message = QCoreApplication::translate("QiValidation", e.rule == QLatin1String("unique") ? "is already taken" : "is required");
    return e;
}
