-- SQLite before 3.35 (Qt 5.15.2 ships 3.33) has no DROP COLUMN, so the column
-- goes the portable way: rebuild the table without it.
DROP TRIGGER note_tag_added;
CREATE TABLE tag_new (
    id   INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);
INSERT INTO tag_new (id, name) SELECT id, name FROM tag;
DROP TABLE tag;
ALTER TABLE tag_new RENAME TO tag;
