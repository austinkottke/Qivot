#define QIVOT_IMPLEMENTATION
#include <qivot.hpp>
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QDebug>

/// A cut-down stand-in for ninjaproxy's Tick: a symbol column and a time column.
class Row : public QiModel {
    QI_MODEL
public:
    QiField<QString> instrument;
    QiField<qint64>  tsNs;
};
QI_DECLARE_MODEL(Row, "row", QI_FIELD(instrument), QI_FIELD(tsNs));

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open())                 { qWarning("db.open failed"); return 2; }
    QiConnection connection;
    if (!connection.open(db))       { qWarning("connection.open failed"); return 2; }
    connection.addModel<Row>();
    if (!connection.createTables()) { qWarning("createTables failed"); return 2; }

    // Two symbols overlapping in tsNs — the shape of the real table, where the
    // filter is the only thing separating them.
    for (int i = 0; i < 5; ++i) { Row r; r.instrument = "MNQ 09-26"; r.tsNs = i; r.save(); }
    for (int i = 0; i < 7; ++i) { Row r; r.instrument = "NG 10-26";  r.tsNs = i; r.save(); }

    QSqlQuery raw(db);
    raw.exec("SELECT COUNT(*) FROM row WHERE instrument='MNQ 09-26' AND tsNs>=0");
    raw.next();
    const int truth = raw.value(0).toInt();

    const int chained = Row::objects()
                            .filter(Row::col().instrument == QString("MNQ 09-26"))
                            .filter(Row::col().tsNs >= qint64(0))
                            .count();
    const int single  = Row::objects()
                            .filter(Row::col().instrument == QString("MNQ 09-26")
                                    && Row::col().tsNs >= qint64(0))
                            .count();

    qInfo().noquote() << "rows in table   : 12  (5 MNQ + 7 NG)";
    qInfo().noquote() << "raw SQL         :" << truth;
    qInfo().noquote() << "single &&filter :" << single;
    qInfo().noquote() << "CHAINED .filter :" << chained;
    const bool ok = (chained == truth && single == truth);
    qInfo().noquote() << (ok ? "PASS - a chained filter keeps both clauses"
                             : "FAIL - a chained filter was silently dropped");
    return ok ? 0 : 1;
}
