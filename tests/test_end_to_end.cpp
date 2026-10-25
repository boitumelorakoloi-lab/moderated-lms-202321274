// End-to-end integration test: one assignment from creation to released marks through all
// six services (SDD Figure 4, activity diagram).
#include "TestFixture.h"

namespace lms::test {

class EndToEnd : public LmsTest {};

// IT-70: the full activity diagram, including a late submission, a return and an adjustment.
TEST_F(EndToEnd, SubmissionToReleasedMark) {
    auto lect = auth.login("lect", kPassword, kNow - hours(48));
    const int a = assignments.createAssignment(static_cast<Lecturer&>(*lect), course, "Essay", kNow, 50);

    const int sub1 = submit().submit(s1(), a, goodFile, kNow - hours(1));   // on time
    const int sub2 = submit().submit(s2(), a, goodFile, kNow + hours(30));  // 2 days late

    EXPECT_DOUBLE_EQ(marking.mark(lecturer(), sub1, 40, "Good", std::nullopt, kNow), 40.0);
    EXPECT_DOUBLE_EQ(marking.mark(lecturer(), sub2, 38, "Fair", std::nullopt, kNow), 28.0);

    moderation.returnForRemarking(hod(), sub2, "Check question 3", kNow);
    EXPECT_THROW(moderation.release(hod(), a, kNow), ValidationException);
    marking.mark(lecturer(), sub2, 40, "Fair", std::string("Rechecked Q3"), kNow);
    moderation.approve(hod(), sub1, kNow);
    EXPECT_DOUBLE_EQ(moderation.adjust(hod(), sub2, 45, "Rubric", kNow), 35.0);

    EXPECT_TRUE(submit().myResults(s1()).empty());  // nothing visible before release
    moderation.release(hod(), a, kNow);
    ASSERT_EQ(submit().myResults(s2()).size(), 1u);
    EXPECT_DOUBLE_EQ(submit().myResults(s2())[0].view.finalMark, 35.0);

    const auto log = moderation.viewAuditLog(hod(), sub2);
    ASSERT_EQ(log.size(), 5u);
    EXPECT_EQ(log[0].action(), "MARK");
    EXPECT_EQ(log[1].action(), "RETURN");
    EXPECT_EQ(log[2].action(), "REMARK");
    EXPECT_EQ(log[3].action(), "HOD_ADJUST");
    EXPECT_EQ(log[4].action(), "RELEASE");
}

}  // namespace lms::test
