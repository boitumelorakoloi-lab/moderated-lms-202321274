// SqliteMarkRepository.cpp
#include "data/SqliteMarkRepository.h"

namespace lms {

namespace {
const char* const kSelectAudit =
    "SELECT a.submission_id, a.changed_by, a.changed_at, a.action, a.old_mark, a.new_mark, "
    "       a.reason "
    "FROM audit_entry a ";

std::vector<AuditEntry> readAudit(Statement& st) {
    std::vector<AuditEntry> result;
    while (st.step()) {
        result.emplace_back(st.getInt(0), st.getInt(1), st.getTime(2), st.getText(3),
                            st.getOptionalInt(4), st.getOptionalInt(5), st.getOptionalText(6));
    }
    return result;
}
}  // namespace

std::optional<Mark> SqliteMarkRepository::find(int submissionId) {
    Statement st(db_,
                 "SELECT submission_id, raw_mark, feedback, policy_code, marked_by, marked_at, "
                 "moderated_by, moderated_at, hod_comment FROM mark WHERE submission_id = ?1");
    st.bind(1, submissionId);
    if (!st.step()) return std::nullopt;
    Mark m(st.getInt(0), st.getInt(1), st.getText(2), st.getText(3), st.getInt(4), st.getTime(5));
    if (!st.isNull(6)) {
        m.setModerated(st.getInt(6), st.getTime(7));
    }
    m.setHodComment(st.getOptionalText(8));
    return m;
}

void SqliteMarkRepository::saveChange(const Mark& m, SubmissionStatus status, const AuditEntry& e) {
    Transaction tx(db_);  // all three writes succeed together or not at all (NFR-07)
    {
        Statement st(db_,
                     "INSERT INTO mark (submission_id, raw_mark, feedback, policy_code, marked_by, "
                     "marked_at, moderated_by, moderated_at, hod_comment) "
                     "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9) "
                     "ON CONFLICT (submission_id) DO UPDATE SET raw_mark = excluded.raw_mark, "
                     "feedback = excluded.feedback, policy_code = excluded.policy_code, "
                     "marked_by = excluded.marked_by, marked_at = excluded.marked_at, "
                     "moderated_by = excluded.moderated_by, moderated_at = excluded.moderated_at, "
                     "hod_comment = excluded.hod_comment");
        st.bind(1, m.submissionId())
            .bind(2, m.rawMark())
            .bind(3, m.feedback())
            .bind(4, m.policyCode())
            .bind(5, m.markedBy())
            .bindTime(6, m.markedAt())
            .bindOptional(7, m.moderatedBy())
            .bindOptionalTime(8, m.moderatedAt())
            .bindOptional(9, m.hodComment());
        st.run();
    }
    {
        Statement st(db_, "UPDATE submission SET status = ?1 WHERE submission_id = ?2");
        st.bind(1, statusCode(status)).bind(2, m.submissionId());
        st.run();
    }
    {
        Statement st(db_,
                     "INSERT INTO audit_entry (submission_id, changed_by, changed_at, action, "
                     "old_mark, new_mark, reason) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)");
        st.bind(1, e.submissionId())
            .bind(2, e.changedBy())
            .bindTime(3, e.changedAt())
            .bind(4, e.action())
            .bindOptional(5, e.oldMark())
            .bindOptional(6, e.newMark())
            .bindOptional(7, e.reason());
        st.run();
    }
    tx.commit();
}

std::vector<AuditEntry> SqliteMarkRepository::listAudit(int submissionId) {
    Statement st(db_, (std::string(kSelectAudit) +
                       "WHERE a.submission_id = ?1 ORDER BY a.changed_at, a.audit_id").c_str());
    st.bind(1, submissionId);
    return readAudit(st);
}

}  // namespace lms
