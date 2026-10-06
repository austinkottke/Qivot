// Release smoke test: the single header compiles, links against Qt Core + Sql
// only, and works. tools/package-release.sh builds and runs it before packaging.
#define QIVOT_IMPLEMENTATION
#include "qivot.hpp"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <cstdio>

class Note : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
    QiField<int>     stars;
};
QI_DECLARE_MODEL(Note, "note", QI_FIELD(title, QiRequired()), QI_FIELD(stars));

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) { std::printf("FAIL: open\n"); return 1; }

    QiConnection conn;
    if (!conn.open(db)) { std::printf("FAIL: QiConnection::open\n"); return 1; }
    conn.addModel<Note>();
    if (!conn.createTables()) { std::printf("FAIL: createTables\n"); return 1; }

    for (int i = 1; i <= 3; i++) {
        Note n; n.title = QString("note %1").arg(i); n.stars = i;
        if (!n.save()) { std::printf("FAIL: save\n"); return 1; }
    }
    Note bad;
    if (bad.validate().isValid()) { std::printf("FAIL: validation\n"); return 1; }

    const int good = Note::objects().filter(Note::col().stars >= 2).count();
    Note top;
    top.load(Note::col().stars == 3);
    top.title = "edited";
    const bool edited = top.save();
    const bool ok = good == 2 && edited && Note::objects().filter(Note::col().title == "edited").count() == 1;
    std::printf("%s: single header works (%d of 3 rows matched, edit saved: %d)\n",
                ok ? "PASS" : "FAIL", good, edited);
    return ok ? 0 : 1;
}
