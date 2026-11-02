// Tests for change request CR1, release v1.1 (FR-27, BR-06, FR-09 as changed).
// "A lecturer should be able to extend the deadline for a whole class, not just an individual
// student, and extensions should be shown on the student's assignment list."
#include "TestFixture.h"

namespace lms::test {

class ClassExtensionTest : public LmsTest {};

// UT-19: an Extension with no student is a whole-class extension.
TEST(ExtensionTest, WholeClassFlag) {
    const Extension mine(1, 7, kNow, "Ill", 3);
    const Extension all(1, std::nullopt, kNow, "Exam clash", 3);
    EXPECT_FALSE(mine.forWholeClass());
    EXPECT_EQ(mine.studentId(), 7);
    EXPECT_TRUE(all.forWholeClass());
    EXPECT_FALSE(all.studentId().has_value());
}

// IT-80: the new deadline applies to every enrolled student (FR-27).
TEST_F(ClassExtensionTest, AppliesToEveryStudent) {
    const int a = makeAssignment();
    EXPECT_EQ(
        assignments.extendForClass(lecturer(), a, kNow + hours(48), "Exam clash", kNow - hours(1)),
        0);
    for (Student* s : {&s1(), &s2()}) {
        const auto rows = assignments.overviewForStudent(*s);
        ASSERT_EQ(rows.size(), 1u);
        EXPECT_EQ(rows[0].effectiveDeadline, kNow + hours(48));
    }
    const auto saved = assignments.classExtension(lecturer(), a);
    ASSERT_TRUE(saved.has_value());
    EXPECT_EQ(saved->reason(), "Exam clash");
}

// IT-81: the student's list shows the extension, its type and reason (CR1 second part, FR-09).
TEST_F(ClassExtensionTest, ShownOnStudentList) {
    const int a = makeAssignment();
    const int b = assignments.createAssignment(lecturer(), course, "Other", kNow, 10);
    assignments.extendForClass(lecturer(), a, kNow + hours(24), "Trip", kNow - hours(1));
    assignments.grantExtension(lecturer(), b, s1().id(), kNow + hours(5), "Ill", kNow - hours(1));

    const auto rows = assignments.overviewForStudent(s1());
    ASSERT_EQ(rows.size(), 2u);
    for (const auto& r : rows) {
        ASSERT_TRUE(r.extension.has_value());
        if (r.assignment.id() == a) {
            EXPECT_TRUE(r.extension->forWholeClass());
            EXPECT_EQ(r.extension->reason(), "Trip");
        } else {
            EXPECT_FALSE(r.extension->forWholeClass());
        }
    }
    // stu2 sees the class extension but not stu1's personal one.
    for (const auto& r : assignments.overviewForStudent(s2())) {
        EXPECT_EQ(r.extension.has_value(), r.assignment.id() == a);
    }
}

// IT-82: own and class extension together - the later one wins (BR-06).
TEST_F(ClassExtensionTest, LaterDeadlineWins) {
    const int a = makeAssignment();
    assignments.grantExtension(lecturer(), a, s1().id(), kNow + hours(72), "Ill", kNow - hours(1));
    assignments.extendForClass(lecturer(), a, kNow + hours(24), "Trip", kNow - hours(1));
    EXPECT_EQ(assignments.overviewForStudent(s1())[0].effectiveDeadline, kNow + hours(72));
    EXPECT_EQ(assignments.overviewForStudent(s2())[0].effectiveDeadline, kNow + hours(24));

    // A later class extension now beats stu1's own one: nobody is worse off than the class.
    assignments.extendForClass(lecturer(), a, kNow + hours(96), "Strike", kNow - hours(1));
    const auto row = assignments.overviewForStudent(s1())[0];
    EXPECT_EQ(row.effectiveDeadline, kNow + hours(96));
    EXPECT_TRUE(row.extension->forWholeClass());
}

// IT-83: work handed in late becomes on time (Late -> Submitted) and gets no penalty.
TEST_F(ClassExtensionTest, LateWorkBecomesOnTime) {
    const int a = makeAssignment();
    const int late = submit().submit(s1(), a, goodFile, kNow + hours(10));
    const int tooLate = submit().submit(s2(), a, goodFile, kNow + hours(30));
    ASSERT_EQ(submissions.findById(late)->status(), SubmissionStatus::Late);

    EXPECT_EQ(
        assignments.extendForClass(lecturer(), a, kNow + hours(24), "Power cut", kNow + hours(31)),
        1);
    EXPECT_EQ(submissions.findById(late)->status(), SubmissionStatus::Submitted);
    EXPECT_EQ(submissions.findById(tooLate)->status(), SubmissionStatus::Late);

    // Marking uses the class deadline: 0 days late for stu1, 1 day late for stu2 (BR-02).
    EXPECT_DOUBLE_EQ(marking.mark(lecturer(), late, 40, "Good", std::nullopt, kNow + hours(40)),
                     40.0);
    EXPECT_DOUBLE_EQ(marking.mark(lecturer(), tooLate, 40, "Good", std::nullopt, kNow + hours(40)),
                     35.0);
}

// IT-84: new work handed in after the old deadline but before the class one is on time.
TEST_F(ClassExtensionTest, NewWorkUsesClassDeadline) {
    const int a = makeAssignment();
    assignments.extendForClass(lecturer(), a, kNow + hours(24), "Trip", kNow - hours(1));
    const CheckResult r = submit().check(s2(), a, goodFile, kNow + hours(2));
    EXPECT_EQ(r.result, SubmissionCheck::OnTime);
    EXPECT_EQ(r.deadline, kNow + hours(24));
    const int sub = submit().submit(s2(), a, goodFile, kNow + hours(2));
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Submitted);
}

// IT-85: input rules, with boundaries (BR-06).
TEST_F(ClassExtensionTest, InputRules) {
    const int a = makeAssignment();
    const TimePoint before = kNow - hours(1);
    EXPECT_THROW(assignments.extendForClass(lecturer(), 999, kNow + hours(5), "R", before),
                 ValidationException);  // no such assignment
    EXPECT_THROW(assignments.extendForClass(lecturer(), a, kNow + hours(5), "   ", before),
                 ValidationException);  // reason required
    EXPECT_THROW(assignments.extendForClass(lecturer(), a, kNow, "R", before),
                 ValidationException);  // boundary: equal to the normal deadline
    EXPECT_THROW(assignments.extendForClass(lecturer(), a, kNow + hours(1), "R", kNow + hours(2)),
                 ValidationException);  // already passed
    EXPECT_NO_THROW(assignments.extendForClass(lecturer(), a, kNow + seconds(1), "R", before));
    // An existing class extension can only move later (equal is refused, one second later is ok).
    EXPECT_THROW(assignments.extendForClass(lecturer(), a, kNow + seconds(1), "R", before),
                 ValidationException);
    EXPECT_NO_THROW(assignments.extendForClass(lecturer(), a, kNow + seconds(2), "R", before));
}

// IT-86: only a lecturer who teaches the course (access control, NFR-08).
TEST_F(ClassExtensionTest, OnlyCourseLecturer) {
    const int a = makeAssignment();
    EXPECT_THROW(assignments.extendForClass(otherHod(), a, kNow + hours(5), "R", kNow - hours(1)),
                 AccessDeniedException);
    EXPECT_THROW(assignments.classExtension(otherHod(), a), AccessDeniedException);
}

// IT-87: refused once marking has started (the marks would change under the moderator).
TEST_F(ClassExtensionTest, RefusedAfterMarkingStarts) {
    const int a = makeAssignment();
    markedSubmission(a);
    EXPECT_THROW(assignments.extendForClass(lecturer(), a, kNow + hours(5), "R", kNow),
                 ValidationException);
    EXPECT_FALSE(courses.findClassExtension(a).has_value());  // nothing was saved
}

// IT-88: a v1.0 database file is upgraded when v1.1 opens it, keeping its data.
TEST(DatabaseUpgradeTest, OldDatabaseGetsNewTable) {
    const fs::path file = fs::temp_directory_path() / "lms_upgrade_test.db";
    fs::remove(file);
    {
        Database db(file.string());
        db.executeScript(
            "INSERT INTO app_user (username, full_name, password_hash, role, created_at) "
            "VALUES ('keep', 'Kept User', 'x', 'ADMIN', 0);");
        db.executeScript("DROP TABLE class_extension;");  // now it looks like a v1.0 file
    }
    {
        Database db(file.string());
        SqliteCourseRepository courses(db);
        EXPECT_FALSE(courses.findClassExtension(1).has_value());  // table exists again
        SqliteUserRepository users(db);
        EXPECT_NE(users.findByUsername("keep"), nullptr);  // old data kept
    }
    fs::remove(file);
    fs::remove(file.string() + "-wal");
    fs::remove(file.string() + "-shm");
}

}  // namespace lms::test
