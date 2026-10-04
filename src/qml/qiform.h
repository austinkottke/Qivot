#ifndef QiFORM_H
#define QiFORM_H

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QVariantMap>
#include <QtQml>
#include <qivalidation.h>

class QiModel;
class QiModelMetaInfo;
class QiFormAttached;

/// A form over one Qivot record, for QML: values, errors as they happen, and saving.
/**
  Give it the model's name; bind controls to `values`, show `errors`, call
  `submit()`. The rules are the model's own (QI_FIELD(email, QiEmail() | …)),
  so the form, `save()` and the database agree.

\code
    import Qivot 1.0
    import Qivot.Forms 1.0

    QiForm { id: signup; model: "User"; onSaved: stack.push(welcome) }

    QiTextField     { form: signup; field: "email" }
    QiPasswordField { form: signup; field: "password" }
    QiSubmitButton  { form: signup; text: "Create account" }
\endcode

  Or wire any control yourself with the attached properties:

\code
    TextField { QiForm.form: signup; QiForm.field: "email" }    // text <-> email, both ways
    Label     { text: emailField.QiForm.error }
\endcode

  When it checks: on every change, the fast rules of the fields visited so far;
  when a field loses focus (touch()), its database rules too (QiUnique, QiExists,
  QiNoOverlap, …); on submit() or validate(), everything, and clean(). Errors
  only show (`errors`) for fields that were visited, or after a submit, so a
  fresh form isn't covered in red. `allErrors` has them all, `valid` says
  whether there are any.

  Values come in as QML gives them (text from a TextField) and are converted to
  the field's type; text that isn't a number, or a date, is an error on that
  field ("must be a number", "is not a valid date").

  Call qiRegisterQml(&engine) once, before loading QML.
 */
class QiForm : public QObject {
    Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QML_ATTACHED(QiFormAttached)
#endif
    /// The model: its class name ("User") or table ("user"). Registered with addModel<T>().
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
    /// "create" / "update" by default; or your own, for rules limited with QiRule::on().
    Q_PROPERTY(QString context READ context WRITE setContext NOTIFY contextChanged)
    /// Validators of your own, in JavaScript: field -> function(value, values).
    /// Return "" (or nothing) when it's fine, a message when it isn't, or
    /// { message: "...", warning: true } for a warning. They run with the model's
    /// rules: as values change, and on submit.
    Q_PROPERTY(QJSValue validators READ validators WRITE setValidators NOTIFY validatorsChanged)

    /// field -> value, as the controls show it (dates as "yyyy-MM-dd", times as "yyyy-MM-dd HH:mm").
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    /// field -> first error, for the fields that should show one.
    Q_PROPERTY(QVariantMap errors READ errors NOTIFY validationChanged)
    /// field -> first error, every field.
    Q_PROPERTY(QVariantMap allErrors READ allErrors NOTIFY validationChanged)
    Q_PROPERTY(QVariantMap warnings READ warnings NOTIFY validationChanged)
    /// No errors anywhere (as far as the fast rules know until a submit).
    Q_PROPERTY(bool valid READ valid NOTIFY validationChanged)
    /// "Label: message" lines for the errors being shown.
    Q_PROPERTY(QString summary READ summary NOTIFY validationChanged)

    /// field -> label (QiLabel, or the field name made readable).
    Q_PROPERTY(QVariantMap labels READ labels NOTIFY modelChanged)
    /// field -> true when it must be filled in (QiNotNull / QiRequired()).
    Q_PROPERTY(QVariantMap required READ required NOTIFY modelChanged)
    /// field -> the values QiOneOf allows (a ComboBox's model).
    Q_PROPERTY(QVariantMap choices READ choices NOTIFY modelChanged)
    /// field -> the most characters QiLength / QiMaxLength allows (0: no limit).
    Q_PROPERTY(QVariantMap maxLengths READ maxLengths NOTIFY modelChanged)
    Q_PROPERTY(QStringList fields READ fields NOTIFY modelChanged)

    /// Changed since load() / reset().
    Q_PROPERTY(bool dirty READ dirty NOTIFY valuesChanged)
    /// field -> true once the person has left it (or a submit has shown everything).
    Q_PROPERTY(QVariantMap visited READ visited NOTIFY validationChanged)
    /// submit() has been tried (every error shows from then on).
    Q_PROPERTY(bool submitted READ submitted NOTIFY validationChanged)
    /// The record's id once saved or loaded (null for a new one).
    Q_PROPERTY(QVariant recordId READ recordId NOTIFY recordChanged)
    Q_PROPERTY(bool isNew READ isNew NOTIFY recordChanged)
    /// Why the last submit() failed, if it wasn't a field ("database is locked").
    Q_PROPERTY(QString error READ error NOTIFY validationChanged)

public:
    explicit QiForm(QObject *parent = nullptr);
    ~QiForm() override;

    QString model() const { return m_modelName; }
    void setModel(const QString &name);
    QString context() const { return m_context; }
    void setContext(const QString &context);
    QJSValue validators() const { return m_validators; }
    void setValidators(const QJSValue &validators);

    QVariantMap values() const { return m_values; }
    QVariantMap errors() const;
    QVariantMap allErrors() const;
    QVariantMap warnings() const;
    bool valid() const;
    QString summary() const;
    QVariantMap labels() const { return m_labels; }
    QVariantMap required() const { return m_required; }
    QVariantMap choices() const { return m_choices; }
    QVariantMap maxLengths() const { return m_maxLengths; }
    QStringList fields() const { return m_fields; }
    bool dirty() const { return m_dirty; }
    bool submitted() const { return m_submitted; }
    QVariantMap visited() const;
    QVariant recordId() const;
    bool isNew() const;
    QString error() const { return m_error; }

    /// Set a field from a control (and check it).
    Q_INVOKABLE void set(const QString &field, const QVariant &value);
    Q_INVOKABLE QVariant value(const QString &field) const { return m_values.value(field); }
    /// The person has left `field`: show its errors, and ask the database about it.
    Q_INVOKABLE void touch(const QString &field);
    /// Check everything (database rules and clean() too); every error shows. True if valid.
    Q_INVOKABLE bool validate(const QString &context = QString());
    /// Check only these fields, with their database rules (a wizard's step). True if they're valid.
    Q_INVOKABLE bool validateFields(const QStringList &fields);
    /// validate(), then save. Emits saved() or failed().
    Q_INVOKABLE bool submit();
    /// Edit an existing record.
    Q_INVOKABLE bool load(const QVariant &id);
    /// A new, empty record.
    Q_INVOKABLE void reset();
    /// Show an error of your own on a field (a server's answer, say).
    Q_INVOKABLE void setError(const QString &field, const QString &message);

    Q_INVOKABLE QString errorFor(const QString &field) const { return errors().value(field).toString(); }
    Q_INVOKABLE QString label(const QString &field) const { return m_labels.value(field).toString(); }

    /// The record being edited (C++).
    QiModel *record() const { return m_record; }

    static QiFormAttached *qmlAttachedProperties(QObject *object);

signals:
    void modelChanged();
    void contextChanged();
    void validatorsChanged();
    void valuesChanged();
    void validationChanged();
    void recordChanged();
    /// submit() saved the record.
    void saved(const QVariant &savedId);
    /// submit() didn't: `summary` says why.
    void failed(const QString &summary);

private:
    QiFormAttached *attached() const;
    void readRecord();
    void check(bool database, const QStringList &also = QStringList());
    void runScriptValidators();
    QVariant toFieldType(int field, const QVariant &value, QString *problem) const;
    QVariant forDisplay(const QVariant &value) const;
    int indexOf(const QString &field) const;

    QString m_modelName, m_context, m_error;
    QJSValue m_validators;
    QiModelMetaInfo *m_info = nullptr;
    QiModel *m_record = nullptr;
    QStringList m_fields;
    QVariantMap m_values, m_labels, m_required, m_choices, m_maxLengths;
    QMap<QString, QString> m_parseErrors;          // text that isn't the field's type
    QMap<QString, QList<QiFieldError>> m_dbErrors;  // from database rules, until the field changes
    QMap<QString, QString> m_ownErrors;             // setError()
    QSet<QString> m_touched;
    QiValidation m_validation;
    bool m_dirty = false;
    bool m_submitted = false;
};

/// QiForm.form / QiForm.field on any control: its value and the field, both ways.
class QiFormAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(QiForm *form READ form WRITE setForm NOTIFY formChanged)
    Q_PROPERTY(QString field READ field WRITE setField NOTIFY formChanged)
    /// The control's property to bind ("" picks: checked, currentText, value or text).
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY formChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString warning READ warning NOTIFY stateChanged)
    Q_PROPERTY(QString label READ label NOTIFY stateChanged)
    Q_PROPERTY(bool required READ required NOTIFY stateChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY stateChanged)
public:
    explicit QiFormAttached(QObject *target);

    QiForm *form() const { return m_form; }
    void setForm(QiForm *form);
    QString field() const { return m_field; }
    void setField(const QString &field);
    QString property() const { return m_property; }
    void setProperty(const QString &property);
    QString error() const;
    QString warning() const;
    QString label() const;
    bool required() const;
    bool hasError() const { return !error().isEmpty(); }

signals:
    void formChanged();
    void stateChanged();

private slots:
    void controlChanged();
    void focusChanged();
    void formValuesChanged();

private:
    void bind();
    QString boundProperty() const;

    QObject *m_target;
    QPointer<QiForm> m_form;
    QString m_field, m_property, m_bound;
    QList<QMetaObject::Connection> m_connections;
    bool m_writing = false;
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QML_DECLARE_TYPEINFO(QiForm, QML_HAS_ATTACHED_PROPERTIES)
#endif

/// Register QiForm (import Qivot 1.0) and the Qivot.Forms controls with `engine`.
void qiRegisterQml(QQmlEngine *engine);

#endif // QiFORM_H
