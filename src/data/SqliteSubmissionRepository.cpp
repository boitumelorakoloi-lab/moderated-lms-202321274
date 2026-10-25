// SqliteSubmissionRepository.cpp
#include "data/SqliteSubmissionRepository.h"

namespace lms {

namespace {
const char* const kSelectSubmission =
    "SELECT submission_id, assignment_id, student_id, status FROM submission ";

Submission readSubmission(const Statement& st) {
    return Submission(st.getInt(0), st.getInt(1), st.getInt(2), statusFromCode(st.getText(3)));
}
}  // namespace

void SqliteSubmissionRepository::loadVersions(Submission& s) {
    Statement st(db_,
                 "SELECT version_no, stored_path, original_name, size_bytes, submitted_at "
                 "FROM submission_version WHERE submission_id = ?1 ORDER BY version_no");
    st.bind(1, s.id());
    while (st.step()) {
        s.addVersion(SubmissionVersion(st.getInt(0), st.getText(1), st.getText(2), st.getInt64(3),
                                       st.getTime(4)));
    }
}

std::optional<Submission> SqliteSubmissionRepository::find(int assignmentId, int studentId) {
    Statement st(db_, (std::string(kSelectSubmission) +
                       "WHERE assignment_id = ?1 AND student_id = ?2").c_str());
    st.bind(1, assignmentId).bind(2, studentId);
    if (!st.step()) return std::nullopt;
    Submission s = readSubmission(st);
    loadVersions(s);
    return s;
}

std::optional<Submission> SqliteSubmissionRepository::findById(int id) {
    Statement st(db_, (std::string(kSelectSubmission) + "WHERE submission_id = ?1").c_str());
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    Submission s = readSubmission(st);
    loadVersions(s);
    return s;
}

std::vector<Submission> SqliteSubmissionRepository::listForAssignment(int assignmentId) {
    std::vector<Submission> result;
    {
        Statement st(db_, (std::string(kSelectSubmission) +
                           "WHERE assignment_id = ?1 ORDER BY student_id").c_str());
        st.bind(1, assignmentId);
        while (st.step()) {
            result.push_back(readSubmission(st));
        }
    }
    for (Submission& s : result) {
        loadVersions(s);
    }
    return result;
}

void SqliteSubmissionRepository::insertVersion(int submissionId, const SubmissionVersion& v) {
    Statement st(db_,
                 "INSERT INTO submission_version (submission_id, version_no, stored_path, "
                 "original_name, size_bytes, submitted_at) VALUES (?1, ?2, ?3, ?4, ?5, ?6)");
    st.bind(1, submissionId)
        .bind(2, v.versionNo())
        .bind(3, v.storedPath())
        .bind(4, v.originalName())
        .bind(5, v.sizeBytes())
        .bindTime(6, v.submittedAt());
    st.run();
}

int SqliteSubmissionRepository::addSubmission(const Submission& s) {
    Transaction tx(db_);
    Statement st(db_, "INSERT INTO submission (assignment_id, student_id, status) VALUES (?1, ?2, ?3)");
    st.bind(1, s.assignmentId()).bind(2, s.studentId()).bind(3, statusCode(s.status()));
    st.run();
    const int id = static_cast<int>(db_.lastInsertId());
    for (const SubmissionVersion& v : s.versions()) {
        insertVersion(id, v);
    }
    tx.commit();
    return id;
}

void SqliteSubmissionRepository::addVersion(int submissionId, const SubmissionVersion& v) {
    insertVersion(submissionId, v);
}

void SqliteSubmissionRepository::updateStatus(int id, SubmissionStatus status) {
    Statement st(db_, "UPDATE submission SET status = ?1 WHERE submission_id = ?2");
    st.bind(1, statusCode(status)).bind(2, id);
    st.run();
}

int SqliteSubmissionRepository::countNotIn(int assignmentId, SubmissionStatus status) {
    Statement st(db_, "SELECT COUNT(*) FROM submission WHERE assignment_id = ?1 AND status <> ?2");
    st.bind(1, assignmentId).bind(2, statusCode(status));
    st.step();
    return st.getInt(0);
}

void SqliteSubmissionRepository::releaseAll(int assignmentId, int by, TimePoint at) {
    // One transaction: one RELEASE audit entry per submission, every submission set to
    // Released, and assignment.released_at set (SDD Figure 7).
    Transaction tx(db_);
    {
        Statement audit(db_,
                        "INSERT INTO audit_entry (submission_id, changed_by, changed_at, action, "
                        "old_mark, new_mark, reason) "
                        "SELECT s.submission_id, ?1, ?2, 'RELEASE', m.raw_mark, m.raw_mark, NULL "
                        "FROM submission s JOIN mark m ON m.submission_id = s.submission_id "
                        "WHERE s.assignment_id = ?3 AND s.status = 'MODERATED'");
        audit.bind(1, by).bindTime(2, at).bind(3, assignmentId);
        audit.run();
    }
    {
        Statement update(db_,
                         "UPDATE submission SET status = 'RELEASED' "
                         "WHERE assignment_id = ?1 AND status = 'MODERATED'");
        update.bind(1, assignmentId);
        update.run();
    }
    {
        Statement assignment(db_,
                             "UPDATE assignment SET released_by = ?1, released_at = ?2 "
                             "WHERE assignment_id = ?3");
        assignment.bind(1, by).bindTime(2, at).bind(3, assignmentId);
        assignment.run();
    }
    tx.commit();
}

}  // namespace lms
