// Unit tests for TenPercentPerDayPolicy (BR-01, BR-02, FR-12, FR-15).
// Test design: equivalence partitioning of "days late" (on time | 1-3 days | more than 3 days)
// and boundary values around each 24-hour step (Test Plan, section 4).
#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include "domain/TenPercentPerDayPolicy.h"
#include "domain/Time.h"

using namespace lms;
using std::chrono::hours;
using std::chrono::minutes;
using std::chrono::seconds;

namespace {
const TimePoint kDeadline = fromUnix(1800000000);

struct LateCase {
    std::chrono::seconds offset;
    int expectedDays;
};

// Readable names in test output, e.g. "plus86401s_is_2_days".
std::ostream& operator<<(std::ostream& out, const LateCase& c) {
    return out << c.offset.count() << "s->" << c.expectedDays;
}
std::string caseName(const ::testing::TestParamInfo<LateCase>& info) {
    const long long s = info.param.offset.count();
    const std::string sign = s < 0 ? "minus" : "plus";
    return sign + std::to_string(s < 0 ? -s : s) + "s_is_" + std::to_string(info.param.expectedDays) + "_days";
}

class DaysLateBoundary : public ::testing::TestWithParam<LateCase> {};
}  // namespace

// UT-01: boundary values for BR-02.
TEST_P(DaysLateBoundary, CountsEachStarted24Hours) {
    TenPercentPerDayPolicy p;
    EXPECT_EQ(p.daysLate(kDeadline + GetParam().offset, kDeadline), GetParam().expectedDays);
}

INSTANTIATE_TEST_SUITE_P(
    PenaltyPolicy, DaysLateBoundary,
    ::testing::Values(LateCase{-hours(48), 0},                // well before (EP: on time)
                      LateCase{-seconds(1), 0},               // just before
                      LateCase{seconds(0), 0},                // exactly at the deadline
                      LateCase{seconds(1), 1},                // just after
                      LateCase{minutes(1), 1},                // SRS example: 1 minute late
                      LateCase{hours(24), 1},                 // end of day 1
                      LateCase{hours(24) + seconds(1), 2},    // start of day 2
                      LateCase{hours(48), 2},
                      LateCase{hours(48) + seconds(1), 3},
                      LateCase{hours(72), 3},                 // last moment of day 3
                      LateCase{hours(72) + seconds(1), 4},    // EP: more than 3 days
                      LateCase{hours(24 * 30), 30}),
    caseName);

// UT-02: the worked example in BR-01.
TEST(PenaltyPolicy, SrsWorkedExample) {
    TenPercentPerDayPolicy p;
    const int days = p.daysLate(kDeadline + hours(30), kDeadline);
    EXPECT_EQ(days, 2);
    EXPECT_DOUBLE_EQ(p.finalMark(38, 50, days), 28.0);
}

// UT-03: one value from each equivalence class of days late.
TEST(PenaltyPolicy, FinalMarkForEachClass) {
    TenPercentPerDayPolicy p;
    EXPECT_DOUBLE_EQ(p.finalMark(40, 50, 0), 40.0);  // on time: no penalty
    EXPECT_DOUBLE_EQ(p.finalMark(40, 50, 1), 35.0);  // 1 day: -5
    EXPECT_DOUBLE_EQ(p.finalMark(40, 50, 3), 25.0);  // 3 days: -15
    EXPECT_DOUBLE_EQ(p.finalMark(40, 50, 4), 0.0);   // more than 3 days: 0
    EXPECT_DOUBLE_EQ(p.finalMark(15, 15, 1), 13.5);  // fractions are kept
}

// UT-04: the final mark is never below 0 (BR-01).
TEST(PenaltyPolicy, NeverBelowZero) {
    TenPercentPerDayPolicy p;
    EXPECT_DOUBLE_EQ(p.finalMark(3, 50, 3), 0.0);
    EXPECT_DOUBLE_EQ(p.finalMark(0, 50, 1), 0.0);
    EXPECT_DOUBLE_EQ(p.finalMark(0, 50, 0), 0.0);
}

// UT-05: boundaries of the maximum mark (1 and 100).
TEST(PenaltyPolicy, MaximumMarkBoundaries) {
    TenPercentPerDayPolicy p;
    EXPECT_DOUBLE_EQ(p.finalMark(1, 1, 1), 0.9);
    EXPECT_DOUBLE_EQ(p.finalMark(100, 100, 2), 80.0);
    EXPECT_EQ(p.code(), "TEN_PCT_PER_DAY_V1");
}
