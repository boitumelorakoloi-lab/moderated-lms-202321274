// Integration tests for AuthService with the SQLite user repository (UC-01, UC-02,
// FR-01, FR-03, FR-26, NFR-04, NFR-05).
#include "TestFixture.h"

namespace lms::test {

class AuthServiceTest : public LmsTest {};

// IT-01: correct login returns the right subclass (Factory) - UC-01 main flow.
TEST_F(AuthServiceTest, LoginReturnsRoleSubclass) {
    auto u = auth.login("hod", kPassword, kNow);
    ASSERT_NE(u, nullptr);
    EXPECT_EQ(u->role(), Role::HeadOfDepartment);
    EXPECT_NE(dynamic_cast<HeadOfDepartment*>(u.get()), nullptr);
    EXPECT_EQ(static_cast<HeadOfDepartment&>(*u).departmentId(), sciences);
}

// IT-02: usernames are not case-sensitive and spaces are trimmed.
TEST_F(AuthServiceTest, UsernameCaseAndSpaces) {
    EXPECT_NO_THROW(auth.login("STU1", kPassword, kNow));
    EXPECT_NO_THROW(auth.login("  stu1 ", kPassword, kNow));
}

// IT-03: wrong password and unknown user give the same message (no username guessing).
TEST_F(AuthServiceTest, WrongPasswordAndUnknownUserSameMessage) {
    std::string a, b;
    try { auth.login("stu1", "wrong", kNow); } catch (const ValidationException& e) { a = e.what(); }
    try { auth.login("nobody", "wrong", kNow); } catch (const ValidationException& e) { b = e.what(); }
    EXPECT_FALSE(a.empty());
    EXPECT_EQ(a, b);
}

// IT-04: SQL injection attempt in the username is treated as plain text (NFR-05).
TEST_F(AuthServiceTest, SqlInjectionIsJustText) {
    EXPECT_THROW(auth.login("' OR 1=1 --", "x", kNow), ValidationException);
    EXPECT_THROW(auth.login("stu1' --", "x", kNow), ValidationException);
    EXPECT_EQ(users.listAll().size(), 6u);  // nothing was changed
}

// IT-05: FR-03 lock-out after 5 wrong passwords, saved in the database, 15 minutes.
TEST_F(AuthServiceTest, LockoutIsSavedAndExpires) {
    for (int i = 0; i < 4; ++i) {
        EXPECT_THROW(auth.login("stu2", "wrong", kNow), ValidationException);
    }
    EXPECT_THROW(auth.login("stu2", "wrong", kNow), AccessDeniedException);  // 5th locks
    EXPECT_TRUE(users.findByUsername("stu2")->isLocked(kNow));               // stored
    EXPECT_THROW(auth.login("stu2", kPassword, kNow + minutes(14)), AccessDeniedException);
    EXPECT_NO_THROW(auth.login("stu2", kPassword, kNow + minutes(15)));
    EXPECT_EQ(users.findByUsername("stu2")->failedAttempts(), 0);            // reset
}

// IT-06: a good login resets the wrong-password count.
TEST_F(AuthServiceTest, GoodLoginResetsCount) {
    for (int i = 0; i < 4; ++i) {
        EXPECT_THROW(auth.login("stu1", "wrong", kNow), ValidationException);
    }
    auth.login("stu1", kPassword, kNow);
    EXPECT_THROW(auth.login("stu1", "wrong", kNow), ValidationException);  // count starts again
    EXPECT_FALSE(users.findByUsername("stu1")->isLocked(kNow));
}

// IT-07: a deactivated account cannot log in (FR-04).
TEST_F(AuthServiceTest, DeactivatedCannotLogIn) {
    admin.deactivateUser(ad(), s1().id());
    EXPECT_THROW(auth.login("stu1", kPassword, kNow), AccessDeniedException);
}

// IT-08: change password - wrong old password, too short (7/8 boundary), success (UC-02).
TEST_F(AuthServiceTest, ChangePassword) {
    auto u = auth.login("stu1", kPassword, kNow);
    EXPECT_THROW(auth.changePassword(*u, "wrong", "NewPass99"), ValidationException);
    EXPECT_THROW(auth.changePassword(*u, kPassword, "1234567"), ValidationException);  // 7 chars
    auth.changePassword(*u, kPassword, "12345678");                                    // 8 chars
    EXPECT_THROW(auth.login("stu1", kPassword, kNow), ValidationException);
    EXPECT_NO_THROW(auth.login("stu1", "12345678", kNow));
}

// IT-09: passwords are stored as salted Argon2id hashes, never plain text (NFR-04).
TEST_F(AuthServiceTest, PasswordsAreSaltedArgon2id) {
    const std::string h1 = AuthService::hashPassword("SamePassword");
    const std::string h2 = AuthService::hashPassword("SamePassword");
    EXPECT_EQ(h1.rfind("$argon2id$", 0), 0u);
    EXPECT_NE(h1, h2);  // a different random salt each time
    EXPECT_EQ(h1.find("SamePassword"), std::string::npos);
    EXPECT_THROW(AuthService::hashPassword("short"), ValidationException);
}

}  // namespace lms::test
