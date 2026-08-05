-- SQL Server test schema for qivot-gen validation
-- Features: Complex FKs, composite keys, unique constraints, check constraints, JSON support

USE master;
GO

IF DB_ID('qivot_test') IS NOT NULL
BEGIN
    ALTER DATABASE qivot_test SET SINGLE_USER WITH ROLLBACK IMMEDIATE;
    DROP DATABASE qivot_test;
END
GO

CREATE DATABASE qivot_test;
GO

USE qivot_test;
GO

-- Users table
CREATE TABLE [dbo].[users] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [username] NVARCHAR(255) UNIQUE NOT NULL,
    [email] NVARCHAR(255) UNIQUE NOT NULL,
    [password_hash] NVARCHAR(MAX),
    [first_name] NVARCHAR(255),
    [last_name] NVARCHAR(255),
    [role] NVARCHAR(50) CHECK ([role] IN ('user', 'admin', 'moderator')),
    [active] BIT DEFAULT 1,
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- Organizations table
CREATE TABLE [dbo].[organizations] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [name] NVARCHAR(255) NOT NULL,
    [slug] NVARCHAR(255) UNIQUE NOT NULL,
    [description] NVARCHAR(MAX),
    [created_by_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]),
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- Organization Members
CREATE TABLE [dbo].[organization_members] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [organization_id] BIGINT NOT NULL REFERENCES [dbo].[organizations]([id]) ON DELETE CASCADE,
    [user_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]) ON DELETE CASCADE,
    [role] NVARCHAR(50) CHECK ([role] IN ('owner', 'admin', 'member')),
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    UNIQUE([organization_id], [user_id])
);

-- Projects table
CREATE TABLE [dbo].[projects] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [organization_id] BIGINT NOT NULL REFERENCES [dbo].[organizations]([id]) ON DELETE CASCADE,
    [created_by_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]),
    [name] NVARCHAR(255) NOT NULL,
    [description] NVARCHAR(MAX),
    [visibility] NVARCHAR(50) CHECK ([visibility] IN ('public', 'private', 'internal')),
    [archived] BIT DEFAULT 0,
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- Issues table
CREATE TABLE [dbo].[issues] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [project_id] BIGINT NOT NULL REFERENCES [dbo].[projects]([id]) ON DELETE CASCADE,
    [created_by_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]),
    [assigned_to_id] BIGINT REFERENCES [dbo].[users]([id]),
    [title] NVARCHAR(255) NOT NULL,
    [description] NVARCHAR(MAX),
    [status] NVARCHAR(50) CHECK ([status] IN ('open', 'in_progress', 'closed', 'resolved')),
    [priority] INT CHECK ([priority] BETWEEN 1 AND 5),
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- Issue Comments
CREATE TABLE [dbo].[issue_comments] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [issue_id] BIGINT NOT NULL REFERENCES [dbo].[issues]([id]) ON DELETE CASCADE,
    [author_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]),
    [content] NVARCHAR(MAX) NOT NULL,
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- Pull Requests
CREATE TABLE [dbo].[pull_requests] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [project_id] BIGINT NOT NULL REFERENCES [dbo].[projects]([id]) ON DELETE CASCADE,
    [created_by_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]),
    [reviewed_by_id] BIGINT REFERENCES [dbo].[users]([id]),
    [title] NVARCHAR(255) NOT NULL,
    [description] NVARCHAR(MAX),
    [source_branch] NVARCHAR(255) NOT NULL,
    [target_branch] NVARCHAR(255) NOT NULL,
    [status] NVARCHAR(50) CHECK ([status] IN ('open', 'merged', 'closed', 'draft')),
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- PR Reviews
CREATE TABLE [dbo].[pr_reviews] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [pr_id] BIGINT NOT NULL REFERENCES [dbo].[pull_requests]([id]) ON DELETE CASCADE,
    [reviewer_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]),
    [verdict] NVARCHAR(50) CHECK ([verdict] IN ('approved', 'changes_requested', 'commented')),
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- Builds
CREATE TABLE [dbo].[builds] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [project_id] BIGINT NOT NULL REFERENCES [dbo].[projects]([id]) ON DELETE CASCADE,
    [pr_id] BIGINT REFERENCES [dbo].[pull_requests]([id]),
    [status] NVARCHAR(50) CHECK ([status] IN ('pending', 'running', 'success', 'failed', 'canceled')),
    [log_output] NVARCHAR(MAX),
    [metadata] NVARCHAR(MAX),
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- Deployments
CREATE TABLE [dbo].[deployments] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [project_id] BIGINT NOT NULL REFERENCES [dbo].[projects]([id]) ON DELETE CASCADE,
    [deployed_by_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]),
    [environment] NVARCHAR(50) CHECK ([environment] IN ('development', 'staging', 'production')),
    [status] NVARCHAR(50) CHECK ([status] IN ('pending', 'in_progress', 'success', 'failed')),
    [created_at] DATETIME DEFAULT GETUTCDATE(),
    [updated_at] DATETIME DEFAULT GETUTCDATE()
);

-- Audit Log
CREATE TABLE [dbo].[audit_logs] (
    [id] BIGINT IDENTITY(1,1) PRIMARY KEY,
    [actor_id] BIGINT NOT NULL REFERENCES [dbo].[users]([id]),
    [action] NVARCHAR(255) NOT NULL,
    [resource_type] NVARCHAR(100),
    [resource_id] BIGINT,
    [details] NVARCHAR(MAX),
    [created_at] DATETIME DEFAULT GETUTCDATE()
);

-- Create indexes
CREATE INDEX [idx_organizations_created_by] ON [dbo].[organizations]([created_by_id]);
CREATE INDEX [idx_projects_organization_id] ON [dbo].[projects]([organization_id]);
CREATE INDEX [idx_issues_project_id] ON [dbo].[issues]([project_id]);
CREATE INDEX [idx_pull_requests_project_id] ON [dbo].[pull_requests]([project_id]);
CREATE INDEX [idx_builds_project_id] ON [dbo].[builds]([project_id]);
CREATE INDEX [idx_deployments_project_id] ON [dbo].[deployments]([project_id]);
