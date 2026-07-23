/** Relations & model conveniences — many-to-many, custom types, lifecycle
    hooks, auto timestamps, and soft delete, in one small program.

    A tiny photo library:
      - Photo has a custom GeoPoint `location` (a value type stored as TEXT),
        auto createdAt/updatedAt, a deletedAt tombstone, a clean() validator,
        and an afterSave() hook.
      - Photos and Tags are linked many-to-many through a `photo_tag` join table.
 */
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <qivot.h>

// --- #5 Custom type converter ---------------------------------------------

struct GeoPoint { double lat = 0.0, lng = 0.0; };
Q_DECLARE_METATYPE(GeoPoint)

static QVariant geoToStorage(const GeoPoint &p) {
    return QStringLiteral("%1,%2").arg(p.lat).arg(p.lng);   // stored as "lat,lng"
}
static GeoPoint geoFromStorage(const QVariant &v) {
    const QStringList s = v.toString().split(',');
    return GeoPoint{ s.value(0).toDouble(), s.value(1).toDouble() };
}
QI_DECLARE_CONVERTER(GeoPoint, geoToStorage, geoFromStorage)

// --- Models ----------------------------------------------------------------

class Photo : public QiModel {
    QI_MODEL
public:
    QiField<QString>   title;
    QiField<GeoPoint>  location;    // custom type (TEXT column)
    QiField<QDateTime> createdAt;   // auto-set on insert  (#7)
    QiField<QDateTime> updatedAt;   // auto-set on every save (#7)
    QiField<QDateTime> deletedAt;   // soft-delete tombstone (#7)

    // #6 validation hook
    bool clean() override {
        if (title->toString().trimmed().isEmpty()) {
            setError(QStringLiteral("title must not be empty"));
            return false;
        }
        return true;
    }
    // #6 lifecycle hook
    void afterSave(bool created) override {
        qInfo().noquote() << (created ? "   [afterSave] inserted #" : "   [afterSave] updated  #")
                          + QString::number(id->toInt()) << title->toString();
    }
};
QI_DECLARE_MODEL(Photo, "photo",
                 QI_FIELD(title, QiNotNull),
                 QI_FIELD_AS(location, "TEXT"),
                 QI_FIELD(createdAt),
                 QI_FIELD(updatedAt),
                 QI_FIELD(deletedAt));

class Tag : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
};
QI_DECLARE_MODEL(Tag, "tag", QI_FIELD(name, QiNotNull | QiUnique));

// The join table for the Photo <-> Tag many-to-many relation.
class PhotoTag : public QiModel {
    QI_MODEL
public:
    QiField<int> photoId;
    QiField<int> tagId;
};
QI_DECLARE_MODEL_NOID(PhotoTag, "photo_tag",
                      QI_FIELD(photoId, QiPrimary | QiNotNull),
                      QI_FIELD(tagId,   QiPrimary | QiNotNull));

static void banner(const QString &t) { qInfo().noquote() << "\n==== " << t << " ===="; }

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    db.open();
    QiConnection conn;
    if (!conn.open(db)) return 1;
    conn.addModel<Photo>();
    conn.addModel<Tag>();
    conn.addModel<PhotoTag>();
    if (!conn.createTables()) return 1;

    // --- #5 custom type + #7 auto timestamps + #6 afterSave hook -----------
    banner("Save a photo (custom GeoPoint, auto timestamps, hook)");
    Photo p;
    p.title    = "Golden Gate at dusk";
    p.location = QVariant::fromValue(GeoPoint{ 37.8199, -122.4783 });
    p.save();
    qInfo().noquote() << "  createdAt:" << p.createdAt->toDateTime().toString(Qt::ISODate);
    qInfo().noquote() << "  updatedAt:" << p.updatedAt->toDateTime().toString(Qt::ISODate);

    // Reload and read the custom type back out.
    Photo loaded;
    loaded.load(Photo::col().id == p.id());
    const GeoPoint g = qvariant_cast<GeoPoint>(loaded.location());
    qInfo().noquote() << "  reloaded location:" << g.lat << "," << g.lng;

    // --- #6 validation rejects an invalid record ---------------------------
    banner("clean() rejects an empty title");
    Photo bad;
    bad.title = "   ";
    if (!bad.save())
        qInfo().noquote() << "  refused:" << bad.lastError().text();

    // --- #4 many-to-many ---------------------------------------------------
    banner("Tag the photo (many-to-many)");
    Tag nature;  nature.name  = "nature";  nature.save();
    Tag evening; evening.name = "evening"; evening.save();
    Tag city;    city.name    = "city";    city.save();

    qiAttach(p, nature,  "photo_tag", "photoId", "tagId");
    qiAttach(p, evening, "photo_tag", "photoId", "tagId");
    qiAttach(p, city,    "photo_tag", "photoId", "tagId");
    qiDetach(p, city,    "photo_tag", "photoId", "tagId");   // changed our mind

    QiList<Tag> tags = qiManyToMany<Tag>(p, "photo_tag", "photoId", "tagId");
    QStringList names;
    for (int i = 0; i < tags.size(); i++) names << tags.at(i)->name->toString();
    qInfo().noquote() << "  tags on photo:" << names.join(", ");

    // --- #7 soft delete ----------------------------------------------------
    banner("Soft delete");
    Photo p2; p2.title = "Blurry test shot"; p2.save();

    qInfo().noquote() << "  alive before:" << qiAlive<Photo>().count()
                      << " total rows:" << Photo::objects().count();
    if (!p2.softRemove()) qInfo() << p2.lastError().text();
    qInfo().noquote() << "  alive after :" << qiAlive<Photo>().count()
                      << " trashed:" << qiTrashed<Photo>().count()
                      << " total rows:" << Photo::objects().count();
    qInfo().noquote() << "  (the row is still there — just tombstoned)";

    return 0;
}
