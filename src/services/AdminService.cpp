// AdminService.cpp
#include "services/AdminService.h"

#include <algorithm>
#include <cctype>

#include "domain/Exceptions.h"
#include "domain/UserFactory.h"
#include "services/AuthService.h"
#include "services/MarkView.h"

namespace lms {

AdminService::AdminService(IUserRepository& users, ICourseRepository& courses,
                           IBackupStore& backupStore)
    : users_(users), courses_(courses), backupStore_(backupStore) {}

void AdminService::require(const Administrator& a, Permission p) const {
    if (!a.can(p)) {
        throw AccessDeniedException("You are not allowed to do this.");
    }
}

std::unique_ptr<User> AdminService::getUser(int userId) {
    auto user = users_.findById(userId);
    if (!user) {
        throw ValidationException("There is no user with ID " + std::to_string(userId) + ".");
    }
    return user;
}

int AdminService::createUser(const Administrator& a, Role role, const std::string& username,
                             const std::string& fullName, const std::string& password) {
    require(a, Permission::ManageUsers);
    const std::string name = trim(username);
    if (name.size() < 3 || name.size() > 30) {
        throw ValidationException("The username must be 3 to 30 characters long.");
    }
    const bool allowed = std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-';
    });
    if (!allowed) {
        throw ValidationException("The username may only use letters, digits, '.', '_' and '-'.");
    }
    if (isBlank(fullName)) {
        throw ValidationException("The full name cannot be blank.");
    }
    if (users_.findByUsername(name)) {
        throw ValidationException("The username '" + name + "' is already taken.");
    }
    auto user = UserFactory::create(role, 0, name, trim(fullName), AuthService::hashPassword(password));
    return users_.add(*user);
}

void AdminService::editUser(const Administrator& a, int userId, const std::string& fullName) {
    require(a, Permission::ManageUsers);
    if (isBlank(fullName)) {
        throw ValidationException("The full name cannot be blank.");
    }
    auto user = getUser(userId);
    user->setFullName(trim(fullName));
    users_.update(*user);
}

void AdminService::resetPassword(const Administrator& a, int userId, const std::string& newPassword) {
    require(a, Permission::ManageUsers);
    auto user = getUser(userId);
    user->setPasswordHash(AuthService::hashPassword(newPassword));
    user->setLoginState(0, std::nullopt);  // also clears any lock
    users_.update(*user);
}

void AdminService::deactivateUser(const Administrator& a, int userId) {
    require(a, Permission::ManageUsers);
    if (userId == a.id()) {
        throw ValidationException("You cannot deactivate your own account.");
    }
    auto user = getUser(userId);
    user->setActive(false);
    users_.update(*user);
}

std::vector<std::unique_ptr<User>> AdminService::listUsers(const Administrator& a) {
    require(a, Permission::ManageUsers);
    return users_.listAll();
}

void AdminService::checkHodCandidate(int hodUserId) {
    auto user = getUser(hodUserId);
    if (user->role() != Role::HeadOfDepartment) {
        throw ValidationException(user->fullName() + " does not have the Head of Department role.");
    }
    // One department per HoD (department.hod_user_id is UNIQUE).
    for (const Department& d : courses_.listDepartments()) {
        if (d.hodUserId() == hodUserId) {
            throw ValidationException(user->fullName() + " is already HoD of " + d.name() + ".");
        }
    }
}

int AdminService::createDepartment(const Administrator& a, const std::string& name,
                                   std::optional<int> hodUserId) {
    require(a, Permission::ManageCourses);
    if (isBlank(name)) {
        throw ValidationException("The department name cannot be blank.");
    }
    for (const Department& d : courses_.listDepartments()) {
        if (d.name() == trim(name)) {
            throw ValidationException("A department called '" + d.name() + "' already exists.");
        }
    }
    if (hodUserId.has_value()) {
        checkHodCandidate(*hodUserId);
    }
    return courses_.addDepartment(Department(0, trim(name), hodUserId));
}

void AdminService::setDepartmentHod(const Administrator& a, int departmentId, int hodUserId) {
    require(a, Permission::ManageCourses);
    if (!courses_.findDepartment(departmentId)) {
        throw ValidationException("There is no department with ID " + std::to_string(departmentId) + ".");
    }
    checkHodCandidate(hodUserId);
    courses_.setDepartmentHod(departmentId, hodUserId);
}

std::vector<Department> AdminService::listDepartments(const Administrator& a) {
    require(a, Permission::ManageCourses);
    return courses_.listDepartments();
}

int AdminService::createCourse(const Administrator& a, const std::string& code,
                               const std::string& title, int departmentId) {
    require(a, Permission::ManageCourses);
    if (isBlank(code) || isBlank(title)) {
        throw ValidationException("The course code and title cannot be blank.");
    }
    if (!courses_.findDepartment(departmentId)) {
        throw ValidationException("There is no department with ID " + std::to_string(departmentId) + ".");
    }
    for (const Course& c : courses_.listCourses()) {
        if (c.code() == trim(code)) {
            throw ValidationException("A course with code '" + c.code() + "' already exists.");
        }
    }
    return courses_.addCourse(Course(0, trim(code), trim(title), departmentId));
}

std::vector<Course> AdminService::listCourses(const Administrator& a) {
    require(a, Permission::ManageCourses);
    return courses_.listCourses();
}

void AdminService::assignLecturer(const Administrator& a, int lecturerId, int courseId) {
    require(a, Permission::ManageCourses);
    auto user = getUser(lecturerId);
    if (user->role() != Role::Lecturer && user->role() != Role::HeadOfDepartment) {
        throw ValidationException(user->fullName() + " is not a lecturer.");
    }
    if (!courses_.findCourse(courseId)) {
        throw ValidationException("There is no course with ID " + std::to_string(courseId) + ".");
    }
    if (courses_.teaches(lecturerId, courseId)) {
        throw ValidationException(user->fullName() + " already teaches this course.");
    }
    courses_.assignLecturer(lecturerId, courseId);
}

void AdminService::enrolStudent(const Administrator& a, int studentId, int courseId) {
    require(a, Permission::ManageCourses);
    auto user = getUser(studentId);
    if (user->role() != Role::Student) {
        throw ValidationException(user->fullName() + " is not a student.");
    }
    if (!courses_.findCourse(courseId)) {
        throw ValidationException("There is no course with ID " + std::to_string(courseId) + ".");
    }
    if (courses_.isEnrolled(studentId, courseId)) {
        throw ValidationException(user->fullName() + " is already enrolled in this course.");
    }
    courses_.enrol(studentId, courseId);
}

void AdminService::backup(const Administrator& a, const std::string& path) {
    require(a, Permission::BackupRestore);
    if (isBlank(path)) {
        throw ValidationException("Please give a file name for the backup.");
    }
    backupStore_.backupTo(trim(path));
}

void AdminService::restore(const Administrator& a, const std::string& path) {
    require(a, Permission::BackupRestore);
    if (isBlank(path)) {
        throw ValidationException("Please give the backup file to restore.");
    }
    backupStore_.restoreFrom(trim(path));
}

}  // namespace lms
