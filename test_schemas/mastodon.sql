-- Mastodon Database Schema (PostgreSQL)
-- Social networking platform with ActivityPub federation support

CREATE TABLE accounts (
    id BIGINT PRIMARY KEY,
    username VARCHAR NOT NULL,
    domain VARCHAR,
    secret VARCHAR,
    private_key TEXT,
    public_key TEXT NOT NULL,
    remote_url VARCHAR,
    salmon_url VARCHAR,
    hub_url VARCHAR,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    avatar_file_name VARCHAR,
    avatar_content_type VARCHAR,
    avatar_file_size INTEGER,
    avatar_updated_at TIMESTAMP,
    header_file_name VARCHAR,
    header_content_type VARCHAR,
    header_file_size INTEGER,
    header_updated_at TIMESTAMP,
    avatar_remote_url VARCHAR,
    subscription_expires_at TIMESTAMP,
    locked BOOLEAN DEFAULT FALSE,
    note_updated_at TIMESTAMP,
    display_name VARCHAR,
    uri VARCHAR,
    url VARCHAR,
    last_webfingered_at TIMESTAMP,
    inboxes_count INTEGER DEFAULT 0,
    INDEX idx_username_domain (username, domain),
    UNIQUE INDEX idx_domain_username (domain, username)
);

CREATE TABLE account_stats (
    id BIGINT PRIMARY KEY,
    account_id BIGINT NOT NULL UNIQUE,
    statuses_count BIGINT DEFAULT 0,
    following_count BIGINT DEFAULT 0,
    followers_count BIGINT DEFAULT 0,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    last_status_at DATE,
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE users (
    id BIGINT PRIMARY KEY,
    email VARCHAR NOT NULL UNIQUE,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    encrypted_password VARCHAR NOT NULL DEFAULT '',
    reset_password_token VARCHAR,
    reset_password_sent_at TIMESTAMP,
    remember_created_at TIMESTAMP,
    sign_in_count INTEGER DEFAULT 0,
    current_sign_in_at TIMESTAMP,
    last_sign_in_at TIMESTAMP,
    current_sign_in_ip INET,
    last_sign_in_ip INET,
    admin BOOLEAN DEFAULT FALSE,
    confirmation_token VARCHAR,
    confirmed_at TIMESTAMP,
    confirmation_sent_at TIMESTAMP,
    unconfirmed_email VARCHAR,
    locale VARCHAR,
    encrypted_otp_secret VARCHAR,
    encrypted_otp_secret_iv VARCHAR,
    otp_required_for_login BOOLEAN DEFAULT FALSE,
    last_otp_backup_code_used_at TIMESTAMP,
    account_id BIGINT NOT NULL,
    disabled BOOLEAN DEFAULT FALSE,
    moderator BOOLEAN DEFAULT FALSE,
    role_id BIGINT,
    created_by_application_id BIGINT,
    approved BOOLEAN DEFAULT TRUE,
    INDEX idx_account_id (account_id),
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE statuses (
    id BIGINT PRIMARY KEY,
    uri VARCHAR,
    text TEXT NOT NULL DEFAULT '',
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    in_reply_to_id BIGINT,
    in_reply_to_account_id BIGINT,
    sensitive BOOLEAN DEFAULT FALSE,
    visibility INTEGER DEFAULT 0,
    spoiler_text VARCHAR DEFAULT '',
    reply BOOLEAN DEFAULT FALSE,
    language VARCHAR,
    conversation_id BIGINT,
    note TEXT,
    account_id BIGINT NOT NULL,
    application_id BIGINT,
    edited_at TIMESTAMP,
    content_type VARCHAR DEFAULT 'text/html',
    INDEX idx_account_id (account_id),
    INDEX idx_in_reply_to_id (in_reply_to_id),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE status_stats (
    id BIGINT PRIMARY KEY,
    status_id BIGINT NOT NULL UNIQUE,
    replies_count BIGINT DEFAULT 0,
    reblogs_count BIGINT DEFAULT 0,
    favourites_count BIGINT DEFAULT 0,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    FOREIGN KEY (status_id) REFERENCES statuses(id) ON DELETE CASCADE
);

CREATE TABLE media_attachments (
    id BIGINT PRIMARY KEY,
    status_id BIGINT,
    account_id BIGINT NOT NULL,
    type INTEGER DEFAULT 0,
    url VARCHAR,
    remote_url VARCHAR,
    preview_url VARCHAR,
    text_url VARCHAR,
    meta JSON,
    description TEXT,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    blurhash VARCHAR,
    processing INTEGER,
    file_file_name VARCHAR,
    file_content_type VARCHAR,
    file_file_size INTEGER,
    file_updated_at TIMESTAMP,
    thumbnail_file_name VARCHAR,
    thumbnail_content_type VARCHAR,
    thumbnail_file_size INTEGER,
    thumbnail_updated_at TIMESTAMP,
    INDEX idx_status_id (status_id),
    INDEX idx_account_id (account_id),
    FOREIGN KEY (status_id) REFERENCES statuses(id) ON DELETE SET NULL
);

CREATE TABLE follows (
    id BIGINT PRIMARY KEY,
    follower_id BIGINT NOT NULL,
    target_account_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    show_reblogs BOOLEAN DEFAULT TRUE,
    uri VARCHAR,
    languages VARCHAR[],
    notify BOOLEAN DEFAULT FALSE,
    UNIQUE INDEX idx_follower_target (follower_id, target_account_id),
    FOREIGN KEY (follower_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (target_account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE follow_requests (
    id BIGINT PRIMARY KEY,
    follower_id BIGINT NOT NULL,
    target_account_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    uri VARCHAR,
    UNIQUE INDEX idx_follower_target (follower_id, target_account_id),
    FOREIGN KEY (follower_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (target_account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE blocks (
    id BIGINT PRIMARY KEY,
    account_id BIGINT NOT NULL,
    target_account_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    uri VARCHAR,
    UNIQUE INDEX idx_account_target (account_id, target_account_id),
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (target_account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE mutes (
    id BIGINT PRIMARY KEY,
    account_id BIGINT NOT NULL,
    target_account_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    hide_notifications BOOLEAN DEFAULT TRUE,
    expires_at TIMESTAMP,
    UNIQUE INDEX idx_account_target (account_id, target_account_id),
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (target_account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE favourites (
    id BIGINT PRIMARY KEY,
    account_id BIGINT NOT NULL,
    status_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    UNIQUE INDEX idx_account_status (account_id, status_id),
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (status_id) REFERENCES statuses(id) ON DELETE CASCADE
);

CREATE TABLE bookmarks (
    id BIGINT PRIMARY KEY,
    account_id BIGINT NOT NULL,
    status_id BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    UNIQUE INDEX idx_account_status (account_id, status_id),
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (status_id) REFERENCES statuses(id) ON DELETE CASCADE
);

CREATE TABLE polls (
    id BIGINT PRIMARY KEY,
    status_id BIGINT,
    account_id BIGINT,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    expires_at TIMESTAMP,
    options TEXT[],
    cached_tallies BIGINT[],
    multiple BOOLEAN DEFAULT FALSE,
    hide_totals BOOLEAN DEFAULT FALSE,
    voters_count BIGINT DEFAULT 0,
    votes_count BIGINT DEFAULT 0,
    FOREIGN KEY (status_id) REFERENCES statuses(id) ON DELETE CASCADE,
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE tags (
    id BIGINT PRIMARY KEY,
    name VARCHAR NOT NULL UNIQUE,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    usable BOOLEAN DEFAULT TRUE,
    trendable BOOLEAN DEFAULT FALSE,
    listable BOOLEAN DEFAULT FALSE,
    review_requested BOOLEAN DEFAULT FALSE,
    last_status_at TIMESTAMP,
    max_score DECIMAL(8,2),
    max_score_at TIMESTAMP,
    INDEX idx_name (name)
);

CREATE TABLE conversations (
    id BIGINT PRIMARY KEY,
    uri VARCHAR,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    participant_accounts BIGINT[]
);

CREATE TABLE notifications (
    id BIGINT PRIMARY KEY,
    type VARCHAR NOT NULL,
    account_id BIGINT NOT NULL,
    from_account_id BIGINT NOT NULL,
    activity_type VARCHAR,
    activity_id BIGINT,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    filtered BOOLEAN DEFAULT FALSE,
    INDEX idx_account_id (account_id),
    INDEX idx_type (type),
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (from_account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

CREATE TABLE domain_blocks (
    id BIGINT PRIMARY KEY,
    domain VARCHAR NOT NULL UNIQUE,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    severity INTEGER DEFAULT 0,
    reject_media BOOLEAN DEFAULT FALSE,
    reject_reports BOOLEAN DEFAULT FALSE,
    private_comment TEXT,
    public_comment TEXT,
    obfuscate BOOLEAN DEFAULT FALSE
);

CREATE TABLE reports (
    id BIGINT PRIMARY KEY,
    status_ids BIGINT[],
    comment TEXT,
    action_taken BOOLEAN DEFAULT FALSE,
    action_taken_at TIMESTAMP,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    account_id BIGINT NOT NULL,
    action_taken_by_account_id BIGINT,
    target_account_id BIGINT NOT NULL,
    assigned_account_id BIGINT,
    uri VARCHAR,
    forwarded BOOLEAN DEFAULT FALSE,
    category VARCHAR DEFAULT 'other',
    INDEX idx_account_id (account_id),
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (target_account_id) REFERENCES accounts(id) ON DELETE CASCADE
);
