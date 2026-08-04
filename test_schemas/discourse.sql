-- Discourse Database Schema (PostgreSQL)
-- Community discussion platform with categories, posts, users, and moderation

CREATE TABLE users (
    id BIGINT PRIMARY KEY,
    username VARCHAR NOT NULL UNIQUE,
    email VARCHAR NOT NULL UNIQUE,
    name VARCHAR,
    active BOOLEAN DEFAULT TRUE,
    approved BOOLEAN DEFAULT TRUE,
    approved_by_id BIGINT,
    suspended_at TIMESTAMP,
    suspended_till TIMESTAMP,
    blocked BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    admin BOOLEAN DEFAULT FALSE,
    moderator BOOLEAN DEFAULT FALSE,
    title VARCHAR,
    uploaded_avatar_id BIGINT,
    locale VARCHAR,
    email_tokens_count INTEGER DEFAULT 0,
    ip_address INET,
    last_seen_at TIMESTAMP,
    auth_token_count INTEGER DEFAULT 0,
    silenced_till TIMESTAMP,
    first_seen_at TIMESTAMP,
    INDEX idx_username (username),
    INDEX idx_email (email)
);

CREATE TABLE user_emails (
    id BIGINT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    email VARCHAR NOT NULL UNIQUE,
    primary_email BOOLEAN DEFAULT TRUE,
    confirmed BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id)
);

CREATE TABLE categories (
    id BIGINT PRIMARY KEY,
    name VARCHAR NOT NULL,
    color VARCHAR NOT NULL,
    topic_id BIGINT,
    topic_count BIGINT DEFAULT 0,
    post_count BIGINT DEFAULT 0,
    description TEXT,
    description_excerpt TEXT,
    user_id BIGINT NOT NULL,
    topics_year BIGINT DEFAULT 0,
    topics_month BIGINT DEFAULT 0,
    topics_week BIGINT DEFAULT 0,
    slug VARCHAR NOT NULL UNIQUE,
    position INTEGER,
    email_in VARCHAR,
    email_in_allow_strangers BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    read_restricted BOOLEAN DEFAULT FALSE,
    parent_category_id BIGINT,
    auto_close_hours DECIMAL(5,2),
    auto_close_based_on_last_post BOOLEAN DEFAULT FALSE,
    topic_template TEXT,
    suppress_from_latest BOOLEAN DEFAULT FALSE,
    allow_badges BOOLEAN DEFAULT TRUE,
    custom_fields JSONB,
    INDEX idx_slug (slug),
    FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE topics (
    id BIGINT PRIMARY KEY,
    title VARCHAR NOT NULL,
    last_posted_at TIMESTAMP,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    views INTEGER DEFAULT 0,
    posts_count INTEGER DEFAULT 0,
    user_id BIGINT NOT NULL,
    last_post_user_id BIGINT,
    reply_count INTEGER DEFAULT 0,
    featured_user1_id BIGINT,
    featured_user2_id BIGINT,
    featured_user3_id BIGINT,
    avg_time INTEGER,
    deleted_at TIMESTAMP,
    highest_post_number INTEGER,
    highest_staff_post_number INTEGER,
    image_url VARCHAR,
    slow_mode_seconds INTEGER,
    category_id BIGINT NOT NULL,
    visible BOOLEAN DEFAULT TRUE,
    moderator_posts_count INTEGER DEFAULT 0,
    closed BOOLEAN DEFAULT FALSE,
    archived BOOLEAN DEFAULT FALSE,
    bumped_at TIMESTAMP,
    has_summary BOOLEAN DEFAULT FALSE,
    archetype VARCHAR DEFAULT 'regular',
    pinned_at TIMESTAMP,
    unpinned_at TIMESTAMP,
    pinned_globally BOOLEAN DEFAULT FALSE,
    pinned_until TIMESTAMP,
    word_count INTEGER,
    excerpt TEXT,
    like_count INTEGER DEFAULT 0,
    subtype VARCHAR,
    notify_moderators_count INTEGER DEFAULT 0,
    spam_count INTEGER DEFAULT 0,
    illegal_count INTEGER DEFAULT 0,
    inappropriate_count INTEGER DEFAULT 0,
    last_read_post_number INTEGER,
    custom_fields JSONB,
    INDEX idx_category_id (category_id),
    INDEX idx_user_id (user_id),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE posts (
    id BIGINT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    topic_id BIGINT NOT NULL,
    post_number INTEGER NOT NULL,
    raw TEXT NOT NULL,
    cooked TEXT,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    reply_to_post_number INTEGER,
    reply_count INTEGER DEFAULT 0,
    quote_count INTEGER DEFAULT 0,
    deleted_at TIMESTAMP,
    off_topic_count INTEGER DEFAULT 0,
    like_count INTEGER DEFAULT 0,
    incoming_link_count INTEGER DEFAULT 0,
    bookmark_count INTEGER DEFAULT 0,
    score DECIMAL(10,2),
    reads INTEGER DEFAULT 0,
    avg_time INTEGER,
    word_count INTEGER,
    version INTEGER DEFAULT 1,
    cook_method INTEGER DEFAULT 0,
    wiki BOOLEAN DEFAULT FALSE,
    baked_at TIMESTAMP,
    baked_version INTEGER,
    hidden BOOLEAN DEFAULT FALSE,
    hidden_reason_id INTEGER,
    notify_moderators_count INTEGER DEFAULT 0,
    spam_count INTEGER DEFAULT 0,
    illegal_count INTEGER DEFAULT 0,
    inappropriate_count INTEGER DEFAULT 0,
    last_version_at TIMESTAMP,
    user_deleted BOOLEAN DEFAULT FALSE,
    edit_reason VARCHAR,
    image_count INTEGER,
    custom_fields JSONB,
    INDEX idx_topic_id (topic_id),
    INDEX idx_user_id (user_id),
    INDEX idx_created_at (created_at),
    UNIQUE INDEX idx_topic_post_number (topic_id, post_number),
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (topic_id) REFERENCES topics(id) ON DELETE CASCADE
);

CREATE TABLE post_actions (
    id BIGINT PRIMARY KEY,
    post_id BIGINT NOT NULL,
    user_id BIGINT NOT NULL,
    post_action_type_id INTEGER NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    deleted_at TIMESTAMP,
    related_post_id BIGINT,
    flags_agreed INTEGER DEFAULT 0,
    flags_disagreed INTEGER DEFAULT 0,
    deferred_by_id BIGINT,
    targets_topic BOOLEAN DEFAULT FALSE,
    custom_message TEXT,
    UNIQUE INDEX idx_user_post_type (user_id, post_id, post_action_type_id),
    FOREIGN KEY (post_id) REFERENCES posts(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE bookmarks (
    id BIGINT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    post_id BIGINT,
    topic_id BIGINT,
    name VARCHAR,
    reminder_type INTEGER,
    reminder_at TIMESTAMP,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    clear_reminder_when_read BOOLEAN DEFAULT TRUE,
    auto_delete_preference INTEGER DEFAULT 0,
    pinned BOOLEAN DEFAULT FALSE,
    FOR_topic BOOLEAN,
    UNIQUE INDEX idx_user_post (user_id, post_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (post_id) REFERENCES posts(id) ON DELETE CASCADE,
    FOREIGN KEY (topic_id) REFERENCES topics(id) ON DELETE CASCADE
);

CREATE TABLE topic_tags (
    id BIGINT PRIMARY KEY,
    topic_id BIGINT NOT NULL,
    tag_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    UNIQUE INDEX idx_topic_tag (topic_id, tag_id),
    FOREIGN KEY (topic_id) REFERENCES topics(id) ON DELETE CASCADE
);

CREATE TABLE tags (
    id BIGINT PRIMARY KEY,
    name VARCHAR NOT NULL UNIQUE,
    description TEXT,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    public BOOLEAN DEFAULT TRUE,
    topic_count INTEGER DEFAULT 0,
    pm_topic_count INTEGER DEFAULT 0,
    INDEX idx_name (name)
);

CREATE TABLE chat_channels (
    id BIGINT PRIMARY KEY,
    name VARCHAR NOT NULL,
    description TEXT,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    chatable_type VARCHAR,
    chatable_id BIGINT,
    deleted_at TIMESTAMP,
    public_channel BOOLEAN DEFAULT FALSE,
    user_count INTEGER DEFAULT 0,
    INDEX idx_created_at (created_at)
);

CREATE TABLE chat_messages (
    id BIGINT PRIMARY KEY,
    chat_channel_id BIGINT NOT NULL,
    user_id BIGINT NOT NULL,
    message TEXT,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    deleted_at TIMESTAMP,
    deleted_by_id BIGINT,
    thread_id BIGINT,
    in_reply_to_id BIGINT,
    edited_at TIMESTAMP,
    edited_by_id BIGINT,
    INDEX idx_chat_channel_id (chat_channel_id),
    INDEX idx_user_id (user_id),
    FOREIGN KEY (chat_channel_id) REFERENCES chat_channels(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE chat_threads (
    id BIGINT PRIMARY KEY,
    chat_channel_id BIGINT NOT NULL,
    original_message_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    title VARCHAR,
    FOREIGN KEY (chat_channel_id) REFERENCES chat_channels(id) ON DELETE CASCADE,
    FOREIGN KEY (original_message_id) REFERENCES chat_messages(id)
);

CREATE TABLE chat_channel_memberships (
    id BIGINT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    chat_channel_id BIGINT NOT NULL,
    last_read_message_id BIGINT,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    UNIQUE INDEX idx_user_channel (user_id, chat_channel_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (chat_channel_id) REFERENCES chat_channels(id) ON DELETE CASCADE
);

CREATE TABLE badges (
    id BIGINT PRIMARY KEY,
    name VARCHAR NOT NULL,
    description TEXT,
    badge_type_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    icon VARCHAR,
    query TEXT,
    enabled BOOLEAN DEFAULT TRUE,
    INDEX idx_name (name)
);

CREATE TABLE user_badges (
    id BIGINT PRIMARY KEY,
    badge_id BIGINT NOT NULL,
    user_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    notification_id BIGINT,
    seq INTEGER,
    granted_at TIMESTAMP,
    granted_by_id BIGINT,
    UNIQUE INDEX idx_user_badge (user_id, badge_id),
    FOREIGN KEY (badge_id) REFERENCES badges(id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE reports (
    id BIGINT PRIMARY KEY,
    post_id BIGINT,
    user_id BIGINT NOT NULL,
    report_type VARCHAR NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    status INTEGER DEFAULT 0,
    subject_type VARCHAR,
    subject_id BIGINT,
    archived BOOLEAN DEFAULT FALSE,
    INDEX idx_created_at (created_at),
    FOREIGN KEY (post_id) REFERENCES posts(id),
    FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE api_keys (
    id BIGINT PRIMARY KEY,
    user_id BIGINT,
    key VARCHAR NOT NULL UNIQUE,
    description VARCHAR,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    revoked_at TIMESTAMP,
    last_used_at TIMESTAMP,
    scopes TEXT[],
    allowed_ips INET[],
    INDEX idx_key (key)
);
