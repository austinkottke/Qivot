#ifndef QiRULE_H
#define QiRULE_H

#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <functional>

class QiAbstractModel;
class QiModelMetaInfo;
class QiConnection;

/// What a rule sees while it checks one field of one record.
class QiRuleContext {
public:
    QiAbstractModel *model = nullptr;
    QiModelMetaInfo *info = nullptr;
    QiConnection    *connection = nullptr;   ///< null: database rules are skipped
    QString          field;
    QString          context;                ///< "create", "update", or one of your own
    bool             creating = true;

    /// This field's value.
    QVariant value() const;
    /// Another field's value on the same record.
    QVariant valueOf(const QString &field) const;
    /// Replace this field's value (normalising rules).
    void setValue(const QVariant &value) const;
    /// The record's table.
    QString table() const;
    /// The record's primary key value (null when it hasn't been saved).
    QVariant id() const;
    /// The record's primary key column ("id" unless the model declares its own).
    QString idColumn() const;
};

/// One validation rule on a field.
/**
  Rules are made by the Qi… functions in qivalidation.h (QiEmail(), QiLength(2, 80),
  QiFuture(), QiNoOverlap("starts_at"), …) and given to QI_FIELD next to the other
  field options:

\code
    QI_DECLARE_MODEL(User, "user",
        QI_FIELD(email, QiNotNull | QiTrim() | QiLower() | QiEmail()),
        QI_FIELD(name,  QiLength(2, 80)),
        QI_FIELD(born,  QiMinAge(13).message("you must be at least 13")),
        QI_FIELD(plan,  QiOneOf({ "free", "pro" }).on("create")));
\endcode

  Each Qi… function returns a QiRule, which the modifiers below adjust:
  message() replaces its text, warning() makes it advisory (it's reported but
  doesn't stop a save), on() limits it to some contexts, when() to some records,
  inZone() evaluates dates in a time zone, and enforced() also writes it into the
  table as a CHECK constraint where the database can express it.

  A message may use {placeholders}: {min}, {max}, {other}, {value}, {label}, …,
  filled from the rule's parameters, and is translated through
  QCoreApplication::translate("QiValidation", …).
 */
class QiRule {
public:
    enum Severity { Error, Warning };

    /// True when the value passes. May add {placeholders} to `params` for the message.
    using Check = std::function<bool(const QiRuleContext &, QVariantMap &params)>;
    using Condition = std::function<bool(const QiRuleContext &)>;
    using Normalize = std::function<QVariant(const QVariant &)>;
    /// The CHECK expression for `column` in `dialect` ("" if it can't be one).
    using Constraint = std::function<QString(const QString &column, const QString &dialect)>;

    QiRule() = default;
    QiRule(const QString &name, const QString &message, Check check);

    // --- Modifiers (each returns a changed copy) ---------------------------
    /// Use this message instead (with {placeholders}).
    QiRule message(const QString &text) const;
    /// Report it, but don't stop a save.
    QiRule warning() const;
    /// Only when validating in one of these contexts ("create", "update", "step2", …).
    QiRule on(const QStringList &contexts) const;
    QiRule on(const QString &context) const { return on(QStringList(context)); }
    /// Only for records where `condition` holds.
    QiRule when(Condition condition) const;
    /// Evaluate "today", "now" and times of day in this zone (an IANA id, "UTC", …).
    QiRule inZone(const QByteArray &zoneId) const;
    /// Also write the rule into CREATE TABLE as a CHECK constraint, where it can be one.
    QiRule enforced() const;
    /// Check null values too (most rules pass a null; QiRequired doesn't).
    QiRule checkingNull() const;

    // --- What it is -------------------------------------------------------
    QString     name;                 ///< "email", "length", "after", … (in errors as `rule`)
    QString     text;                 ///< the message, with {placeholders}
    QVariantMap params;               ///< min, max, other, zone, …
    Check       check;
    Condition   condition;
    Normalize   normalize;            ///< set: runs before the checks and changes the value
    Constraint  constraint;
    Severity    severity = Error;
    QStringList contexts;             ///< empty: always
    bool        needsDatabase = false;
    bool        enforce = false;
    bool        skipNull = true;
};

using QiRuleList = QList<QiRule>;
Q_DECLARE_METATYPE(QiRuleList)

#endif // QiRULE_H
