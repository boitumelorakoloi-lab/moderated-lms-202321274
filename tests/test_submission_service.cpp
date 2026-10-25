// Integration tests for SubmissionService (UC-17 to UC-21, FR-10 to FR-13, FR-20, BR-02, BR-04).
#include "TestFixture.h"

namespace lms::test {

class SubmissionServiceTest : public LmsTest {};

// IT-30: file size boundaries 0 / 1 byte / 10 MB / 10 MB + 1 (FR-13, UC-18).
TEST_F(SubmissionServiceTest, FileSizeBoundaries) {
    const int a = makeAssignment();
    const auto max = static_cast<std::uintmax_t>(SubmissionService::kMaxFileBytes);
    EXPECT_THROW(submit().check(s1(), a, makeFile("empty.txt", 0), kNow), ValidationException);
    EXPECT_NO_THROW(submit().check(s1(), a, makeFile("one.txt", 1), kNow));
    EXPECT_NO_THROW(submit().check(s1(), a, makeFile("max.pdf", max), kNow));
    EXPECT_THROW(submit().check(s1(), a, makeFile("big.pdf", max + 1), kNow), ValidationException);
}

// IT-31: file type partitions - allowed types (any case) and rejected types; missing file.
TEST_F(SubmissionServiceTest, FileTypes) {
    const int a = makeAssignment();
    for (const char* ok : {"a.pdf", "b.docx", "c.txt", "d.zip", "E.PDF", "f.DocX"}) {
        EXPECT_NO_THROW(submit().check(s1(), a, makeFile(ok, 10), kNow)) << ok;
    }
    for (const char* bad : {"a.exe", "b.doc", "c.jpg", "noextension", "d.pdf.exe"}) {
        EXPECT_THROW(submit().check(s1(), a, makeFile(bad, 10), kNow), ValidationException) << bad;
    }
    EXPECT_THROW(submit().check(s1(), a, (folder / "missing.pdf").string(), kNow), ValidationException);
    EXPECT_THROW(submit().check(s1(), a, "   ", kNow), ValidationException);
    EXPECT_THROW(submit().check(s1(), a, folder.string(), kNow), ValidationException);  // a folder
}

// IT-32: UC-17 main flow and the deadline boundary (exactly at the deadline is on time).
TEST_F(SubmissionServiceTest, OnTimeAtDeadlineBoundary) {
    const int a = makeAssignment();
    const CheckResult r = submit().check(s1(), a, goodFile, kNow);
    EXPECT_EQ(r.result, SubmissionCheck::OnTime);
    EXPECT_EQ(r.deadline, kNow);
    EXPECT_EQ(r.nextVersion, 1);
    const int id = submit().submit(s1(), a, goodFile, kNow);
    const auto sub = submissions.findById(id);
    EXPECT_EQ(sub->status(), SubmissionStatus::Submitted);
    EXPECT_EQ(sub->latest().submittedAt(), kNow);
    EXPECT_EQ(sub->latest().originalName(), "work.txt");
    EXPECT_TRUE(fs::exists(sub->latest().storedPath()));  // a copy was saved
}

// IT-33: A1 resubmission before the deadline keeps the old version (FR-11, BR-04).
TEST_F(SubmissionServiceTest, ResubmitKeepsVersions) {
    const int a = makeAssignment();
    const int id = submit().submit(s1(), a, goodFile, kNow - hours(2));
    EXPECT_EQ(submit().check(s1(), a, goodFile, kNow - hours(1)).result, SubmissionCheck::Resubmit);
    EXPECT_EQ(submit().submit(s1(), a, goodFile, kNow - hours(1)), id);
    EXPECT_EQ(submit().submit(s1(), a, goodFile, kNow), id);
    const auto sub = submissions.findById(id);
    ASSERT_EQ(sub->versions().size(), 3u);
    EXPECT_EQ(sub->latest().versionNo(), 3);
    EXPECT_NE(sub->versions()[0].storedPath(), sub->versions()[2].storedPath());
}

// IT-34: A2 late submission one second after the deadline, with the days late (BR-02).
TEST_F(SubmissionServiceTest, LateOneSecondAfter) {
    const int a = makeAssignment();
    const CheckResult r = submit().check(s1(), a, goodFile, kNow + seconds(1));
    EXPECT_EQ(r.result, SubmissionCheck::Late);
    EXPECT_EQ(r.daysLate, 1);
    const int id = submit().submit(s1(), a, goodFile, kNow + seconds(1));
    EXPECT_EQ(submissions.findById(id)->status(), SubmissionStatus::Late);
}

// IT-35: E2 resubmission after the deadline is refused; late work cannot be resubmitted.
TEST_F(SubmissionServiceTest, NoResubmitAfterDeadline) {
    const int a = makeAssignment();
    submit().submit(s1(), a, goodFile, kNow - hours(1));
    EXPECT_EQ(submit().check(s1(), a, goodFile, kNow + seconds(1)).result, SubmissionCheck::Rejected);
    EXPECT_THROW(submit().submit(s1(), a, goodFile, kNow + seconds(1)), ValidationException);

    submit().submit(s2(), a, goodFile, kNow + hours(1));  // late, once
    EXPECT_THROW(submit().submit(s2(), a, goodFile, kNow + hours(2)), ValidationException);
    EXPECT_EQ(submissions.find(a, s2().id())->versions().size(), 1u);
}

// IT-36: an extension moves the student's deadline (effective deadline).
TEST_F(SubmissionServiceTest, ExtensionChangesDeadline) {
    const int a = makeAssignment();
    assignments.grantExtension(lecturer(), a, s1().id(), kNow + hours(24), "Sick", kNow - hours(1));
    EXPECT_EQ(submit().check(s1(), a, goodFile, kNow + hours(23)).result, SubmissionCheck::OnTime);
    EXPECT_EQ(submit().check(s2(), a, goodFile, kNow + hours(23)).result, SubmissionCheck::Late);
}

// IT-37: access rules - not enrolled, unknown assignment, marked work, released assignment.
TEST_F(SubmissionServiceTest, Rejections) {
    const int other = assignments.createAssignment(lecturer(), course, "X", kNow, 10);
    const int stranger = addUser(Role::Student, "stranger", "Not Enrolled");
    auto st = users.findById(stranger);
    EXPECT_THROW(submit().check(static_cast<Student&>(*st), other, goodFile, kNow - hours(1)),
                 AccessDeniedException);
    EXPECT_THROW(submit().check(s1(), 999, goodFile, kNow), ValidationException);

    const int a = makeAssignment();
    markedSubmission(a);
    EXPECT_EQ(submit().check(s1(), a, goodFile, kNow - minutes(1)).result, SubmissionCheck::Rejected);
}

// IT-38: E3 - if the database save fails, the copied file is deleted again.
TEST_F(SubmissionServiceTest, FailedSaveRemovesCopiedFile) {
    const int a = makeAssignment();
    // Simulate a database failure: make every insert into submission fail.
    db.executeScript("CREATE TRIGGER fail_insert BEFORE INSERT ON submission "
                     "BEGIN SELECT RAISE(ABORT, 'disk error'); END;");
    EXPECT_THROW(submit().submit(s1(), a, goodFile, kNow), DatabaseException);
    EXPECT_FALSE(submissions.find(a, s1().id()).has_value());
    const fs::path store = folder / "store";
    const bool empty = !fs::exists(store) || fs::is_empty(store);
    EXPECT_TRUE(empty);
}

// IT-39: students only see released marks, and only their own (FR-20, NFR-08).
TEST_F(SubmissionServiceTest, OnlyReleasedOwnResults) {
    const int a = makeAssignment();
    const int sub = markedSubmission(a, 40);
    EXPECT_TRUE(submit().myResults(s1()).empty());           // marked, not released
    moderation.approve(hod(), sub, kNow);
    EXPECT_TRUE(submit().myResults(s1()).empty());           // moderated, not released
    moderation.release(hod(), a, kNow);
    const auto mine = submit().myResults(s1());
    ASSERT_EQ(mine.size(), 1u);
    EXPECT_DOUBLE_EQ(mine[0].view.finalMark, 40.0);
    EXPECT_EQ(mine[0].view.mark->feedback(), "Good work");
    EXPECT_TRUE(submit().myResults(s2()).empty());           // not stu2's work
}

}  // namespace lms::test
