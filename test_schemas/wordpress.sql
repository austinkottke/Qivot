-- WordPress Database Schema (PostgreSQL/MySQL Compatible)
-- Core tables and user-related tables with multisite support

CREATE TABLE wp_users (
    ID BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_login VARCHAR(60) NOT NULL UNIQUE,
    user_pass VARCHAR(255) NOT NULL,
    user_email VARCHAR(100) NOT NULL UNIQUE,
    user_url VARCHAR(100),
    user_registered DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    user_activation_key VARCHAR(255),
    user_status INT DEFAULT 0,
    display_name VARCHAR(250),
    INDEX idx_user_login (user_login),
    INDEX idx_user_email (user_email)
);

CREATE TABLE wp_usermeta (
    umeta_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT UNSIGNED NOT NULL,
    meta_key VARCHAR(255),
    meta_value LONGTEXT,
    FOREIGN KEY (user_id) REFERENCES wp_users(ID) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_meta_key (meta_key)
);

CREATE TABLE wp_posts (
    ID BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    post_author BIGINT UNSIGNED NOT NULL,
    post_date DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    post_date_gmt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    post_content LONGTEXT,
    post_title TEXT,
    post_excerpt TEXT,
    post_status VARCHAR(20) DEFAULT 'publish',
    comment_status VARCHAR(20) DEFAULT 'open',
    ping_status VARCHAR(20) DEFAULT 'open',
    post_password VARCHAR(255),
    post_name VARCHAR(200),
    to_ping TEXT,
    pinged TEXT,
    post_modified DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    post_modified_gmt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    post_content_filtered LONGTEXT,
    post_parent BIGINT UNSIGNED DEFAULT 0,
    guid VARCHAR(255),
    menu_order INT DEFAULT 0,
    post_type VARCHAR(20) DEFAULT 'post',
    post_mime_type VARCHAR(100),
    comment_count BIGINT DEFAULT 0,
    FOREIGN KEY (post_author) REFERENCES wp_users(ID),
    INDEX idx_post_author (post_author),
    INDEX idx_post_type (post_type),
    INDEX idx_post_status (post_status),
    INDEX idx_post_parent (post_parent)
);

CREATE TABLE wp_postmeta (
    meta_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    post_id BIGINT UNSIGNED NOT NULL,
    meta_key VARCHAR(255),
    meta_value LONGTEXT,
    FOREIGN KEY (post_id) REFERENCES wp_posts(ID) ON DELETE CASCADE,
    INDEX idx_post_id (post_id),
    INDEX idx_meta_key (meta_key)
);

CREATE TABLE wp_comments (
    comment_ID BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    comment_post_ID BIGINT UNSIGNED NOT NULL,
    comment_author VARCHAR(245) NOT NULL DEFAULT '',
    comment_author_email VARCHAR(100) NOT NULL DEFAULT '',
    comment_author_url VARCHAR(200) NOT NULL DEFAULT '',
    comment_author_IP VARCHAR(100) NOT NULL DEFAULT '',
    comment_date DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    comment_date_gmt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    comment_content TEXT,
    comment_karma INT DEFAULT 0,
    comment_approved VARCHAR(20) DEFAULT '1',
    comment_agent VARCHAR(255),
    comment_type VARCHAR(20) DEFAULT '',
    comment_parent BIGINT UNSIGNED DEFAULT 0,
    user_id BIGINT UNSIGNED DEFAULT 0,
    FOREIGN KEY (comment_post_ID) REFERENCES wp_posts(ID) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES wp_users(ID),
    INDEX idx_comment_post_ID (comment_post_ID),
    INDEX idx_comment_approved (comment_approved),
    INDEX idx_comment_date_gmt (comment_date_gmt)
);

CREATE TABLE wp_commentmeta (
    meta_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    comment_id BIGINT UNSIGNED NOT NULL,
    meta_key VARCHAR(255),
    meta_value LONGTEXT,
    FOREIGN KEY (comment_id) REFERENCES wp_comments(comment_ID) ON DELETE CASCADE,
    INDEX idx_comment_id (comment_id)
);

CREATE TABLE wp_terms (
    term_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(200) NOT NULL,
    slug VARCHAR(200) NOT NULL UNIQUE,
    term_group BIGINT DEFAULT 0,
    INDEX idx_name (name),
    INDEX idx_slug (slug)
);

CREATE TABLE wp_term_taxonomy (
    term_taxonomy_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    term_id BIGINT UNSIGNED NOT NULL,
    taxonomy VARCHAR(32) NOT NULL,
    description LONGTEXT,
    parent BIGINT UNSIGNED DEFAULT 0,
    count BIGINT DEFAULT 0,
    FOREIGN KEY (term_id) REFERENCES wp_terms(term_id) ON DELETE CASCADE,
    UNIQUE KEY (term_id, taxonomy),
    INDEX idx_taxonomy (taxonomy),
    INDEX idx_parent (parent)
);

CREATE TABLE wp_term_relationships (
    object_id BIGINT UNSIGNED NOT NULL,
    term_taxonomy_id BIGINT UNSIGNED NOT NULL,
    term_order INT DEFAULT 0,
    PRIMARY KEY (object_id, term_taxonomy_id),
    FOREIGN KEY (term_taxonomy_id) REFERENCES wp_term_taxonomy(term_taxonomy_id) ON DELETE CASCADE,
    INDEX idx_term_taxonomy_id (term_taxonomy_id)
);

CREATE TABLE wp_termmeta (
    meta_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    term_id BIGINT UNSIGNED NOT NULL,
    meta_key VARCHAR(255),
    meta_value LONGTEXT,
    FOREIGN KEY (term_id) REFERENCES wp_terms(term_id) ON DELETE CASCADE,
    INDEX idx_term_id (term_id),
    INDEX idx_meta_key (meta_key)
);

CREATE TABLE wp_options (
    option_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    option_name VARCHAR(191) NOT NULL UNIQUE,
    option_value LONGTEXT,
    autoload VARCHAR(20) DEFAULT 'yes',
    INDEX idx_option_name (option_name)
);

CREATE TABLE wp_links (
    link_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    link_url VARCHAR(255),
    link_name VARCHAR(255),
    link_image VARCHAR(255),
    link_target VARCHAR(25),
    link_description VARCHAR(255),
    link_visible VARCHAR(20) DEFAULT 'Y',
    link_owner BIGINT UNSIGNED DEFAULT 1,
    link_rating INT DEFAULT 0,
    link_updated DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    link_rel VARCHAR(255),
    link_notes MEDIUMTEXT,
    link_rss VARCHAR(255),
    FOREIGN KEY (link_owner) REFERENCES wp_users(ID),
    INDEX idx_link_visible (link_visible)
);

-- Multisite Tables
CREATE TABLE wp_blogs (
    blog_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    site_id BIGINT UNSIGNED DEFAULT 0,
    domain VARCHAR(200) NOT NULL,
    path VARCHAR(100) NOT NULL,
    registered DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_updated DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    public TINYINT DEFAULT 1,
    archived TINYINT DEFAULT 0,
    mature TINYINT DEFAULT 0,
    spam TINYINT DEFAULT 0,
    deleted TINYINT DEFAULT 0,
    lang_id INT DEFAULT 0,
    UNIQUE KEY (domain, path),
    INDEX idx_site_id (site_id)
);

CREATE TABLE wp_site (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    domain VARCHAR(200) NOT NULL UNIQUE,
    path VARCHAR(100) NOT NULL,
    INDEX idx_domain (domain)
);

CREATE TABLE wp_sitemeta (
    meta_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    site_id BIGINT UNSIGNED NOT NULL,
    meta_key VARCHAR(191),
    meta_value LONGTEXT,
    FOREIGN KEY (site_id) REFERENCES wp_site(id),
    INDEX idx_site_id (site_id)
);
