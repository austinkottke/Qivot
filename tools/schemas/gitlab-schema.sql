-- GitLab-like schema: DevOps platform with 15 core tables
-- Features: UUIDs, extensive JSONB, complex FKs, soft deletes

CREATE TABLE users (
  id BIGSERIAL PRIMARY KEY,
  username TEXT UNIQUE NOT NULL,
  email TEXT UNIQUE NOT NULL,
  encrypted_password TEXT,
  avatar_url TEXT,
  bio TEXT,
  admin BOOLEAN DEFAULT FALSE,
  can_create_group BOOLEAN DEFAULT TRUE,
  projects_limit INTEGER DEFAULT 10,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE namespaces (
  id BIGSERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  path TEXT NOT NULL,
  owner_id BIGINT REFERENCES users(id) ON DELETE CASCADE,
  namespace_type TEXT CHECK (namespace_type IN ('user', 'group')),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE(name, owner_id)
);

CREATE TABLE projects (
  id BIGSERIAL PRIMARY KEY,
  namespace_id BIGINT NOT NULL REFERENCES namespaces(id) ON DELETE CASCADE,
  creator_id BIGINT NOT NULL REFERENCES users(id),
  name TEXT NOT NULL,
  path TEXT NOT NULL,
  description TEXT,
  visibility TEXT CHECK (visibility IN ('public', 'internal', 'private')),
  star_count INTEGER DEFAULT 0,
  forks_count INTEGER DEFAULT 0,
  archived BOOLEAN DEFAULT FALSE,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE(namespace_id, path)
);

CREATE TABLE project_members (
  id BIGSERIAL PRIMARY KEY,
  project_id BIGINT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
  user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  access_level INTEGER CHECK (access_level IN (10, 20, 30, 40, 50)),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE(project_id, user_id)
);

CREATE TABLE issues (
  id BIGSERIAL PRIMARY KEY,
  project_id BIGINT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
  author_id BIGINT NOT NULL REFERENCES users(id),
  assignee_id BIGINT REFERENCES users(id),
  title TEXT NOT NULL,
  description TEXT,
  state TEXT CHECK (state IN ('opened', 'closed', 'locked')),
  priority INTEGER CHECK (priority BETWEEN 1 AND 4),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  closed_at TIMESTAMP
);

CREATE TABLE issue_metrics (
  id BIGSERIAL PRIMARY KEY,
  issue_id BIGINT NOT NULL UNIQUE REFERENCES issues(id) ON DELETE CASCADE,
  time_estimate INTEGER,
  total_time_spent INTEGER,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE merge_requests (
  id BIGSERIAL PRIMARY KEY,
  project_id BIGINT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
  author_id BIGINT NOT NULL REFERENCES users(id),
  assignee_id BIGINT REFERENCES users(id),
  source_branch TEXT NOT NULL,
  target_branch TEXT NOT NULL,
  title TEXT NOT NULL,
  description TEXT,
  state TEXT CHECK (state IN ('opened', 'closed', 'locked', 'merged')),
  merged_at TIMESTAMP,
  merged_by_id BIGINT REFERENCES users(id),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE notes (
  id BIGSERIAL PRIMARY KEY,
  noteable_type TEXT NOT NULL,
  noteable_id BIGINT NOT NULL,
  author_id BIGINT NOT NULL REFERENCES users(id),
  content TEXT NOT NULL,
  confidential BOOLEAN DEFAULT FALSE,
  deleted_at TIMESTAMP,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE commits (
  id BIGSERIAL PRIMARY KEY,
  project_id BIGINT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
  committed_by_id BIGINT REFERENCES users(id),
  committed_date TIMESTAMP,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE ci_pipelines (
  id BIGSERIAL PRIMARY KEY,
  project_id BIGINT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
  created_by_id BIGINT REFERENCES users(id),
  source TEXT CHECK (source IN ('push', 'web', 'trigger', 'schedule')),
  status TEXT CHECK (status IN ('created', 'pending', 'running', 'success', 'failed', 'canceled')),
  yaml_errors TEXT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE ci_jobs (
  id BIGSERIAL PRIMARY KEY,
  pipeline_id BIGINT NOT NULL REFERENCES ci_pipelines(id) ON DELETE CASCADE,
  project_id BIGINT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
  name TEXT NOT NULL,
  status TEXT CHECK (status IN ('created', 'pending', 'running', 'success', 'failed', 'canceled')),
  stage TEXT,
  trace TEXT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE labels (
  id BIGSERIAL PRIMARY KEY,
  project_id BIGINT REFERENCES projects(id) ON DELETE CASCADE,
  title TEXT NOT NULL,
  color TEXT CHECK (color ~ '^#[0-9a-fA-F]{6}$'),
  description TEXT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE label_links (
  id BIGSERIAL PRIMARY KEY,
  label_id BIGINT NOT NULL REFERENCES labels(id) ON DELETE CASCADE,
  target_type TEXT NOT NULL,
  target_id BIGINT NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE(label_id, target_type, target_id)
);

CREATE TABLE deployments (
  id BIGSERIAL PRIMARY KEY,
  project_id BIGINT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
  created_by_id BIGINT NOT NULL REFERENCES users(id),
  environment TEXT NOT NULL,
  status TEXT CHECK (status IN ('created', 'running', 'success', 'failed')),
  deployable_type TEXT,
  deployable_id BIGINT,
  ref TEXT,
  sha TEXT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE project_settings (
  id BIGSERIAL PRIMARY KEY,
  project_id BIGINT NOT NULL UNIQUE REFERENCES projects(id) ON DELETE CASCADE,
  settings JSONB DEFAULT '{}',
  ci_config JSONB,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Indexes
CREATE INDEX idx_projects_namespace_id ON projects(namespace_id);
CREATE INDEX idx_issues_project_id ON issues(project_id);
CREATE INDEX idx_merge_requests_project_id ON merge_requests(project_id);
CREATE INDEX idx_ci_pipelines_project_id ON ci_pipelines(project_id);
