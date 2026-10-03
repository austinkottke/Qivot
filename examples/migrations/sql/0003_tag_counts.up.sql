-- Keep a usage count on each tag. The trigger body has semicolons of its own;
-- QiMigrator keeps BEGIN ... END together.
ALTER TABLE tag ADD COLUMN uses INTEGER NOT NULL DEFAULT 0;
CREATE TRIGGER note_tag_added AFTER INSERT ON note_tag BEGIN
    UPDATE tag SET uses = uses + 1 WHERE id = new.tag_id;
END;
