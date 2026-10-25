// Integration tests for AdminService (UC-03 to UC-05, FR-04 to FR-06, FR-25).
#include "TestFixture.h"

namespace lms::test {

class AdminServiceTest : public LmsTest {};

// IT-10: username length boundaries 2 / 3 / 30 / 31 and allowed characters.
TEST_F(AdminServiceTest, UsernameBoundaries) {
    EXPECT_THROW(admin.createUser(ad(), Role::Student, "ab", "A B", kPassword), ValidationException);
    EXPECT_NO_THROW(admin.createUser(ad(), Role::Student, "abc", "A B", kPassword));
    EXPECT_NO_THROW(admin.createUser(ad(), Role::Student, std::string(30, 'u'), "U", kPassword));
    EXPECT_THROW(admin.createUser(ad(), Role::Student, std::string(31, 'v'), "V", kPassword),
                 ValidationException);
    EXPECT_THROW(admin.createUser(ad(), Role::Student, "bad name", "X", kPassword), ValidationException);
    EXPECT_THROW(admin.createUser(ad(), Role::Student, "x';--", "X", kPassword), ValidationException);
    EXPECT_NO_THROW(admin.createUser(ad(), Role::Student, "first.last-2_b", "X", kPassword));
}

// IT-11: other create-user rules - blank name, duplicate (any case), short password.
TEST_F(AdminServiceTest, CreateUserRules) {
    EXPECT_THROW(admin.createUser(ad(), Role::Student, "newone", "  ", kPassword), ValidationException);
    EXPECT_THROW(admin.createUser(ad(), Role::Student, "STU1", "Dup", kPassword), ValidationException);
    EXPECT_THROW(admin.createUser(ad(), Role::Student, "newone", "New", "1234567"), ValidationException);
    const int id = admin.createUser(ad(), Role::Lecturer, "newone", " New Person ", kPassword);
    auto u = users.findById(id);
    EXPECT_EQ(u->role(), Role::Lecturer);
    EXPECT_EQ(u->fullName(), "New Person");
}

// IT-12: edit, reset password (also unlocks) and deactivate; cannot deactivate yourself.
TEST_F(AdminServiceTest, EditResetDeactivate) {
    admin.editUser(ad(), s1().id(), "Renamed Student");
    EXPECT_EQ(users.findById(s1().id())->fullName(), "Renamed Student");
    EXPECT_THROW(admin.editUser(ad(), s1().id(), ""), ValidationException);
    EXPECT_THROW(admin.editUser(ad(), 999, "X"), ValidationException);

    for (int i = 0; i < 5; ++i) {
        try { auth.login("stu1", "wrong", kNow); } catch (const LmsException&) {}
    }
    admin.resetPassword(ad(), s1().id(), "Brand-new1");
    EXPECT_NO_THROW(auth.login("stu1", "Brand-new1", kNow));  // lock cleared
    EXPECT_THROW(admin.resetPassword(ad(), s1().id(), "short"), ValidationException);

    EXPECT_THROW(admin.deactivateUser(ad(), ad().id()), ValidationException);
    admin.deactivateUser(ad(), s2().id());
    EXPECT_FALSE(users.findById(s2().id())->isActive());
    EXPECT_EQ(admin.listUsers(ad()).size(), 6u);  // records are kept
}

// IT-13: departments - blank/duplicate name, HoD must have HOD role, one department per HoD.
TEST_F(AdminServiceTest, DepartmentRules) {
    EXPECT_THROW(admin.createDepartment(ad(), " ", std::nullopt), ValidationException);
    EXPECT_THROW(admin.createDepartment(ad(), "Sciences", std::nullopt), ValidationException);
    EXPECT_THROW(admin.createDepartment(ad(), "Arts", lecturer().id()), ValidationException);
    EXPECT_THROW(admin.createDepartment(ad(), "Arts", hod().id()), ValidationException);  // already HoD
    const int arts = admin.createDepartment(ad(), "Arts", std::nullopt);
    const int newHod = addUser(Role::HeadOfDepartment, "hod3", "Hod Three");
    EXPECT_THROW(admin.setDepartmentHod(ad(), 999, newHod), ValidationException);
    EXPECT_THROW(admin.setDepartmentHod(ad(), arts, s1().id()), ValidationException);
    admin.setDepartmentHod(ad(), arts, newHod);
    EXPECT_EQ(admin.listDepartments(ad()).size(), 3u);
    auto loaded = users.findById(newHod);
    EXPECT_EQ(static_cast<HeadOfDepartment&>(*loaded).departmentId(), arts);
}

// IT-14: courses - blank fields, unknown department, duplicate code.
TEST_F(AdminServiceTest, CourseRules) {
    EXPECT_THROW(admin.createCourse(ad(), "", "Title", sciences), ValidationException);
    EXPECT_THROW(admin.createCourse(ad(), "BIO", " ", sciences), ValidationException);
    EXPECT_THROW(admin.createCourse(ad(), "BIO", "Biology", 999), ValidationException);
    EXPECT_THROW(admin.createCourse(ad(), "MATH", "Again", sciences), ValidationException);
    admin.createCourse(ad(), "BIO", "Biology", sciences);
    EXPECT_EQ(admin.listCourses(ad()).size(), 3u);
}

// IT-15: assign lecturers and enrol students - role checks and duplicates (FR-06).
TEST_F(AdminServiceTest, AssignAndEnrolRules) {
    EXPECT_THROW(admin.assignLecturer(ad(), s1().id(), course), ValidationException);   // not a lecturer
    EXPECT_THROW(admin.assignLecturer(ad(), lecturer().id(), 999), ValidationException);
    EXPECT_THROW(admin.assignLecturer(ad(), lecturer().id(), course), ValidationException);  // already
    admin.assignLecturer(ad(), hod().id(), course);  // an HoD can teach (is a lecturer)
    EXPECT_TRUE(courses.teaches(hod().id(), course));

    EXPECT_THROW(admin.enrolStudent(ad(), lecturer().id(), course), ValidationException);  // not a student
    EXPECT_THROW(admin.enrolStudent(ad(), s1().id(), 999), ValidationException);
    EXPECT_THROW(admin.enrolStudent(ad(), s1().id(), course), ValidationException);       // already
    admin.enrolStudent(ad(), s1().id(), otherCourse);
    EXPECT_TRUE(courses.isEnrolled(s1().id(), otherCourse));
}

// IT-16: backup and restore round trip; bad paths are refused (FR-25).
TEST_F(AdminServiceTest, BackupAndRestore) {
    const std::string backupFile = (folder / "backup.db").string();
    EXPECT_THROW(admin.backup(ad(), "  "), ValidationException);
    admin.backup(ad(), backupFile);
    admin.createCourse(ad(), "BIO", "Biology", sciences);   // change after the backup
    EXPECT_EQ(admin.listCourses(ad()).size(), 3u);
    admin.restore(ad(), backupFile);
    EXPECT_EQ(admin.listCourses(ad()).size(), 2u);           // back to the backup

    EXPECT_THROW(admin.restore(ad(), ""), ValidationException);
    EXPECT_THROW(admin.restore(ad(), (folder / "missing.db").string()), DatabaseException);
    EXPECT_THROW(admin.restore(ad(), goodFile), DatabaseException);  // not an LMS database
    EXPECT_EQ(admin.listCourses(ad()).size(), 2u);                   // nothing was damaged
}

}  // namespace lms::test
