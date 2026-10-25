// Integration tests for MarkingService (UC-08 to UC-10, FR-14 to FR-16, FR-21, BR-01, BR-05).
#include "TestFixture.h"

namespace lms::test {

class MarkingServiceTest : public LmsTest {
protected:
    int a = 0;
    int sub = 0;
    void SetUp() override {
        LmsTest::SetUp();
        a = makeAssignment(50);
        sub = submit().submit(s1(), a, goodFile, kNow - hours(1));
    }
};

// IT-40: raw mark boundaries -1 / 0 / max / max + 1 (FR-14).
TEST_F(MarkingServiceTest, RawMarkBoundaries) {
    EXPECT_THROW(marking.mark(lecturer(), sub, -1, "Ok", std::nullopt, kNow), ValidationException);
    EXPECT_THROW(marking.mark(lecturer(), sub, 51, "Ok", std::nullopt, kNow), ValidationException);
    EXPECT_FALSE(marks.find(sub).has_value());  // E1: nothing saved
    EXPECT_DOUBLE_EQ(marking.mark(lecturer(), sub, 0, "Nothing correct", std::nullopt, kNow), 0.0);
    EXPECT_DOUBLE_EQ(marking.mark(lecturer(), sub, 50, "Perfect", std::string("Rechecked"), kNow), 50.0);
}

// IT-41: written feedback is required (C8).
TEST_F(MarkingServiceTest, FeedbackRequired) {
    EXPECT_THROW(marking.mark(lecturer(), sub, 30, "", std::nullopt, kNow), ValidationException);
    EXPECT_THROW(marking.mark(lecturer(), sub, 30, "   ", std::nullopt, kNow), ValidationException);
}

// IT-42: first mark needs no reason; any change needs one (BR-05, FR-16); audit is written.
TEST_F(MarkingServiceTest, ChangeNeedsReasonAndIsAudited) {
    marking.mark(lecturer(), sub, 30, "Ok", std::nullopt, kNow);
    EXPECT_THROW(marking.mark(lecturer(), sub, 35, "Ok", std::nullopt, kNow), ValidationException);
    EXPECT_THROW(marking.mark(lecturer(), sub, 35, "Ok", std::string("  "), kNow), ValidationException);
    marking.mark(lecturer(), sub, 35, "Better", std::string("Missed page 2"), kNow + minutes(5));

    const auto log = marks.listAudit(sub);
    ASSERT_EQ(log.size(), 2u);
    EXPECT_EQ(log[0].action(), "MARK");
    EXPECT_FALSE(log[0].oldMark().has_value());
    EXPECT_EQ(log[0].newMark().value(), 30);
    EXPECT_EQ(log[1].action(), "REMARK");
    EXPECT_EQ(log[1].oldMark().value(), 30);
    EXPECT_EQ(log[1].newMark().value(), 35);
    EXPECT_EQ(log[1].reason().value(), "Missed page 2");
    EXPECT_EQ(log[1].changedBy(), lecturer().id());
    EXPECT_EQ(log[1].changedAt(), kNow + minutes(5));
    EXPECT_EQ(marks.find(sub)->policyCode(), "TEN_PCT_PER_DAY_V1");
}

// IT-43: late penalty is applied using the latest version's time (FR-15, UC-08 A2).
TEST_F(MarkingServiceTest, PenaltyFromLatestVersion) {
    const int late2 = submit().submit(s2(), a, goodFile, kNow + hours(30));        // 2 days
    EXPECT_DOUBLE_EQ(marking.mark(lecturer(), late2, 38, "Ok", std::nullopt, kNow + hours(40)), 28.0);
    const int b = assignments.createAssignment(lecturer(), course, "B", kNow, 20);
    const int veryLate = submit().submit(s2(), b, goodFile, kNow + hours(80));     // 4 days
    EXPECT_DOUBLE_EQ(marking.mark(lecturer(), veryLate, 20, "Good but late", std::nullopt, kNow), 0.0);
}

// IT-44: only the course lecturer can mark; unknown submission refused.
TEST_F(MarkingServiceTest, OnlyCourseLecturer) {
    EXPECT_THROW(marking.mark(otherHod(), sub, 30, "Ok", std::nullopt, kNow), AccessDeniedException);
    EXPECT_THROW(marking.mark(lecturer(), 999, 30, "Ok", std::nullopt, kNow), ValidationException);
    EXPECT_THROW(marking.listToMark(otherHod(), a), AccessDeniedException);
    EXPECT_THROW(marking.listToMark(lecturer(), 999), ValidationException);
}

// IT-45: moderated work cannot be re-marked; returned work can (with a reason).
TEST_F(MarkingServiceTest, ModeratedAndReturned) {
    marking.mark(lecturer(), sub, 30, "Ok", std::nullopt, kNow);
    moderation.returnForRemarking(hod(), sub, "Check Q2", kNow);
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Returned);
    marking.mark(lecturer(), sub, 33, "Ok", std::string("Q2 fixed"), kNow);
    EXPECT_EQ(submissions.findById(sub)->status(), SubmissionStatus::Marked);
    EXPECT_EQ(marks.find(sub)->hodComment().value(), "Check Q2");  // kept for the record
    moderation.approve(hod(), sub, kNow);
    EXPECT_THROW(marking.mark(lecturer(), sub, 40, "Ok", std::string("Late change"), kNow),
                 InvalidTransitionException);
}

// IT-46: list to mark shows Submitted, Late, Marked and Returned work with days late.
TEST_F(MarkingServiceTest, ListToMark) {
    submit().submit(s2(), a, goodFile, kNow + hours(25));  // 2 days late
    const auto rows = marking.listToMark(lecturer(), a);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].daysLate, 0);
    EXPECT_EQ(rows[1].daysLate, 2);
    EXPECT_FALSE(rows[1].mark.has_value());
    marking.mark(lecturer(), sub, 30, "Ok", std::nullopt, kNow);
    moderation.approve(hod(), sub, kNow);
    EXPECT_EQ(marking.listToMark(lecturer(), a).size(), 1u);  // moderated work drops off
}

}  // namespace lms::test
