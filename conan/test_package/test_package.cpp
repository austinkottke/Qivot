// Smallest program that exercises an installed Qivot: declare a model, open an
// in-memory SQLite database, create the table, save a row and read it back.
#include <qivot.h>

#include <QCoreApplication>
#include <QSqlDatabase>
#include <cstdio>

class Note : public QiModel {
    QI_MODEL
public:
    QiField<QString> text;
};

QI_DECLARE_MODEL(Note, "note", QI_FIELD(text, QiNotNull));

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        // The package built and linked; the SQLite plugin is Qt's business.
        std::puts("qivot: linked (QSQLITE driver not available, skipping the round trip)");
        return 0;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open())
        return 1;

    QiConnection connection;
    if (!connection.open(db))
        return 1;
    connection.addModel<Note>();
    if (!connection.createTables())
        return 1;

    Note note;
    note.text = "hello from the package";
    note.save();

    const int count = Note::objects().count();
    connection.close();

    std::printf("qivot: saved and counted %d row(s)\n", count);
    return count == 1 ? 0 : 1;
}
