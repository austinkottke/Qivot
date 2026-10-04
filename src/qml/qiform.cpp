#include "qiform.h"
#include <QLocale>
#include <QJSValueIterator>
#include <QMetaProperty>
#include <qimodel.h>
#include <qimodelmetainfo.h>

// ============================================================================
// QiForm
// ============================================================================

QiForm::QiForm(QObject *parent) : QObject(parent) {
}

QiForm::~QiForm() {
    delete m_record;
}

void QiForm::setModel(const QString &name) {
    if (name == m_modelName)
        return;
    m_modelName = name;
    m_info = qiFindModel(name);
    if (!m_info)
        qWarning().noquote() << QStringLiteral("QiForm: no model \"%1\" (register it with addModel<%1>() first)").arg(name);

    m_fields.clear();
    m_labels.clear();
    m_required.clear();
    m_choices.clear();
    m_maxLengths.clear();
    if (m_info) {
        for (int i = 0; i < m_info->size(); ++i) {
            const QiModelMetaInfoField *f = m_info->at(i);
            if (f->name == QLatin1String("id"))
                continue;
            QiClause clause = f->clause;
            m_fields << f->name;
            m_labels.insert(f->name, clause.testFlag(QiClause::LABEL) ? clause.flag(QiClause::LABEL).toString() : qiHumanize(f->name));
            bool required = clause.testFlag(QiClause::NOT_NULL) && !clause.testFlag(QiClause::DEFAULT);
            for (const QiRule &r : clause.rules()) {
                if (r.name == QLatin1String("required") && !r.condition && r.contexts.isEmpty())
                    required = true;
                if (r.name == QLatin1String("oneOf"))
                    m_choices.insert(f->name, r.params.value(QStringLiteral("choices")));
                if (r.name == QLatin1String("length") && r.params.value(QStringLiteral("max")).toInt() > 0)
                    m_maxLengths.insert(f->name, r.params.value(QStringLiteral("max")));
            }
            m_required.insert(f->name, required);
        }
    }
    reset();
    emit modelChanged();
}

void QiForm::setContext(const QString &context) {
    if (context == m_context)
        return;
    m_context = context;
    emit contextChanged();
}

void QiForm::setValidators(const QJSValue &validators) {
    m_validators = validators;
    emit validatorsChanged();
    check(false);
}

// The JavaScript validators: function(value, values) -> "" | "message" | { message, warning }.
void QiForm::runScriptValidators() {
    if (!m_validators.isObject())
        return;
    QJSEngine *engine = qjsEngine(this);
    if (!engine)
        return;
    const QJSValue values = engine->toScriptValue(m_values);
    QJSValueIterator it(m_validators);
    while (it.hasNext()) {
        it.next();
        QJSValue fn = it.value();
        if (!fn.isCallable())
            continue;
        const QString field = it.name();
        const QJSValue result = fn.call(QJSValueList() << engine->toScriptValue(m_values.value(field)) << values);
        if (result.isError()) {
            qWarning().noquote() << QStringLiteral("QiForm(%1): the validator for \"%2\" threw: %3").arg(m_modelName, field, result.toString());
            continue;
        }
        QString message;
        bool warning = false;
        if (result.isString()) {
            message = result.toString();
        } else if (result.isObject()) {
            message = result.property(QStringLiteral("message")).toString();
            warning = result.property(QStringLiteral("warning")).toBool();
        }
        if (message.isEmpty())
            continue;
        if (warning) m_validation.addWarning(field, message, QStringLiteral("script"));
        else         m_validation.addError(field, message, QStringLiteral("script"));
    }
}

int QiForm::indexOf(const QString &field) const {
    if (!m_info)
        return -1;
    for (int i = 0; i < m_info->size(); ++i)
        if (m_info->at(i)->name == field)
            return i;
    return -1;
}

QVariant QiForm::forDisplay(const QVariant &value) const {
    if (value.isNull())
        return QVariant();
    switch (value.userType()) {
    case QMetaType::QDate: return value.toDate().toString(Qt::ISODate);
    case QMetaType::QTime: return value.toTime().toString(QStringLiteral("HH:mm"));
    case QMetaType::QDateTime: {
        const QDateTime dt = value.toDateTime();
        return dt.toString(dt.time().second() ? QStringLiteral("yyyy-MM-dd HH:mm:ss") : QStringLiteral("yyyy-MM-dd HH:mm"));
    }
    default: return value;
    }
}

QVariant QiForm::toFieldType(int index, const QVariant &in, QString *problem) const {
    const int type = m_info->at(index)->type;
    QVariant value = in;
    if (value.userType() == qMetaTypeId<QJSValue>())
        value = value.value<QJSValue>().toVariant();
    const bool text = value.userType() == QMetaType::QString;
    if (value.isNull() || !value.isValid() || (text && value.toString().trimmed().isEmpty() && type != QMetaType::QString))
        return QVariant();

    auto number = [&](bool whole) -> QVariant {
        bool ok = false;
        double d = 0;
        if (text) {
            const QString s = value.toString().trimmed();
            d = QLocale().toDouble(s, &ok);
            if (!ok) d = s.toDouble(&ok);                 // "3.5" whatever the locale
        } else {
            d = value.toDouble(&ok);
        }
        if (!ok || (whole && d != double(qint64(d)))) {
            *problem = whole ? QStringLiteral("must be a whole number") : QStringLiteral("must be a number");
            return QVariant();
        }
        return whole ? QVariant(qint64(d)) : QVariant(d);
    };
    switch (type) {
    case QMetaType::QString:   return value.toString();
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong: case QMetaType::ULongLong:
    case QMetaType::Short: case QMetaType::UShort:
        return number(true);
    case QMetaType::Double: case QMetaType::Float:
        return number(false);
    case QMetaType::Bool:      return value.toBool();
    case QMetaType::QDate: case QMetaType::QDateTime: case QMetaType::QTime: {
        const QDateTime dt = qiDateTimeOf(value);
        if (!dt.isValid()) {
            *problem = QStringLiteral("is not a valid date");
            return QVariant();
        }
        if (type == QMetaType::QDate) return dt.date();
        if (type == QMetaType::QTime) return dt.time();
        return dt;
    }
    default:                   return value;
    }
}

void QiForm::readRecord() {
    m_values.clear();
    if (m_record)
        for (const QString &f : m_fields)
            m_values.insert(f, forDisplay(m_info->value(m_record, f)));
    m_dirty = false;
}

void QiForm::check(bool database, const QStringList &also) {
    if (!m_record)
        return;
    QiConnection conn = m_record->connection();
    QiValidator::Options fast;
    fast.context = m_context;
    fast.database = false;
    fast.uniqueKeys = false;
    fast.normalize = true;            // the record is normalised; `values` keeps what was typed
    m_validation = QiValidator::validate(m_record, m_info, &conn, fast);

    if (database && !also.isEmpty()) {
        // The database rules' verdicts on these fields, kept until the field changes.
        QiValidator::Options slow = fast;
        slow.fields = also;
        slow.database = true;
        slow.uniqueKeys = true;
        QiValidator::Options same = fast;
        same.fields = also;
        const QiValidation with = QiValidator::validate(m_record, m_info, &conn, slow);
        const QiValidation without = QiValidator::validate(m_record, m_info, &conn, same);
        for (const QString &f : also) {
            QList<QiFieldError> fromDb;
            for (const QiFieldError &e : with.all()) {
                bool known = false;
                for (const QiFieldError &x : without.all())
                    known = known || (x.field == e.field && x.rule == e.rule && x.message == e.message);
                if (e.field == f && !known)
                    fromDb << e;
            }
            m_dbErrors.insert(f, fromDb);
        }
    }
    for (auto it = m_dbErrors.constBegin(); it != m_dbErrors.constEnd(); ++it)
        for (const QiFieldError &e : it.value())
            m_validation.add(e);
    for (auto it = m_parseErrors.constBegin(); it != m_parseErrors.constEnd(); ++it) {
        m_validation.clear(it.key());             // "not a number" says it all
        m_validation.addError(it.key(), QCoreApplication::translate("QiValidation", it.value().toUtf8().constData()), QStringLiteral("type"));
    }
    for (auto it = m_ownErrors.constBegin(); it != m_ownErrors.constEnd(); ++it)
        m_validation.addError(it.key(), it.value());
    runScriptValidators();
    emit validationChanged();
}

void QiForm::set(const QString &field, const QVariant &value) {
    const int index = indexOf(field);
    if (index < 0) {
        qWarning().noquote() << QStringLiteral("QiForm(%1): no field \"%2\"").arg(m_modelName, field);
        return;
    }
    QVariant shown = value;
    if (shown.userType() == qMetaTypeId<QJSValue>())
        shown = shown.value<QJSValue>().toVariant();
    if (m_values.value(field) == shown && m_values.contains(field))
        return;
    m_values.insert(field, shown);
    m_dirty = true;

    QString problem;
    const QVariant typed = toFieldType(index, shown, &problem);
    if (problem.isEmpty()) m_parseErrors.remove(field);
    else m_parseErrors.insert(field, problem);
    m_info->setValue(m_record, field, typed);
    m_dbErrors.remove(field);
    m_ownErrors.remove(field);
    m_error.clear();

    emit valuesChanged();
    check(false);
}

void QiForm::touch(const QString &field) {
    if (!m_record || indexOf(field) < 0)
        return;
    m_touched.insert(field);
    check(true, QStringList(field));
}

bool QiForm::validate(const QString &context) {
    if (!m_record)
        return false;
    m_submitted = true;
    m_dbErrors.clear();
    m_record->validate(context.isEmpty() ? m_context : context);    // the rules, the database, clean()
    m_validation = m_record->validation();
    for (auto it = m_parseErrors.constBegin(); it != m_parseErrors.constEnd(); ++it) {
        m_validation.clear(it.key());
        m_validation.addError(it.key(), QCoreApplication::translate("QiValidation", it.value().toUtf8().constData()), QStringLiteral("type"));
    }
    for (auto it = m_ownErrors.constBegin(); it != m_ownErrors.constEnd(); ++it)
        m_validation.addError(it.key(), it.value());
    runScriptValidators();
    emit validationChanged();
    return m_validation.isValid();
}

bool QiForm::validateFields(const QStringList &fields) {
    for (const QString &f : fields)
        m_touched.insert(f);
    check(true, fields);
    for (const QString &f : fields)
        if (m_validation.hasError(f))
            return false;
    return true;
}

bool QiForm::submit() {
    if (!validate()) {
        emit failed(m_validation.summary());
        return false;
    }
    if (!m_record->save()) {
        m_validation = m_record->validation();       // a database error on a field comes back here
        m_error = m_validation.isValid() ? m_record->lastError().text() : QString();
        emit validationChanged();
        emit failed(m_error.isEmpty() ? m_validation.summary() : m_error);
        return false;
    }
    readRecord();                                     // as saved: trimmed, lower-cased, …
    m_touched.clear();
    m_submitted = false;
    m_dbErrors.clear();
    m_validation.clear();
    emit valuesChanged();
    emit recordChanged();
    emit validationChanged();
    emit saved(recordId());
    return true;
}

bool QiForm::load(const QVariant &id) {
    if (!m_info)
        return false;
    reset();
    const QString pk = m_info->primaryKeyName().isEmpty() ? QStringLiteral("id") : m_info->primaryKeyName();
    const bool ok = m_record->load(QiWhere(pk + QStringLiteral(" = "), id));
    readRecord();
    emit valuesChanged();
    emit recordChanged();
    emit validationChanged();
    return ok;
}

void QiForm::reset() {
    delete m_record;
    m_record = m_info ? static_cast<QiModel *>(m_info->create()) : nullptr;
    m_touched.clear();
    m_parseErrors.clear();
    m_dbErrors.clear();
    m_ownErrors.clear();
    m_submitted = false;
    m_error.clear();
    readRecord();
    emit valuesChanged();
    emit recordChanged();
    check(false);
}

void QiForm::setError(const QString &field, const QString &message) {
    m_ownErrors.insert(field, message);
    m_touched.insert(field);
    check(false);
}

QVariantMap QiForm::errors() const {
    QVariantMap shown;
    const QVariantMap all = m_validation.errorMap();
    for (auto it = all.constBegin(); it != all.constEnd(); ++it)
        if (m_submitted || m_touched.contains(it.key()) || it.key().isEmpty())
            shown.insert(it.key(), it.value());
    return shown;
}

QVariantMap QiForm::allErrors() const {
    return m_validation.errorMap();
}

QVariantMap QiForm::warnings() const {
    QVariantMap shown;
    const QVariantMap all = m_validation.warningMap();
    for (auto it = all.constBegin(); it != all.constEnd(); ++it)
        if (m_submitted || m_touched.contains(it.key()))
            shown.insert(it.key(), it.value());
    return shown;
}

QVariantMap QiForm::visited() const {
    QVariantMap out;
    for (const QString &f : m_fields)
        if (m_submitted || m_touched.contains(f))
            out.insert(f, true);
    return out;
}

bool QiForm::valid() const {
    return m_validation.isValid();
}

QString QiForm::summary() const {
    QStringList lines;
    for (const QiFieldError &e : m_validation.errors()) {
        if (!(m_submitted || m_touched.contains(e.field) || e.field.isEmpty()))
            continue;
        lines << (e.field.isEmpty() ? e.message : m_labels.value(e.field, qiHumanize(e.field)).toString() + QStringLiteral(": ") + e.message);
    }
    if (!m_error.isEmpty())
        lines << m_error;
    return lines.join(QLatin1Char('\n'));
}

QVariant QiForm::recordId() const {
    if (!m_record)
        return QVariant();
    const QString pk = m_info->primaryKeyName().isEmpty() ? QStringLiteral("id") : m_info->primaryKeyName();
    return m_info->value(m_record, pk);
}

bool QiForm::isNew() const {
    return recordId().isNull();
}

QiFormAttached *QiForm::qmlAttachedProperties(QObject *object) {
    return new QiFormAttached(object);
}

// ============================================================================
// QiFormAttached
// ============================================================================

QiFormAttached::QiFormAttached(QObject *target) : QObject(target), m_target(target) {
}

void QiFormAttached::setForm(QiForm *form) {
    if (form == m_form)
        return;
    m_form = form;
    bind();
    emit formChanged();
    emit stateChanged();
}

void QiFormAttached::setField(const QString &field) {
    if (field == m_field)
        return;
    m_field = field;
    bind();
    emit formChanged();
    emit stateChanged();
}

void QiFormAttached::setProperty(const QString &property) {
    if (property == m_property)
        return;
    m_property = property;
    bind();
    emit formChanged();
}

QString QiFormAttached::boundProperty() const {
    if (!m_property.isEmpty())
        return m_property;
    const QMetaObject *mo = m_target->metaObject();
    auto has = [mo](const char *name) { return mo->indexOfProperty(name) >= 0; };
    if (has("checkable") && m_target->property("checkable").toBool() && has("checked")) return QStringLiteral("checked");
    if (has("currentText") && has("currentIndex")) return QStringLiteral("currentText");   // ComboBox
    if (has("value") && has("from")) return QStringLiteral("value");                       // SpinBox, Slider
    if (has("text")) return QStringLiteral("text");                                        // TextField, TextArea
    if (has("checked")) return QStringLiteral("checked");
    return QString();
}

void QiFormAttached::bind() {
    for (const QMetaObject::Connection &c : m_connections)
        QObject::disconnect(c);
    m_connections.clear();
    if (!m_form || m_field.isEmpty() || !m_target)
        return;

    m_bound = boundProperty();
    const QMetaObject *mo = m_target->metaObject();
    const int index = mo->indexOfProperty(m_bound.toUtf8().constData());
    if (index < 0) {
        qWarning().noquote() << QStringLiteral("QiForm.field \"%1\": don't know which property of %2 to bind (set QiForm.property)")
                                    .arg(m_field, QString::fromUtf8(mo->className()));
        return;
    }
    const QMetaMethod changed = metaObject()->method(metaObject()->indexOfSlot("controlChanged()"));
    const QMetaProperty p = mo->property(index);
    if (p.hasNotifySignal())
        m_connections << QObject::connect(m_target, p.notifySignal(), this, changed);
    const int focus = mo->indexOfProperty("activeFocus");
    if (focus >= 0 && mo->property(focus).hasNotifySignal())
        m_connections << QObject::connect(m_target, mo->property(focus).notifySignal(), this,
                                          metaObject()->method(metaObject()->indexOfSlot("focusChanged()")));
    m_connections << QObject::connect(m_form, &QiForm::valuesChanged, this, &QiFormAttached::formValuesChanged);
    m_connections << QObject::connect(m_form, &QiForm::validationChanged, this, &QiFormAttached::stateChanged);
    m_connections << QObject::connect(m_form, &QiForm::modelChanged, this, &QiFormAttached::stateChanged);
    formValuesChanged();
}

void QiFormAttached::controlChanged() {
    if (m_writing || !m_form)
        return;
    m_form->set(m_field, m_target->property(m_bound.toUtf8().constData()));
}

void QiFormAttached::focusChanged() {
    if (m_form && !m_target->property("activeFocus").toBool())
        m_form->touch(m_field);
}

void QiFormAttached::formValuesChanged() {
    if (!m_form || m_bound.isEmpty())
        return;
    const QVariant v = m_form->value(m_field);
    const QByteArray name = m_bound.toUtf8();
    if (m_target->property(name.constData()) == v || (v.isNull() && m_target->property(name.constData()).toString().isEmpty()))
        return;
    m_writing = true;
    if (m_bound == QLatin1String("currentText")) {               // a ComboBox: pick the entry
        int found = -1;
        QMetaObject::invokeMethod(m_target, "find", Q_RETURN_ARG(int, found), Q_ARG(QString, v.toString()));
        m_target->setProperty("currentIndex", found);
    } else if (m_bound == QLatin1String("text")) {
        m_target->setProperty(name.constData(), v.isNull() ? QString() : v.toString());
    } else {
        m_target->setProperty(name.constData(), v);
    }
    m_writing = false;
}

QString QiFormAttached::error() const { return m_form ? m_form->errors().value(m_field).toString() : QString(); }
QString QiFormAttached::warning() const { return m_form ? m_form->warnings().value(m_field).toString() : QString(); }
QString QiFormAttached::label() const { return m_form ? m_form->labels().value(m_field).toString() : QString(); }
bool QiFormAttached::required() const { return m_form && m_form->required().value(m_field).toBool(); }

// ============================================================================

void qiRegisterQml(QQmlEngine *engine) {
    Q_INIT_RESOURCE(qivotqml);        // the Qivot.Forms controls, even from a static library
    qmlRegisterType<QiForm>("Qivot", 1, 0, "QiForm");
    if (engine)
        engine->addImportPath(QStringLiteral("qrc:/qivot/qml"));
}
