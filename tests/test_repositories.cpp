// Integration tests for the data-access layer: transactions, constraints, triggers and
// persistence to a real file (NFR-05, NFR-07, NFR-11, FR-21).
#include <sqlite3.h>

#include "TestFixture.h"

namespace lms::test {

class RepositoryTest : public LmsTest {};

// IT-60: a failed save is fully undone (NFR-07). The audit insert fails (a REMARK with no
// reason breaks the CHECK constraint), so the mark and the state change must not be kept.
TEST_F(RepositoryTest, FailedSaveIsRolledBack) {
    const int a = makeAssignment();
    const int sub = submit().submit(s1(), a, goodFile, kNow);
    const Mark m(sub, 30, "Ok", policy.code(), lecturer().id(), kNow);
    const AuditEntry bad(sub, lecturer().id(), kNow, "REMARK", 20, 30, std::nullopt);
    EXPECT_THROW(marks.saveChange(m, SubmissionStatus::Marked, bad), DatabaseException);
    EXPECT_FALSE(marks.find(sub).has_value());
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Submitted);
    EXPECT_TRUE(marks.listAudit(sub).empty());
}

// IT-61: the raw mark trigger stops a mark above max_mark even if a service forgot to check.
TEST_F(RepositoryTest, TriggerStopsMarkAboveMaximum) {
    const int a = makeAssignment(50);
    const int sub = submit().submit(s1(), a, goodFile, kNow);
    const Mark m(sub, 60, "Ok", policy.code(), lecturer().id(), kNow);
    EXPECT_THROW(marks.saveChange(m, SubmissionStatus::Marked,
                                  AuditEntry(sub, lecturer().id(), kNow, "MARK", std::nullopt, 60,
                                             std::nullopt)),
                 DatabaseException);
}

// IT-62: the audit log cannot be edited or deleted, even with direct SQL (FR-21).
TEST_F(RepositoryTest, AuditLogIsAppendOnly) {
    const int a = makeAssignment();
    const int sub = markedSubmission(a);
    EXPECT_NE(sqlite3_exec(db.handle(), "UPDATE audit_entry SET new_mark = 50", nullptr, nullptr,
                           nullptr),
              SQLITE_OK);
    EXPECT_NE(sqlite3_exec(db.handle(), "DELETE FROM audit_entry", nullptr, nullptr, nullptr),
              SQLITE_OK);
    EXPECT_EQ(marks.listAudit(sub).size(), 1u);
}

// IT-63: foreign keys and the HoD-role trigger are enforced in the database itself.
TEST_F(RepositoryTest, ConstraintsEnforced) {
    EXPECT_THROW(courses.enrol(999, course), DatabaseException);           // no such user
    EXPECT_THROW(courses.setDepartmentHod(sciences, s1().id()), DatabaseException);  // not HOD
    EXPECT_THROW(users.add(*UserFactory::create(Role::Student, 0, "STU1", "Dup", testHash())),
                 DatabaseException);                                         // case-insensitive UNIQUE
}

// IT-64: data persists in a real file, in WAL mode, and the schema is created only once.
TEST_F(RepositoryTest, PersistsToFileInWalMode) {
    const std::string path = (folder / "persist.db").string();
    {
        Database fileDb(path);
        SqliteUserRepository repo(fileDb);
        repo.add(*UserFactory::create(Role::Student, 0, "saved", "Saved User", testHash()));
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(fileDb.handle(), "PRAGMA journal_mode", -1, &st, nullptr);
        ASSERT_EQ(sqlite3_step(st), SQLITE_ROW);
        EXPECT_STREQ(reinterpret_cast<const char*>(sqlite3_column_text(st, 0)), "wal");
        sqlite3_finalize(st);
    }
    Database reopened(path);  // opening again must not recreate or wipe the tables
    SqliteUserRepository repo(reopened);
    auto u = repo.findByUsername("saved");
    ASSERT_NE(u, nullptr);
    EXPECT_EQ(u->fullName(), "Saved User");
}

// IT-65: a database file that cannot be opened gives a DatabaseException, not a crash.
TEST_F(RepositoryTest, BadPathThrows) {
    EXPECT_THROW(Database((folder / "no_such_dir" / "x.db").string()), DatabaseException);
}

}  // namespace lms::test
