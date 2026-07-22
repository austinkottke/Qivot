#include <QtCore>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "qifield.h"

/// Separator for StringList storage
#define SEP " & "

/// Replace & and " by HTML entities.

static QString escape(QString value) {
    QString result;
    QMap<char , QString> map;
    map['&'] = "&amp;";
    map['"'] = "&quot;";

    result.reserve(value.size() * 1.1);
    int n = value.size();
    for (int i = 0 ; i < n;i++) {
        QChar c = value.at(i);
        char l = c.toLatin1();
        if (map.contains(l)) {
            result += map[l];
        } else {
            result += c;
        }
    }
    result.squeeze();
    return result;
}

static QString unescape(QString value) {
    QMap<QString,char> map;
    map["&amp;"] = '&';
    map["&quot;"] = '"';

    QString result;
    result.reserve(value.size());

    int n = value.size();

    for (int i = 0 ; i<n;i++){
        QChar q = value.at(i) ;

        if (q == '&') {
            bool found = false;
            QMap<QString, char>::const_iterator iter = map.constBegin();
            while (iter!= map.end()) {
                QString key = iter.key();
                int len = key.size();

                QStringView ref = QStringView(value).mid(i,len);
                if (ref == QStringView(key)) {
                    q = iter.value();
                    i+=len-1;
                    found = true;
                    break;
                }
                iter++;
            }

            if (!found) {
                qWarning() << QString("Invalid escaped string : %1").arg(value);
            }
        }

        result += q;
    }

    result.squeeze();
    return result;
}

template <>
bool QiField<QStringList>::set(QVariant value){
    QString str;

    if (value.userType() == QMetaType::QString) {
        str = value.toString();
        QStringList list = str.split(SEP);
        QStringList result;

        foreach (str,list) {
            result << unescape(str);
        }

        value = result;
    }

//    qDebug() << __func__ << str;
    return QiBaseField::set(value);;
}

template <>
QVariant QiField<QStringList>::get(bool convert) const {
    QVariant val = QiBaseField::get(convert);

    if (convert && val.userType() == QMetaType::QStringList ) {
        QStringList list = val.toStringList();
        QStringList result;
        QString str;

        foreach (str,list) {
            result << escape(str);
        }

        str = result.join(SEP);
        val = str;
    }
//    qDebug() << __func__ << str << list;

    return val;
}


// --- Structured JSON fields ------------------------------------------------
// In memory the field holds a QJsonObject/QJsonArray so you can read and mutate
// the nested structure; get(convert=true) serializes it to a compact JSON string
// for the TEXT column, and set() parses that string back on load.

template <>
bool QiField<QJsonObject>::set(QVariant value) {
    if (value.userType() == QMetaType::QString)
        value = QJsonDocument::fromJson(value.toString().toUtf8()).object();
    return QiBaseField::set(value);
}

template <>
QVariant QiField<QJsonObject>::get(bool convert) const {
    QVariant val = QiBaseField::get(convert);
    if (convert && val.userType() == QMetaType::QJsonObject)
        return QString::fromUtf8(QJsonDocument(val.toJsonObject()).toJson(QJsonDocument::Compact));
    return val;
}

template <>
bool QiField<QJsonArray>::set(QVariant value) {
    if (value.userType() == QMetaType::QString)
        value = QJsonDocument::fromJson(value.toString().toUtf8()).array();
    return QiBaseField::set(value);
}

template <>
QVariant QiField<QJsonArray>::get(bool convert) const {
    QVariant val = QiBaseField::get(convert);
    if (convert && val.userType() == QMetaType::QJsonArray)
        return QString::fromUtf8(QJsonDocument(val.toJsonArray()).toJson(QJsonDocument::Compact));
    return val;
}


QiPrimaryKey::QiPrimaryKey(){
}

QiClause QiPrimaryKey::clause(){
    return QiClause(QiClause::PRIMARY_KEY);
}
