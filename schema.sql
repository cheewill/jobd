-- NOTE: requires a compile-time setting to allow this
PRAGMA foreign_keys = ON;

CREATE TABLE jobs (
    id INTEGER PRIMARY KEY,
    job_id TEXT UNIQUE NOT NULL,
    command TEXT NOT NULL,
    description VARCHAR,
    enable BOOLEAN NOT NULL DEFAULT 1 CHECK (enable IN (0,1)),
    gid VARCHAR,
    init_groups BOOLEAN NOT NULL DEFAULT 1 CHECK (init_groups IN (0,1)),
    keep_alive BOOLEAN NOT NULL DEFAULT 0 CHECK (keep_alive IN (0,1)),
    root_directory VARCHAR NOT NULL DEFAULT '/',
    standard_error_path VARCHAR NOT NULL DEFAULT '/dev/null',
    standard_in_path NOT NULL DEFAULT '/dev/null',
    standard_out_path NOT NULL DEFAULT '/dev/null',
    start_order INT,
    umask VARCHAR,
    user_name VARCHAR,
    working_directory VARCHAR NOT NULL DEFAULT '/'
    --char **options;
);

CREATE TABLE job_methods (
    id INTEGER PRIMARY KEY,
    job_id INTEGER NOT NULL,
    name TEXT NOT NULL,  -- start, stop, etc.
    script TEXT NOT NULL, -- a sh(1) script
    FOREIGN KEY (job_id) REFERENCES jobs (id) ON DELETE CASCADE
);

-- Ordering: the "before_job_id" will be started before the "after_job_id"
CREATE TABLE job_depends (
    id INTEGER PRIMARY KEY,
    before_job_id TEXT NOT NULL,
    after_job_id TEXT NOT NULL,
    FOREIGN KEY (before_job_id) REFERENCES jobs (job_id) ON DELETE CASCADE,
    FOREIGN KEY (after_job_id) REFERENCES jobs (job_id) ON DELETE CASCADE
);

CREATE TABLE processes (
    id INTEGER PRIMARY KEY,
    job_id INTEGER NOT NULL,
    pid INTEGER DEFAULT 0,
    process_state_id INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (process_state_id) REFERENCES process_states (id) ON DELETE RESTRICT,
    FOREIGN KEY (job_id) REFERENCES jobs (id) ON DELETE RESTRICT
);

CREATE TABLE process_states (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE NOT NULL
);
INSERT INTO process_states (id, name) VALUES (1, 'uninitialized');
INSERT INTO process_states (id, name) VALUES (2, 'starting');
INSERT INTO process_states (id, name) VALUES (3, 'running');
INSERT INTO process_states (id, name) VALUES (4, 'stopping');
INSERT INTO process_states (id, name) VALUES (5, 'stopped');
INSERT INTO process_states (id, name) VALUES (6, 'error');

CREATE VIEW process_table_view
AS
SELECT 
  jobs.id,
  jobs.job_id,
  processes.pid AS pid,
  processes.process_state_id AS state_id,
  process_states.name AS state
FROM jobs 
INNER JOIN processes ON jobs.id = processes.job_id 
INNER JOIN process_states on processes.process_state_id = process_states.id
ORDER BY jobs.job_id;

-- Environment variables for each job.
CREATE TABLE jobs_environment (
    id INTEGER PRIMARY KEY,
    job_id INTEGER,
    env_key TEXT NOT NULL,
    env_value TEXT NOT NULL,
    UNIQUE (job_id, env_key),
    FOREIGN KEY (job_id) REFERENCES jobs (id) ON DELETE CASCADE 
);

--- names for datatypes
CREATE TABLE datatypes (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE NOT NULL
);
INSERT INTO datatypes (id, name) VALUES (1, 'integer');
INSERT INTO datatypes (id, name) VALUES (2, 'string');
INSERT INTO datatypes (id, name) VALUES (3, 'boolean');

--- scope of a property in properties table.
--- scopes with higher IDs override values at lower scopes.
CREATE TABLE property_scopes (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE NOT NULL
);
INSERT INTO property_scopes (id, name) VALUES (1, 'service');
INSERT INTO property_scopes (id, name) VALUES (2, 'instance');
INSERT INTO property_scopes (id, name) VALUES (3, 'admin');

--- variables that can be configured by a sysadmin
CREATE TABLE properties (
    id INTEGER PRIMARY KEY,
    job_id INTEGER NOT NULL,
    datatype_id INTEGER NOT NULL,
    scope_id INTEGER NOT NULL,
    name TEXT UNIQUE NOT NULL,
    value TEXT NOT NULL,
    UNIQUE(scope_id, name),
    FOREIGN KEY (datatype_id) REFERENCES datatypes (id) ON DELETE RESTRICT,
    FOREIGN KEY (scope_id) REFERENCES property_scopes (id) ON DELETE RESTRICT,
    FOREIGN KEY (job_id) REFERENCES jobs (id) ON DELETE CASCADE
);
