// Unit tests for the domain layer: Submission life cycle (SDD Figure 3), User roles and
// lock-out (FR-02, FR-03), Assignment deadlines, and the type/time helpers.
#include <gtest/gtest.h>

#include "domain/Assignment.h"
#include "domain/Exceptions.h"
#include "domain/Submission.h"
#include "domain/Time.h"
#include "domain/UserFactory.h"

using namespace lms;
using S = SubmissionStatus;

// UT-10: every arrow on the state machine is allowed.
TEST(SubmissionState, AllowedArrows) {
    EXPECT_TRUE(Submission::canTransition(S::Submitted, S::Submitted));  // resubmit
    EXPECT_TRUE(Submission::canTransition(S::Submitted, S::Marked));
    EXPECT_TRUE(Submission::canTransition(S::Late, S::Submitted));       // extension
    EXPECT_TRUE(Submission::canTransition(S::Late, S::Marked));
    EXPECT_TRUE(Submission::canTransition(S::Marked, S::Marked));        // change mark
    EXPECT_TRUE(Submission::canTransition(S::Marked, S::Returned));
    EXPECT_TRUE(Submission::canTransition(S::Marked, S::Moderated));
    EXPECT_TRUE(Submission::canTransition(S::Returned, S::Marked));
    EXPECT_TRUE(Submission::canTransition(S::Moderated, S::Released));
}

// UT-11: moves that are not on the diagram are refused.
TEST(SubmissionState, ForbiddenArrows) {
    EXPECT_FALSE(Submission::canTransition(S::Late, S::Late));           // BR-04
    EXPECT_FALSE(Submission::canTransition(S::Submitted, S::Late));
    EXPECT_FALSE(Submission::canTransition(S::Submitted, S::Released));
    EXPECT_FALSE(Submission::canTransition(S::Marked, S::Released));     // FR-19
    EXPECT_FALSE(Submission::canTransition(S::Returned, S::Moderated));
    EXPECT_FALSE(Submission::canTransition(S::Moderated, S::Marked));
    for (S to : {S::Submitted, S::Late, S::Marked, S::Returned, S::Moderated, S::Released}) {
        EXPECT_FALSE(Submission::canTransition(S::Released, to));         // final state
    }
}

// UT-12: transitionTo() throws and leaves the state unchanged.
TEST(SubmissionState, TransitionThrowsOnBadMove) {
    Submission s(1, 1, 1, S::Submitted);
    EXPECT_THROW(s.transitionTo(S::Released), InvalidTransitionException);
    EXPECT_EQ(s.status(), S::Submitted);
    s.transitionTo(S::Marked);
    EXPECT_EQ(s.status(), S::Marked);
}

// UT-13: the newest version is the one marked (FR-11).
TEST(SubmissionVersions, LatestIsHighestVersion) {
    Submission s(1, 1, 1, S::Submitted);
    EXPECT_THROW(s.latest(), ValidationException);  // no files yet
    s.addVersion(SubmissionVersion(1, "a", "a.pdf", 10, fromUnix(100)));
    s.addVersion(SubmissionVersion(2, "b", "b.pdf", 20, fromUnix(200)));
    EXPECT_EQ(s.latest().versionNo(), 2);
    EXPECT_EQ(s.latest().storedPath(), "b");
    EXPECT_EQ(s.versions().size(), 2u);
}

// UT-14: effective deadline uses the extension when there is one.
TEST(AssignmentDeadline, EffectiveDeadline) {
    const Assignment a(1, 1, "A", fromUnix(1000), 50, 2);
    EXPECT_EQ(a.effectiveDeadline(std::nullopt), fromUnix(1000));
    const Extension ext(1, 5, fromUnix(5000), "Sick", 2);
    EXPECT_EQ(a.effectiveDeadline(ext), fromUnix(5000));
}

// UT-15: the Factory builds the right subclass, and permissions follow the roles (FR-02).
TEST(Users, FactoryAndPermissions) {
    auto hod = UserFactory::create(Role::HeadOfDepartment, 1, "h", "H", "x", 7);
    ASSERT_NE(dynamic_cast<Lecturer*>(hod.get()), nullptr);  // an HoD is a Lecturer
    EXPECT_EQ(hod->role(), Role::HeadOfDepartment);
    EXPECT_TRUE(hod->can(Permission::MarkSubmission));      // kept from Lecturer
    EXPECT_TRUE(hod->can(Permission::ModerateMarks));
    EXPECT_TRUE(hod->can(Permission::ReleaseMarks));
    EXPECT_FALSE(hod->can(Permission::ManageUsers));
    EXPECT_EQ(static_cast<HeadOfDepartment&>(*hod).departmentId(), 7);

    auto lecturer = UserFactory::create(Role::Lecturer, 2, "l", "L", "x");
    EXPECT_TRUE(lecturer->can(Permission::GrantExtension));
    EXPECT_FALSE(lecturer->can(Permission::ModerateMarks));

    auto student = UserFactory::create(Role::Student, 3, "s", "S", "x");
    EXPECT_TRUE(student->can(Permission::SubmitWork));
    EXPECT_TRUE(student->can(Permission::ViewOwnMarks));
    EXPECT_FALSE(student->can(Permission::MarkSubmission));
    EXPECT_FALSE(student->can(Permission::ViewAuditLog));

    auto admin = UserFactory::create(Role::Administrator, 4, "a", "A", "x");
    EXPECT_TRUE(admin->can(Permission::ManageUsers));
    EXPECT_TRUE(admin->can(Permission::BackupRestore));
    EXPECT_TRUE(admin->can(Permission::ViewAuditLog));
    EXPECT_FALSE(admin->can(Permission::ModerateMarks));
    EXPECT_FALSE(admin->can(Permission::SubmitWork));
}

// UT-16: FR-03 boundary values - 4 wrong passwords do not lock, the 5th does, for 15 minutes.
TEST(Users, LockAfterFiveWrongPasswords) {
    auto u = UserFactory::create(Role::Student, 1, "s", "S", "x");
    const TimePoint now = fromUnix(1000000);
    for (int i = 0; i < 4; ++i) u->recordFailedLogin(now);
    EXPECT_FALSE(u->isLocked(now));
    EXPECT_EQ(u->failedAttempts(), 4);
    u->recordFailedLogin(now);
    EXPECT_TRUE(u->isLocked(now));
    EXPECT_TRUE(u->isLocked(now + std::chrono::minutes(15) - std::chrono::seconds(1)));
    EXPECT_FALSE(u->isLocked(now + std::chrono::minutes(15)));
}

// UT-17: text codes match the CHECK constraints in schema.sql and round-trip.
TEST(Types, CodesRoundTrip) {
    for (Role r : {Role::Student, Role::Lecturer, Role::HeadOfDepartment, Role::Administrator}) {
        EXPECT_EQ(roleFromCode(roleCode(r)), r);
        EXPECT_FALSE(roleName(r).empty());
    }
    for (S s : {S::Submitted, S::Late, S::Marked, S::Returned, S::Moderated, S::Released}) {
        EXPECT_EQ(statusFromCode(statusCode(s)), s);
        EXPECT_FALSE(statusName(s).empty());
    }
    EXPECT_EQ(roleCode(Role::HeadOfDepartment), "HOD");
    EXPECT_THROW(roleFromCode("GOD"), DatabaseException);
    EXPECT_THROW(statusFromCode("DONE"), DatabaseException);
}

// UT-18: date input - valid and invalid partitions (used by the menus, NFR-07).
TEST(Time, ParseDateTime) {
    const auto t = parseDateTime("2026-10-30 14:00");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(formatTime(*t), "2026-10-30 14:00");
    EXPECT_EQ(fromUnix(toUnix(*t)), *t);
    EXPECT_FALSE(parseDateTime("").has_value());
    EXPECT_FALSE(parseDateTime("tomorrow").has_value());
    EXPECT_FALSE(parseDateTime("2026-02-30 10:00").has_value());  // no 30 February
    EXPECT_FALSE(parseDateTime("2026-13-01 10:00").has_value());  // no month 13
    EXPECT_FALSE(parseDateTime("2026-10-30 24:00").has_value());
    EXPECT_FALSE(parseDateTime("2026-10-30 10:00x").has_value());  // extra characters
}
