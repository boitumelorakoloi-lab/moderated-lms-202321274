// AdminMenu.h
#pragma once

#include "presentation/RoleMenu.h"
#include "services/AdminService.h"
#include "services/ModerationService.h"

namespace lms {

class AdminMenu final : public RoleMenu {
public:
    AdminMenu(Administrator& admin, AuthService& auth, AdminService& adminService,
              ModerationService& moderation);

protected:
    void showOptions() const override;
    bool handle(int choice) override;

private:
    void listUsers();
    void createUser();
    void editUser();
    void resetPassword();
    void deactivateUser();
    void listDepartmentsAndCourses();
    void createDepartment();
    void setDepartmentHod();
    void createCourse();
    void assignLecturer();
    void enrolStudent();
    void viewAuditLog();
    void backup();
    void restore();

    Administrator& admin_;
    AdminService& admin;
    ModerationService& moderation_;
};

}  // namespace lms
