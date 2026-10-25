// Integration tests for AssignmentService (UC-06, UC-07, UC-16, FR-07 to FR-09, BR-03).
#include "TestFixture.h"

namespace lms::test {

class AssignmentServiceTest : public LmsTest {};

// IT-20: maximum mark boundaries 0 / 1 / 100 / 101 and other create rules (FR-07).
TEST_F(AssignmentServiceTest, CreateAssignmentRules) {
    EXPECT_THROW(assignments.createAssignment(lecturer(), course, "A", kNow, 0), ValidationException);
    EXPECT_NO_THROW(assignments.createAssignment(lecturer(), course, "A", kNow, 1));
    EXPECT_NO_THROW(assignments.createAssignment(lecturer(), course, "B", kNow, 100));
    EXPECT_THROW(assignments.createAssignment(lecturer(), course, "C", kNow, 101), ValidationException);
    EXPECT_THROW(assignments.createAssignment(lecturer(), course, "   ", kNow, 10), ValidationException);
    EXPECT_THROW(assignments.createAssignment(lecturer(), 999, "D", kNow, 10), ValidationException);
    EXPECT_THROW(assignments.createAssignment(lecturer(), otherCourse, "E", kNow, 10),
                 AccessDeniedException);  // does not teach it
    EXPECT_EQ(assignments.listForCourse(lecturer(), course).size(), 2u);
    EXPECT_THROW(assignments.listForCourse(lecturer(), otherCourse), AccessDeniedException);
    EXPECT_EQ(assignments.listMyCourses(lecturer()).size(), 1u);
}

// IT-21: extension input rules (BR-03).
TEST_F(AssignmentServiceTest, ExtensionInputRules) {
    const int a = makeAssignment();
    EXPECT_THROW(assignments.grantExtension(lecturer(), 999, s1().id(), kNow + hours(24), "R", kNow),
                 ValidationException);
    EXPECT_THROW(assignments.grantExtension(lecturer(), a, lecturer().id(), kNow + hours(24), "R", kNow),
                 ValidationException);  // not an enrolled student
    EXPECT_THROW(assignments.grantExtension(lecturer(), a, s1().id(), kNow + hours(24), "  ", kNow),
                 ValidationException);  // reason required
    EXPECT_THROW(assignments.grantExtension(lecturer(), a, s1().id(), kNow, "Same time", kNow - hours(1)),
                 ValidationException);  // boundary: equal to the normal deadline
    EXPECT_NO_THROW(assignments.grantExtension(lecturer(), a, s1().id(), kNow + seconds(1), "Just after",
                                               kNow - hours(1)));
}

// IT-22: only a lecturer who teaches the course may give an extension.
TEST_F(AssignmentServiceTest, OnlyCourseLecturer) {
    const int a = makeAssignment();
    EXPECT_THROW(assignments.grantExtension(otherHod(), a, s1().id(), kNow + hours(24), "R", kNow),
                 AccessDeniedException);
}

// IT-23: an extension that has already passed is refused when nothing was handed in.
TEST_F(AssignmentServiceTest, ExpiredExtensionWithNoSubmission) {
    const int a = makeAssignment();
    EXPECT_THROW(assignments.grantExtension(lecturer(), a, s1().id(), kNow + hours(1), "R",
                                            kNow + hours(2)),
                 ValidationException);
}

// IT-24: Late -> Submitted when the extension covers the work (state machine, SDD Figure 3).
TEST_F(AssignmentServiceTest, ExtensionCoversLateWork) {
    const int a = makeAssignment();
    const int sub = submit().submit(s1(), a, goodFile, kNow + hours(5));
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Late);
    assignments.grantExtension(lecturer(), a, s1().id(), kNow + hours(4), "Too short", kNow + hours(6));
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Late);       // does not cover
    assignments.grantExtension(lecturer(), a, s1().id(), kNow + hours(24), "Sick note", kNow + hours(6));
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Submitted);  // covers
    EXPECT_EQ(courses.findExtension(a, s1().id())->reason(), "Sick note");        // replaced
}

// IT-25: no extension once the work is marked; none earlier than on-time work.
TEST_F(AssignmentServiceTest, ExtensionAfterMarkingOrTooEarly) {
    const int a = makeAssignment();
    markedSubmission(a);
    EXPECT_THROW(assignments.grantExtension(lecturer(), a, s1().id(), kNow + hours(24), "R", kNow),
                 ValidationException);
    // stu2 used an extension to +24h and handed in at +20h. Shortening it to +12h would make
    // that work late after the fact, so it is refused.
    assignments.grantExtension(lecturer(), a, s2().id(), kNow + hours(24), "Sick", kNow);
    submit().submit(s2(), a, goodFile, kNow + hours(20));
    EXPECT_THROW(assignments.grantExtension(lecturer(), a, s2().id(), kNow + hours(12), "Shorter",
                                            kNow + hours(21)),
                 ValidationException);
}

// IT-26: student overview shows deadlines, extensions and states (FR-09); only own courses.
TEST_F(AssignmentServiceTest, StudentOverview) {
    const int a = makeAssignment();
    assignments.createAssignment(lecturer(), course, "Second", kNow + hours(48), 20);
    assignments.grantExtension(lecturer(), a, s1().id(), kNow + hours(24), "Sick", kNow - hours(1));
    submit().submit(s1(), a, goodFile, kNow + hours(1));

    const auto rows = assignments.overviewForStudent(s1());
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].courseCode, "MATH");
    EXPECT_TRUE(rows[0].hasExtension);
    EXPECT_EQ(rows[0].effectiveDeadline, kNow + hours(24));
    EXPECT_EQ(rows[0].status, SubmissionStatus::Submitted);  // on time thanks to the extension
    EXPECT_FALSE(rows[1].hasExtension);
    EXPECT_FALSE(rows[1].status.has_value());               // not submitted

    const auto s2rows = assignments.overviewForStudent(s2());
    EXPECT_FALSE(s2rows[0].hasExtension);                    // extensions are per student
    EXPECT_EQ(assignments.listCoursesForStudent(s1()).size(), 1u);
}

}  // namespace lms::test
