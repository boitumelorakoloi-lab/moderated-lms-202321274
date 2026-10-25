// AdminService.h - accounts, departments, courses, enrolment and backup (UC-03 to UC-05, FR-25).
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "data/Repositories.h"
#include "domain/User.h"

namespace lms {

class AdminService {
public:
    AdminService(IUserRepository& users, ICourseRepository& courses, IBackupStore& backupStore);

    // FR-04
    int createUser(const Administrator& a, Role role, const std::string& username,
                   const std::string& fullName, const std::string& password);
    void editUser(const Administrator& a, int userId, const std::string& fullName);
    void resetPassword(const Administrator& a, int userId, const std::string& newPassword);
    void deactivateUser(const Administrator& a, int userId);
    std::vector<std::unique_ptr<User>> listUsers(const Administrator& a);

    // FR-05
    int createDepartment(const Administrator& a, const std::string& name,
                         std::optional<int> hodUserId);
    void setDepartmentHod(const Administrator& a, int departmentId, int hodUserId);
    std::vector<Department> listDepartments(const Administrator& a);
    int createCourse(const Administrator& a, const std::string& code, const std::string& title,
                     int departmentId);
    std::vector<Course> listCourses(const Administrator& a);

    // FR-06
    void assignLecturer(const Administrator& a, int lecturerId, int courseId);
    void enrolStudent(const Administrator& a, int studentId, int courseId);

    // FR-25
    void backup(const Administrator& a, const std::string& path);
    void restore(const Administrator& a, const std::string& path);

private:
    void require(const Administrator& a, Permission p) const;
    std::unique_ptr<User> getUser(int userId);
    void checkHodCandidate(int hodUserId);

    IUserRepository& users_;
    ICourseRepository& courses_;
    IBackupStore& backupStore_;
};

}  // namespace lms
