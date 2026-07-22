#include "qisharedlist.h"
#include <QSharedData>
#include <QList>
#include <QMap>
#include "qimodel.h"
#include "qiconnection.h"
#include "qisql.h"

class QiSharedListPriv : public QSharedData {
public:
    QiSharedListPriv() {
        metaInfo = 0;
    }

    ~QiSharedListPriv() {
        clear();
    }

    void clear(){
        foreach (QiAbstractModel*model, list){
            delete model;
        }
        list.clear();
        metaInfo = 0;
    }

    QList <QiAbstractModel*> list;
    QiModelMetaInfo *metaInfo;
};

QiSharedList::QiSharedList() : data(new QiSharedListPriv){
}

QiSharedList::QiSharedList(const QiSharedList &rhs) : data(rhs.data)
{
}

QiSharedList &QiSharedList::operator=(const QiSharedList &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QiSharedList::~QiSharedList()
{
}

int QiSharedList::size() const{
    return data->list.size();
}

QiAbstractModel* QiSharedList::at(int i) const{
    return data->list.value(i);
}

bool QiSharedList::append(QiAbstractModel* model){
    if (data->metaInfo &&
        model->metaInfo() != data->metaInfo) {
        return false;
    }

    data->list << model;
    return true;
}

void QiSharedList::clear() {
    data->clear();
}

void QiSharedList::removeAt(int index){
    QiAbstractModel *model = data->list.value(index);
    data->list.removeAt(index);
    delete model;
}

bool QiSharedList::save(bool forceInsert,bool forceAllField) {
    int n = size();
    if (n == 0)
        return true;

    // Group records by (model, exact column set) so each group shares one
    // prepared statement, and run the whole batch inside a single transaction.
    QList<QiModelMetaInfo*> groupMeta;
    QList<QStringList>      groupFields;
    QList<QList<QiModel*> > groups;
    QMap<QString,int>       index;

    for (int i = 0 ; i < n ; i++) {
        QiModel *model = static_cast<QiModel*>(at(i));
        if (!model->clean())          // clean() may mutate fields, so run it first
            return false;

        QiModelMetaInfo *info = model->metaInfo();
        QStringList allFields = info->fieldNameList();
        QStringList fields;
        if (forceAllField) {
            fields = allFields;
        } else {
            foreach (QString field , allFields) {
                if (forceInsert && field == "id")
                    continue;
                if (!info->value(model,field).isNull())
                    fields << field;
            }
        }

        QString sig = QString::number((quintptr) info) + "|" + fields.join(",");
        if (!index.contains(sig)) {
            index[sig] = groups.size();
            groupMeta   << info;
            groupFields << fields;
            groups      << QList<QiModel*>();
        }
        groups[index[sig]].append(model);
    }

    QiConnection connection = static_cast<QiModel*>(at(0))->connection();

    bool ownTransaction = connection.transaction();
    bool res = true;

    for (int g = 0 ; g < groups.size() ; g++) {
        if (!connection.sql().insertIntoBatch(groupMeta.at(g), groups.at(g), groupFields.at(g), true)) {
            res = false;
            break;
        }
    }

    if (ownTransaction) {
        if (res) {
            if (!connection.commit())
                res = false;
        } else {
            connection.rollback();
        }
    }

    return res;
}

QiModelMetaInfo* QiSharedList::metaInfo(){
    return data->metaInfo;
}

void QiSharedList::setMetaInfo(QiModelMetaInfo* metaInfo){
    data->metaInfo = metaInfo;
}
