-- Moderated LMS - SQLite schema v1.0 (Phase 3)
-- Times are INTEGER Unix seconds (UTC) from the host clock.
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE app_user (
    user_id         INTEGER PRIMARY KEY,
    username        TEXT    NOT NULL UNIQUE COLLATE NOCASE,
    full_name       TEXT    NOT NULL,
    password_hash   TEXT    NOT NULL,
    role            TEXT    NOT NULL CHECK (role IN ('STUDENT','LECTURER','HOD','ADMIN')),
    is_active       INTEGER NOT NULL DEFAULT 1 CHECK (is_active IN (0,1)),
    failed_attempts INTEGER NOT NULL DEFAULT 0 CHECK (failed_attempts >= 0),
    locked_until    INTEGER,
    created_at      INTEGER NOT NULL
);

CREATE TABLE department (
    department_id INTEGER PRIMARY KEY,
    name          TEXT    NOT NULL UNIQUE,
    hod_user_id   INTEGER UNIQUE REFERENCES app_user(user_id) ON DELETE RESTRICT
);

CREATE TABLE course (
    course_id     INTEGER PRIMARY KEY,
    code          TEXT    NOT NULL UNIQUE,
    title         TEXT    NOT NULL,
    department_id INTEGER NOT NULL REFERENCES department(department_id) ON DELETE RESTRICT
);

CREATE TABLE course_lecturer (
    course_id   INTEGER NOT NULL REFERENCES course(course_id) ON DELETE CASCADE,
    lecturer_id INTEGER NOT NULL REFERENCES app_user(user_id) ON DELETE RESTRICT,
    PRIMARY KEY (course_id, lecturer_id)
);

CREATE TABLE enrolment (
    course_id   INTEGER NOT NULL REFERENCES course(course_id) ON DELETE CASCADE,
    student_id  INTEGER NOT NULL REFERENCES app_user(user_id) ON DELETE RESTRICT,
    enrolled_at INTEGER NOT NULL,
    PRIMARY KEY (course_id, student_id)
);

CREATE TABLE assignment (
    assignment_id INTEGER PRIMARY KEY,
    course_id     INTEGER NOT NULL REFERENCES course(course_id) ON DELETE RESTRICT,
    title         TEXT    NOT NULL,
    description   TEXT    NOT NULL DEFAULT '',
    deadline      INTEGER NOT NULL,
    max_mark      INTEGER NOT NULL CHECK (max_mark BETWEEN 1 AND 100),
    created_by    INTEGER NOT NULL REFERENCES app_user(user_id),
    created_at    INTEGER NOT NULL,
    released_by   INTEGER REFERENCES app_user(user_id),
    released_at   INTEGER,
    CHECK ((released_by IS NULL) = (released_at IS NULL))
);

CREATE TABLE extension (
    extension_id  INTEGER PRIMARY KEY,
    assignment_id INTEGER NOT NULL REFERENCES assignment(assignment_id) ON DELETE CASCADE,
    student_id    INTEGER NOT NULL REFERENCES app_user(user_id),
    new_deadline  INTEGER NOT NULL,
    reason        TEXT    NOT NULL CHECK (length(trim(reason)) > 0),
    granted_by    INTEGER NOT NULL REFERENCES app_user(user_id),
    granted_at    INTEGER NOT NULL,
    UNIQUE (assignment_id, student_id)
);

CREATE TABLE submission (
    submission_id INTEGER PRIMARY KEY,
    assignment_id INTEGER NOT NULL REFERENCES assignment(assignment_id) ON DELETE RESTRICT,
    student_id    INTEGER NOT NULL REFERENCES app_user(user_id),
    status        TEXT    NOT NULL CHECK (status IN
                  ('SUBMITTED','LATE','MARKED','RETURNED','MODERATED','RELEASED')),
    UNIQUE (assignment_id, student_id)
);

CREATE TABLE submission_version (
    version_id    INTEGER PRIMARY KEY,
    submission_id INTEGER NOT NULL REFERENCES submission(submission_id) ON DELETE RESTRICT,
    version_no    INTEGER NOT NULL CHECK (version_no >= 1),
    stored_path   TEXT    NOT NULL UNIQUE,
    original_name TEXT    NOT NULL,
    size_bytes    INTEGER NOT NULL CHECK (size_bytes BETWEEN 1 AND 10485760),
    submitted_at  INTEGER NOT NULL,
    UNIQUE (submission_id, version_no)
);

CREATE TABLE mark (
    submission_id INTEGER PRIMARY KEY REFERENCES submission(submission_id) ON DELETE RESTRICT,
    raw_mark      INTEGER NOT NULL CHECK (raw_mark >= 0),
    feedback      TEXT    NOT NULL CHECK (length(trim(feedback)) > 0),
    policy_code   TEXT    NOT NULL DEFAULT 'TEN_PCT_PER_DAY_V1',
    marked_by     INTEGER NOT NULL REFERENCES app_user(user_id),
    marked_at     INTEGER NOT NULL,
    moderated_by  INTEGER REFERENCES app_user(user_id),
    moderated_at  INTEGER,
    hod_comment   TEXT,
    CHECK ((moderated_by IS NULL) = (moderated_at IS NULL))
);

CREATE TABLE audit_entry (
    audit_id      INTEGER PRIMARY KEY,
    submission_id INTEGER NOT NULL REFERENCES submission(submission_id) ON DELETE RESTRICT,
    changed_by    INTEGER NOT NULL REFERENCES app_user(user_id),
    changed_at    INTEGER NOT NULL,
    action        TEXT    NOT NULL CHECK (action IN
                  ('MARK','REMARK','HOD_ADJUST','APPROVE','RETURN','RELEASE')),
    old_mark      INTEGER,
    new_mark      INTEGER,
    reason        TEXT,
    CHECK (action IN ('MARK','APPROVE','RELEASE')
           OR COALESCE(length(trim(reason)), 0) > 0)
);

-- Indexes for NFR-01 (list screens <= 1 s with 20,000 submissions)
CREATE INDEX ix_course_department     ON course(department_id);
CREATE INDEX ix_enrolment_student     ON enrolment(student_id);
CREATE INDEX ix_course_lecturer_lect  ON course_lecturer(lecturer_id);
CREATE INDEX ix_assignment_course     ON assignment(course_id);
CREATE INDEX ix_submission_student    ON submission(student_id);
CREATE INDEX ix_submission_status     ON submission(assignment_id, status);
CREATE INDEX ix_audit_submission      ON audit_entry(submission_id, changed_at);

-- FR-21: the audit log cannot be edited or deleted
CREATE TRIGGER trg_audit_no_update BEFORE UPDATE ON audit_entry
BEGIN SELECT RAISE(ABORT, 'audit_entry is append-only'); END;
CREATE TRIGGER trg_audit_no_delete BEFORE DELETE ON audit_entry
BEGIN SELECT RAISE(ABORT, 'audit_entry is append-only'); END;

-- FR-14: raw mark cannot exceed the assignment's maximum (cross-table rule)
CREATE TRIGGER trg_mark_max_ins BEFORE INSERT ON mark
WHEN NEW.raw_mark > (SELECT a.max_mark FROM submission s
                     JOIN assignment a ON a.assignment_id = s.assignment_id
                     WHERE s.submission_id = NEW.submission_id)
BEGIN SELECT RAISE(ABORT, 'raw_mark exceeds max_mark'); END;
CREATE TRIGGER trg_mark_max_upd BEFORE UPDATE OF raw_mark ON mark
WHEN NEW.raw_mark > (SELECT a.max_mark FROM submission s
                     JOIN assignment a ON a.assignment_id = s.assignment_id
                     WHERE s.submission_id = NEW.submission_id)
BEGIN SELECT RAISE(ABORT, 'raw_mark exceeds max_mark'); END;

-- A department's HoD must have role HOD
CREATE TRIGGER trg_dept_hod_ins BEFORE INSERT ON department
WHEN NEW.hod_user_id IS NOT NULL
 AND (SELECT role FROM app_user WHERE user_id = NEW.hod_user_id) <> 'HOD'
BEGIN SELECT RAISE(ABORT, 'HoD must have role HOD'); END;
CREATE TRIGGER trg_dept_hod_upd BEFORE UPDATE OF hod_user_id ON department
WHEN NEW.hod_user_id IS NOT NULL
 AND (SELECT role FROM app_user WHERE user_id = NEW.hod_user_id) <> 'HOD'
BEGIN SELECT RAISE(ABORT, 'HoD must have role HOD'); END;
