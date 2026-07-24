#include <QtCore>
#include "qifield.h"

// The QiField<QStringList/QJsonObject/QJsonArray> specializations now live inline
// in qifield.h (see the comment there) so MSVC emits them in every translation
// unit. Only QiPrimaryKey remains a normal out-of-line definition here.

QiPrimaryKey::QiPrimaryKey(){
}

QiClause QiPrimaryKey::clause(){
    return QiClause(QiClause::PRIMARY_KEY);
}
