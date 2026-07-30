#ifndef PRODUCT_H
#define PRODUCT_H

#include <qivot.h>
#include <QJsonArray>

/// A tiny product catalog row. The point of this example is that this ONE model
/// definition — and all the code that uses it — works unchanged on SQLite, MySQL,
/// and PostgreSQL. Qivot maps each field to the right column type per backend:
///
///   field     SQLite    MySQL            PostgreSQL
///   -------   -------   --------------   ----------
///   sku       TEXT      VARCHAR(255)*    TEXT        (*VARCHAR because it's UNIQUE)
///   name      TEXT      TEXT             TEXT
///   price     DOUBLE    DOUBLE           DOUBLE PRECISION
///   inStock   BOOLEAN   TINYINT(1)       BOOLEAN
///   tags      TEXT      JSON             JSONB
class Product : public QiModel {
    QI_MODEL
public:
    QiField<QString>    sku;       // a unique product code
    QiField<QString>    name;
    QiField<double>     price;
    QiField<bool>       inStock;
    QiField<QJsonArray> tags;
};

QI_DECLARE_MODEL(Product, "product",
    QI_FIELD(sku, QiUnique | QiNotNull),
    QI_FIELD(name),
    QI_FIELD(price),
    QI_FIELD(inStock),
    QI_FIELD(tags));

#endif // PRODUCT_H
