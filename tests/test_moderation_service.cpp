// Integration tests for ModerationService (UC-11 to UC-15, FR-17 to FR-19, FR-21, FR-22).
#include "TestFixture.h"

namespace lms::test {

class ModerationServiceTest : public LmsTest {
protected:
    int a = 0;
    int sub = 0;
    void SetUp() override {
        LmsTest::SetUp();
        a = makeAssignment(50);
        sub = markedSubmission(a, 40);
    }
};

// IT-50: E1 - an HoD of another department, or an HoD with no department, is refused.
TEST_F(ModerationServiceTest, OtherDepartmentRefused) {
    EXPECT_THROW(moderation.listForModeration(otherHod(), a), AccessDeniedException);
    EXPECT_THROW(moderation.approve(otherHod(), sub, kNow), AccessDeniedException);
    EXPECT_THROW(moderation.release(otherHod(), a, kNow), AccessDeniedException);
    const int noDept = addUser(Role::HeadOfDepartment, "hod9", "No Dept");
    auto h = users.findById(noDept);
    EXPECT_THROW(moderation.listForModeration(static_cast<HeadOfDepartment&>(*h), a),
                 AccessDeniedException);
    EXPECT_TRUE(moderation.listAssignmentsInDepartment(static_cast<HeadOfDepartment&>(*h)).empty());
    EXPECT_THROW(moderation.listForModeration(hod(), 999), ValidationException);
}

// IT-51: the HoD sees raw mark, penalty, final mark, feedback and extension (FR-17).
TEST_F(ModerationServiceTest, ListShowsEverything) {
    assignments.grantExtension(lecturer(), a, s2().id(), kNow + hours(10), "Sick", kNow);
    const int late = submit().submit(s2(), a, goodFile, kNow + hours(30));  // 1 day after extension
    marking.mark(lecturer(), late, 30, "Fair", std::nullopt, kNow + hours(31));
    const auto rows = moderation.listForModeration(hod(), a);
    ASSERT_EQ(rows.size(), 2u);
    const MarkView& v = rows[1];
    EXPECT_EQ(v.mark->rawMark(), 30);
    EXPECT_EQ(v.daysLate, 1);
    EXPECT_DOUBLE_EQ(v.penalty, 5.0);
    EXPECT_DOUBLE_EQ(v.finalMark, 25.0);
    EXPECT_EQ(v.mark->feedback(), "Fair");
    ASSERT_TRUE(v.extension.has_value());
    EXPECT_EQ(v.extension->reason(), "Sick");
    EXPECT_EQ(moderation.listAssignmentsInDepartment(hod()).size(), 1u);
}

// IT-52: approve sets Moderated and records who approved (UC-11 main flow).
TEST_F(ModerationServiceTest, Approve) {
    moderation.approve(hod(), sub, kNow);
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Moderated);
    EXPECT_EQ(marks.find(sub)->moderatedBy().value(), hod().id());
    EXPECT_EQ(marks.listAudit(sub).back().action(), "APPROVE");
    EXPECT_THROW(moderation.approve(hod(), sub, kNow), InvalidTransitionException);  // twice
}

// IT-53: adjust - range boundaries, reason required, new final mark (UC-12, BR-05).
TEST_F(ModerationServiceTest, Adjust) {
    EXPECT_THROW(moderation.adjust(hod(), sub, -1, "R", kNow), ValidationException);
    EXPECT_THROW(moderation.adjust(hod(), sub, 51, "R", kNow), ValidationException);
    EXPECT_THROW(moderation.adjust(hod(), sub, 45, " ", kNow), ValidationException);
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Marked);  // nothing changed
    EXPECT_DOUBLE_EQ(moderation.adjust(hod(), sub, 50, "Rubric misapplied", kNow), 50.0);
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Moderated);
    const AuditEntry last = marks.listAudit(sub).back();
    EXPECT_EQ(last.action(), "HOD_ADJUST");
    EXPECT_EQ(last.oldMark().value(), 40);
    EXPECT_EQ(last.newMark().value(), 50);
    EXPECT_EQ(last.reason().value(), "Rubric misapplied");
}

// IT-54: return for re-marking needs a comment (UC-13).
TEST_F(ModerationServiceTest, ReturnNeedsComment) {
    EXPECT_THROW(moderation.returnForRemarking(hod(), sub, "  ", kNow), ValidationException);
    moderation.returnForRemarking(hod(), sub, "Check Q4", kNow);
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Returned);
    EXPECT_THROW(moderation.approve(hod(), sub, kNow), InvalidTransitionException);
}

// IT-55: release only when every submission is Moderated, and only once (FR-19, UC-14).
TEST_F(ModerationServiceTest, ReleaseRules) {
    const int empty = assignments.createAssignment(lecturer(), course, "Empty", kNow, 10);
    EXPECT_THROW(moderation.release(hod(), empty, kNow), ValidationException);  // nothing handed in

    const int sub2 = submit().submit(s2(), a, goodFile, kNow - hours(1));
    moderation.approve(hod(), sub, kNow);
    EXPECT_THROW(moderation.release(hod(), a, kNow), ValidationException);      // sub2 not marked
    marking.mark(lecturer(), sub2, 20, "Ok", std::nullopt, kNow);
    EXPECT_THROW(moderation.release(hod(), a, kNow), ValidationException);      // sub2 not approved
    moderation.approve(hod(), sub2, kNow);
    moderation.release(hod(), a, kNow + minutes(1));

    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Released);
    EXPECT_EQ(submissions.findById(sub2)->status(), SubmissionStatus::Released);
    EXPECT_EQ(courses.findAssignment(a)->releasedAt().value(), kNow + minutes(1));
    EXPECT_EQ(marks.listAudit(sub).back().action(), "RELEASE");
    EXPECT_THROW(moderation.release(hod(), a, kNow), ValidationException);      // only once
}

// IT-56: audit log access - HoD (own department), administrator yes; others no (FR-22).
TEST_F(ModerationServiceTest, AuditLogAccess) {
    EXPECT_EQ(moderation.viewAuditLog(hod(), sub).size(), 1u);
    EXPECT_EQ(moderation.viewAuditLog(ad(), sub).size(), 1u);
    EXPECT_THROW(moderation.viewAuditLog(otherHod(), sub), AccessDeniedException);
    EXPECT_THROW(moderation.viewAuditLog(lecturer(), sub), AccessDeniedException);
    EXPECT_THROW(moderation.viewAuditLog(s1(), sub), AccessDeniedException);
    EXPECT_THROW(moderation.viewAuditLog(ad(), 999), ValidationException);
}

}  // namespace lms::test
