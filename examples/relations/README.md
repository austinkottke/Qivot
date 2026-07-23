# Tutorial — relations & model conveniences

Five features that make models feel first-class, in one small photo-library
program:

- **many-to-many** relations through a join table,
- **custom type converters** (store any value type in a column),
- **lifecycle hooks** (`clean()` validation, `afterSave()`),
- **auto timestamps** (`createdAt` / `updatedAt`),
- **soft delete** (a `deletedAt` tombstone + query scopes).

> **Run it**
> ```sh
> cd examples/relations
> qmake && make
> ./relations
> ```

---

## Step 1 — A custom type converter

Teach Qivot to store a value type by giving it two functions and the
`QI_DECLARE_CONVERTER` macro. Register the type with `Q_DECLARE_METATYPE`, and
set the column's SQL type with `QI_FIELD_AS`.

```cpp
struct GeoPoint { double lat = 0, lng = 0; };
Q_DECLARE_METATYPE(GeoPoint)

static QVariant geoToStorage(const GeoPoint &p) {           // -> "lat,lng"
    return QStringLiteral("%1,%2").arg(p.lat).arg(p.lng);
}
static GeoPoint geoFromStorage(const QVariant &v) {         // <- "lat,lng"
    const QStringList s = v.toString().split(',');
    return GeoPoint{ s.value(0).toDouble(), s.value(1).toDouble() };
}
QI_DECLARE_CONVERTER(GeoPoint, geoToStorage, geoFromStorage)
```

## Step 2 — A model with timestamps, soft delete, and hooks

Declare conventional timestamp columns and a `deletedAt` tombstone; override
`clean()` to validate and `afterSave()` to react.

```cpp
class Photo : public QiModel {
    QI_MODEL
public:
    QiField<QString>   title;
    QiField<GeoPoint>  location;    // custom type
    QiField<QDateTime> createdAt;   // auto-set on insert
    QiField<QDateTime> updatedAt;   // auto-set on every save
    QiField<QDateTime> deletedAt;   // soft-delete tombstone

    bool clean() override {                                  // validation hook
        if (title->toString().trimmed().isEmpty()) {
            setError("title must not be empty");
            return false;
        }
        return true;
    }
    void afterSave(bool created) override {                  // lifecycle hook
        qInfo() << (created ? "inserted" : "updated") << id->toInt();
    }
};
QI_DECLARE_MODEL(Photo, "photo",
                 QI_FIELD(title, QiNotNull),
                 QI_FIELD_AS(location, "TEXT"),
                 QI_FIELD(createdAt), QI_FIELD(updatedAt), QI_FIELD(deletedAt));
```

`createdAt`/`updatedAt` are filled automatically on save (no code needed) — the
names are the convention. Saving a photo fires the hook and stamps the times:

```text
   [afterSave] inserted #1 Golden Gate at dusk
  createdAt: 2026-07-23T14:53:59Z
  updatedAt: 2026-07-23T14:53:59Z
  reloaded location: 37.8199 , -122.478
```

And `clean()` rejects an invalid record, with the reason on `lastError()`:

```text
  refused: title must not be empty
```

## Step 3 — Many-to-many through a join table

Declare a join model with a composite key, then link/unlink/fetch with the
relation helpers.

```cpp
class PhotoTag : public QiModel {
    QI_MODEL
public:
    QiField<int> photoId;
    QiField<int> tagId;
};
QI_DECLARE_MODEL_NOID(PhotoTag, "photo_tag",
                      QI_FIELD(photoId, QiPrimary | QiNotNull),
                      QI_FIELD(tagId,   QiPrimary | QiNotNull));
```

```cpp
qiAttach(photo, nature,  "photo_tag", "photoId", "tagId");   // link
qiAttach(photo, evening, "photo_tag", "photoId", "tagId");
qiAttach(photo, city,    "photo_tag", "photoId", "tagId");
qiDetach(photo, city,    "photo_tag", "photoId", "tagId");   // unlink

QiList<Tag> tags = qiManyToMany<Tag>(photo, "photo_tag", "photoId", "tagId");
```

```text
  tags on photo: nature, evening
```

`qiManyToMany` reads the linked keys from the join table, then loads the targets
with a single `IN` query.

## Step 4 — Soft delete

`softRemove()` stamps `deletedAt` instead of deleting the row; `qiAlive<T>()` and
`qiTrashed<T>()` scope your queries.

```cpp
photo2.softRemove();

qiAlive<Photo>().count();      // excludes tombstoned rows
qiTrashed<Photo>().count();    // only tombstoned rows
Photo::objects().count();      // everything — the row is still there
```

```text
  alive before: 2  total rows: 2
  alive after : 1  trashed: 1  total rows: 2
  (the row is still there — just tombstoned)
```

---

## Files

| File | Role |
|---|---|
| `main.cpp` | `GeoPoint` converter, the `Photo` / `Tag` / `PhotoTag` models, and a tour of all five features. |

## See also

- [`schema`](../schema) — keys, constraints, cascades, and column migrations.
- [`migrations`](../migrations) — evolve the schema over versioned releases.
