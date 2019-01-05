-- A volatile database living in $runstatedir that is recreated
-- at every boot.

BEGIN TRANSACTION;

CREATE TABLE volatile.job_states (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE NOT NULL
);

-- Keep this in sync with enum job_state.
INSERT INTO job_states (id, name)
VALUES
    (1, 'disabled'),
    (2, 'pending'),
    (3, 'starting'),
    (4, 'running'),
    (5, 'stopping'),
    (6, 'stopped'),
    (7, 'complete'),
    (8, 'error');

CREATE TABLE volatile.active_jobs
(
    id           INTEGER PRIMARY KEY,
    job_id       TEXT UNIQUE NOT NULL,
    job_state_id INTEGER     NOT NULL,
    FOREIGN KEY (job_state_id) REFERENCES job_states (id) ON DELETE RESTRICT
);

INSERT INTO volatile.active_jobs (id, job_id, job_state_id)
SELECT id, job_id, (SELECT id FROM job_states WHERE name = 'disabled') AS job_state_id
FROM main.jobs
WHERE jobs.enable = 0;

INSERT INTO volatile.active_jobs (id, job_id, job_state_id)
SELECT id, job_id, (SELECT id FROM job_states WHERE name = 'pending') AS job_state_id
FROM main.jobs
WHERE jobs.enable = 1;

CREATE TABLE volatile.processes
(
    pid           INTEGER PRIMARY KEY, -- matches kernel PID
    job_id        INTEGER UNIQUE NOT NULL,
    exited        INTEGER CHECK (exited IN (0, 1)),
    exit_status   INTEGER,
    signaled      INTEGER CHECK (signaled IN (0, 1)),
    signal_number INTEGER,
    start_time    INTEGER        NOT NULL DEFAULT 0,
    end_time      INTEGER        NOT NULL DEFAULT 0,
    FOREIGN KEY (job_id) REFERENCES active_jobs (id) ON DELETE RESTRICT
);


-- the order that jobs are started in
CREATE TABLE volatile.job_order (
    job_id INTEGER UNIQUE NOT NULL, -- FOREIGN KEY of jobs table
    wave INTEGER
);

COMMIT;
