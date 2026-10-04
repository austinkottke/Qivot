#ifndef QiVALIDATION_H
#define QiVALIDATION_H

#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QRegularExpression>
#include <QStringList>
#include <QTime>
#include <QVariant>
#include <functional>
#include <qirule.h>

/// One problem with one field.
struct QiFieldError {
    QString field;      ///< "email", or a path: "lines[2].quantity"
    QString rule;       ///< the rule that failed: "email", "length", "unique", "custom", …
    QString message;    ///< ready to show: "must be at least 8 characters"
    bool    warning = false;
};

/// What validating a record found: errors per field, and warnings.
/**
\code
    QiValidation v = user.validate();
    if (!v.isValid()) {
        qDebug() << v.error("email");     // "is already taken"
        qDebug() << v.summary();          // "Email: is already taken\nPassword: …"
    }
\endcode

  Warnings (QiRule::warning()) are reported but don't make a record invalid.
  Validations combine: merge() adds another one's problems, optionally under
  a prefix, which is how an order and its lines are checked together.
 */
class QiValidation {
public:
    /// No errors (warnings are allowed).
    bool isValid() const;
    bool hasWarnings() const;
    bool isEmpty() const { return m_items.isEmpty(); }

    QList<QiFieldError> errors() const;
    QList<QiFieldError> warnings() const;
    QList<QiFieldError> all() const { return m_items; }

    /// The first error for `field` ("" if none).
    QString error(const QString &field) const;
    /// Every error for `field`.
    QStringList errorsFor(const QString &field) const;
    QString warning(const QString &field) const;
    bool hasError(const QString &field) const;
    /// The fields with errors, in the order they were found.
    QStringList fields() const;

    void addError(const QString &field, const QString &message, const QString &rule = QStringLiteral("custom"));
    void addWarning(const QString &field, const QString &message, const QString &rule = QStringLiteral("custom"));
    void add(const QiFieldError &error);

    /// Add `other`'s problems, their fields under `prefix` ("lines[2].").
    void merge(const QiValidation &other, const QString &prefix = QString());
    /// Forget `field`'s problems (and everything under "field." / "field[").
    void clear(const QString &field);
    void clear() { m_items.clear(); }

    /// One line per error, "Label: message" (labels from QiLabel, or the field name made readable).
    QString summary() const;
    /// field -> first error message (for QML: `errors.email`).
    QVariantMap errorMap() const;
    QVariantMap warningMap() const;
    /// `{ "valid": false, "errors": { "email": ["is already taken"] }, "warnings": { … } }`
    QJsonObject toJson() const;

    /// Labels for summary() (set by the validator from QiLabel / field names).
    void setLabel(const QString &field, const QString &label) { m_labels.insert(field, label); }

private:
    QList<QiFieldError> m_items;
    QMap<QString, QString> m_labels;
};

/// "first_name" -> "First name" (the label a field gets without QiLabel).
QString qiHumanize(const QString &field);

/// A date or time, from a QDate/QDateTime/QTime or an ISO string. Invalid if it isn't one.
QDateTime qiDateTimeOf(const QVariant &value, const QByteArray &zoneId = QByteArray());

/// A point in time: a QDate/QDateTime, an ISO string, or relative to now:
/// "now", "today", "tomorrow", "yesterday", "today+30d", "now-2h", "+1w", "-18y"
/// (units: s, min, h, d, w, m (months), y). Evaluated in `zoneId` (default: local).
QDateTime qiResolveDate(const QVariant &spec, const QByteArray &zoneId = QByteArray());

// ============================================================================
// Rules. Each returns a QiRule: give it to QI_FIELD, combined with `|`.
// ============================================================================

// --- Presence -----------------------------------------------------------------
/// Must have a value: not null, and not an empty or blank string.
QiRule QiRequired();
/// Required when `otherField` equals `value` ("state" when "country" is "US").
QiRule QiRequiredIf(const QString &otherField, const QVariant &value);
/// Required when `condition` holds.
QiRule QiRequiredIf(std::function<bool(const QiRuleContext &)> condition);
/// Required unless `otherField` equals `value`.
QiRule QiRequiredUnless(const QString &otherField, const QVariant &value);
/// Must be empty when `otherField` equals `value` (no "other" text unless "other" was picked, …).
QiRule QiProhibitedIf(const QString &otherField, const QVariant &value);

// --- Text ---------------------------------------------------------------------
/// Length between `min` and `max` characters (-1: no limit).
QiRule QiLength(int min, int max = -1);
QiRule QiMinLength(int min);
QiRule QiMaxLength(int max);
/// Matches the regular expression (anchor it with ^…$ for the whole value).
QiRule QiPattern(const QString &regex, const QString &message = QString());
QiRule QiEmail();
QiRule QiUrl();
/// A phone number: optional +, then 7 to 15 digits (spaces, dots, dashes and brackets allowed).
QiRule QiPhone();
QiRule QiUuid();
/// Passes the Luhn checksum (card numbers, IMEIs).
QiRule QiLuhn();
/// Only these characters (a regular-expression character class): QiCharset("a-z0-9_").
QiRule QiCharset(const QString &allowed);
/// At least one lower-case letter, upper-case letter, digit and symbol (each optional).
QiRule QiStrongPassword(int minLength = 8, bool needUpper = true, bool needLower = true, bool needDigit = true, bool needSymbol = false);

// --- Numbers ------------------------------------------------------------------
QiRule QiRange(double min, double max);
QiRule QiMin(double min);
QiRule QiMax(double max);
QiRule QiPositive();
QiRule QiNonNegative();
/// At most `places` digits after the point (money: 2).
QiRule QiDecimals(int places);
/// A multiple of `step` (quantities in packs of 6, prices in 0.05s).
QiRule QiMultipleOf(double step);

// --- Choices ------------------------------------------------------------------
QiRule QiOneOf(const QVariantList &values);
QiRule QiNotOneOf(const QVariantList &values);

// --- Comparing with another field ----------------------------------------------
/// Equal to `otherField` (confirm password, confirm email).
QiRule QiSameAs(const QString &otherField);
QiRule QiDifferentFrom(const QString &otherField);
/// Greater than `otherField` (numbers, dates or text); `orEqual` allows equality.
QiRule QiGreaterThan(const QString &otherField, bool orEqual = false);
QiRule QiLessThan(const QString &otherField, bool orEqual = false);

// --- The database (run by validate(), and by save() for the explicit ones) -----
/// No other row has this value; `scope`: within rows that share these fields
/// ("sku" unique per "store_id"). Matching is the database's (its collation).
QiRule QiUniqueIn(const QStringList &scope = QStringList());
/// The value exists in `table`.`column` (a reference that must resolve).
QiRule QiExists(const QString &table, const QString &column = QStringLiteral("id"));
/// [startField, this field) doesn't overlap another row's range in the same
/// `scope` (bookings of one room): another row overlaps when it starts before
/// this one ends and ends after this one starts.
QiRule QiNoOverlap(const QString &startField, const QStringList &scope = QStringList());

// --- Dates and times ------------------------------------------------------------
// Values may be QDate, QDateTime, QTime or ISO strings; a string that isn't a real
// date ("2026-02-30") fails every date rule with "is not a valid date".
// Limits may be QDate/QDateTime or relative: "today", "now", "today+30d", "-18y", …
QiRule QiDate();                                     ///< is a valid date / time
QiRule QiDateMin(const QVariant &min);
QiRule QiDateMax(const QVariant &max);
QiRule QiDateBetween(const QVariant &min, const QVariant &max);
QiRule QiPast();                                     ///< before now
QiRule QiFuture();                                   ///< after now
QiRule QiTodayOrLater();                             ///< not before today (by date)
QiRule QiTodayOrEarlier();
/// A birth date at least `years` ago (as of today).
QiRule QiMinAge(int years);
QiRule QiMaxAge(int years);
/// Monday to Friday.
QiRule QiWeekday();
QiRule QiWeekend();
/// On one of these days (Qt::Monday … Qt::Sunday).
QiRule QiOnDays(const QList<int> &days);
/// Not on any of these dates (holidays).
QiRule QiNotOn(const QList<QDate> &dates);
/// A business day: Monday to Friday and not a holiday (`isHoliday` may be empty).
QiRule QiBusinessDay(std::function<bool(const QDate &)> isHoliday = {});
/// The time of day between `from` and `to` (opening hours); `to` before `from` spans midnight.
QiRule QiTimeBetween(const QTime &from, const QTime &to);
/// On a `minutes` grid from midnight, with no seconds (15-minute slots).
QiRule QiTimeStep(int minutes);
/// After (or at, with `orEqual`) `otherField`'s date/time.
QiRule QiAfter(const QString &otherField, bool orEqual = false);
QiRule QiBefore(const QString &otherField, bool orEqual = false);
/// Between `minDays` and `maxDays` days after `otherField` (-1: no limit): check-out after check-in.
QiRule QiDaysAfter(const QString &otherField, int minDays, int maxDays = -1);
/// Between `minMinutes` and `maxMinutes` after `otherField` (meetings of 15 minutes to 4 hours).
QiRule QiMinutesAfter(const QString &otherField, int minMinutes, int maxMinutes = -1);
/// A card's expiry, "MM/YY", "MM/YYYY" or a date: valid through the end of that month.
QiRule QiNotExpired();
/// A local time that exists in `zoneId` (not inside a daylight-saving gap, like 02:30 when clocks spring forward).
QiRule QiExistingLocalTime(const QByteArray &zoneId);

// --- Clean-up (runs before the checks, and changes the value) --------------------
QiRule QiTrim();
QiRule QiLower();
QiRule QiUpper();
/// Trim, and runs of white space become one space.
QiRule QiCollapseSpaces();
/// Anything: `transform` returns the value to keep.
QiRule QiNormalize(std::function<QVariant(const QVariant &)> transform);

// --- Your own ------------------------------------------------------------------------
/// `check(value)` true when it passes.
QiRule QiCustom(const QString &name, const QString &message, std::function<bool(const QVariant &)> check);
/// With the whole record: `check(context)`.
QiRule QiCustom(const QString &name, const QString &message, std::function<bool(const QiRuleContext &)> check);

// ============================================================================
// Running them.
// ============================================================================

namespace QiValidator {
/// What to run.
struct Options {
    QString     context;          ///< "create" / "update" / your own; "" = by whether the record is new
    QStringList fields;           ///< only these (live checking while typing); empty = all
    bool        database = true;  ///< run the rules that ask the database
    bool        uniqueKeys = true;///< check QiUnique fields and foreign keys too (validate(); save() leaves them to the database)
    bool        normalize = true; ///< apply QiTrim() & co. to the record first
};

/// Validate `model` (a QiModel) against its fields' rules.
QiValidation validate(QiAbstractModel *model, QiModelMetaInfo *info, QiConnection *connection, const Options &options);

/// The CHECK constraints a field's enforced() rules add, for `dialect`.
QStringList constraints(const QString &column, const QiRuleList &rules, const QString &dialect);

/// A database error from a failed write, as a field error ("UNIQUE constraint
/// failed: user.email" -> email: "is already taken"). Empty field if it isn't one.
QiFieldError fromDatabaseError(const QString &error, QiModelMetaInfo *info);
}

/// Validate every record of a list, their errors under "name[i]." ("lines[2].quantity").
template <class List>
QiValidation qiValidateAll(List &rows, const QString &name = QString(), const QString &context = QString())
{
    QiValidation all;
    for (int i = 0; i < rows.size(); ++i) {
        auto *row = rows.at(i);
        all.merge(row->validate(context), QStringLiteral("%1[%2].").arg(name).arg(i));
    }
    return all;
}

#endif // QiVALIDATION_H
