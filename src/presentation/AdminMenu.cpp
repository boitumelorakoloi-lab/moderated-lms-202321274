// AdminMenu.cpp
#include "presentation/AdminMenu.h"

#include <iostream>
#include <map>

#include "domain/Time.h"
#include "presentation/ConsoleIO.h"

namespace lms {

AdminMenu::AdminMenu(Administrator& administrator, AuthService& auth, AdminService& adminService,
                     ModerationService& moderation)
    : RoleMenu(administrator, auth),
      admin_(administrator),
      admin(adminService),
      moderation_(moderation) {}

void AdminMenu::showOptions() const {
    io::heading("Administrator menu - " + user.fullName());
    std::cout << "  Users:        1. List users     2. Create user     3. Edit name\n"
              << "                4. Reset password 5. Deactivate user\n"
              << "  Courses:      6. List departments and courses\n"
              << "                7. Create department  8. Set department HoD\n"
              << "                9. Create course     10. Assign lecturer  11. Enrol student\n"
              << "  Audit/data:  12. View audit log    13. Back up data     14. Restore data\n";
}

bool AdminMenu::handle(int choice) {
    switch (choice) {
        case 1: listUsers(); return true;
        case 2: createUser(); return true;
        case 3: editUser(); return true;
        case 4: resetPassword(); return true;
        case 5: deactivateUser(); return true;
        case 6: listDepartmentsAndCourses(); return true;
        case 7: createDepartment(); return true;
        case 8: setDepartmentHod(); return true;
        case 9: createCourse(); return true;
        case 10: assignLecturer(); return true;
        case 11: enrolStudent(); return true;
        case 12: viewAuditLog(); return true;
        case 13: backup(); return true;
        case 14: restore(); return true;
        default: return false;
    }
}

void AdminMenu::listUsers() {
    io::heading("Users");
    std::cout << "  " << io::col("ID", 5) << io::col("Username", 18) << io::col("Name", 24)
              << io::col("Role", 20) << "Status\n";
    for (const auto& u : admin.listUsers(admin_)) {
        std::string status = u->isActive() ? "Active" : "Deactivated";
        if (u->lockedUntil()) status += " (locked until " + formatTime(*u->lockedUntil()) + ")";
        std::cout << "  " << io::col(std::to_string(u->id()), 5) << io::col(u->username(), 18)
                  << io::col(u->fullName(), 24) << io::col(roleName(u->role()), 20) << status << '\n';
    }
}

void AdminMenu::createUser() {
    io::heading("Create user");
    std::cout << "  1. Student  2. Lecturer  3. Head of Department  4. Administrator\n";
    const int r = io::readInt("Role: ", 1, 4);
    const Role role = r == 1   ? Role::Student
                      : r == 2 ? Role::Lecturer
                      : r == 3 ? Role::HeadOfDepartment
                               : Role::Administrator;
    const std::string username = io::readNonEmpty("Username: ");
    const std::string fullName = io::readNonEmpty("Full name: ");
    const std::string password = io::readPassword("First password (at least 8 characters): ");
    const int id = admin.createUser(admin_, role, username, fullName, password);
    io::info(roleName(role) + " created with ID " + std::to_string(id) + ".");
}

void AdminMenu::editUser() {
    io::heading("Edit user's name");
    const int id = io::readInt("User ID: ", 1, 1000000000);
    const std::string name = io::readNonEmpty("New full name: ");
    admin.editUser(admin_, id, name);
    io::info("Saved.");
}

void AdminMenu::resetPassword() {
    io::heading("Reset a password");
    const int id = io::readInt("User ID: ", 1, 1000000000);
    const std::string pw = io::readPassword("New password (at least 8 characters): ");
    admin.resetPassword(admin_, id, pw);
    io::info("Password reset. Any lock on the account has been removed.");
}

void AdminMenu::deactivateUser() {
    io::heading("Deactivate user");
    const int id = io::readInt("User ID: ", 1, 1000000000);
    admin.deactivateUser(admin_, id);
    io::info("The account is deactivated and cannot log in. Its records are kept.");
}

void AdminMenu::listDepartmentsAndCourses() {
    io::heading("Departments");
    std::map<int, std::string> hodNames;
    for (const auto& u : admin.listUsers(admin_)) {
        hodNames[u->id()] = u->fullName();
    }
    for (const Department& d : admin.listDepartments(admin_)) {
        std::cout << "  " << io::col(std::to_string(d.id()), 5) << io::col(d.name(), 22) << "HoD: "
                  << (d.hodUserId() ? hodNames[*d.hodUserId()] : "(none)") << '\n';
    }
    io::heading("Courses");
    for (const Course& c : admin.listCourses(admin_)) {
        std::cout << "  " << io::col(std::to_string(c.id()), 5) << io::col(c.code(), 12)
                  << io::col(c.title(), 28) << "Dept " << c.departmentId() << '\n';
    }
}

void AdminMenu::createDepartment() {
    io::heading("Create department");
    const std::string name = io::readNonEmpty("Department name: ");
    const auto hod = io::readOptionalInt("HoD user ID (blank for none yet): ", 1, 1000000000);
    const int id = admin.createDepartment(admin_, name, hod);
    io::info("Department created with ID " + std::to_string(id) + ".");
}

void AdminMenu::setDepartmentHod() {
    io::heading("Set department HoD");
    const int dept = io::readInt("Department ID: ", 1, 1000000000);
    const int hod = io::readInt("HoD user ID: ", 1, 1000000000);
    admin.setDepartmentHod(admin_, dept, hod);
    io::info("Saved.");
}

void AdminMenu::createCourse() {
    io::heading("Create course");
    const std::string code = io::readNonEmpty("Course code (e.g. MATH-F4): ");
    const std::string title = io::readNonEmpty("Title: ");
    const int dept = io::readInt("Department ID: ", 1, 1000000000);
    const int id = admin.createCourse(admin_, code, title, dept);
    io::info("Course created with ID " + std::to_string(id) + ".");
}

void AdminMenu::assignLecturer() {
    io::heading("Assign lecturer to course");
    const int lecturer = io::readInt("Lecturer user ID: ", 1, 1000000000);
    const int course = io::readInt("Course ID: ", 1, 1000000000);
    admin.assignLecturer(admin_, lecturer, course);
    io::info("Saved.");
}

void AdminMenu::enrolStudent() {
    io::heading("Enrol student in course");
    const int student = io::readInt("Student user ID: ", 1, 1000000000);
    const int course = io::readInt("Course ID: ", 1, 1000000000);
    admin.enrolStudent(admin_, student, course);
    io::info("Saved.");
}

void AdminMenu::viewAuditLog() {
    io::heading("Audit log");
    const int sub = io::readInt("Submission ID: ", 1, 1000000000);
    const auto entries = moderation_.viewAuditLog(admin_, sub);
    if (entries.empty()) {
        io::info("No entries.");
        return;
    }
    std::cout << "  " << io::col("When", 18) << io::col("Sub", 5) << io::col("Who", 10)
              << io::col("Action", 12) << io::col("Old", 5) << io::col("New", 5) << "Reason\n";
    for (const AuditEntry& e : entries) {
        std::cout << "  " << io::col(formatTime(e.changedAt()), 18)
                  << io::col(std::to_string(e.submissionId()), 5) << io::col("User " + std::to_string(e.changedBy()), 10)
                  << io::col(e.action(), 12)
                  << io::col(e.oldMark() ? std::to_string(*e.oldMark()) : "-", 5)
                  << io::col(e.newMark() ? std::to_string(*e.newMark()) : "-", 5)
                  << e.reason().value_or("") << '\n';
    }
}

void AdminMenu::backup() {
    io::heading("Back up data");
    const std::string path = io::readNonEmpty("Backup file name (e.g. backup-2026-10-30.db): ");
    admin.backup(admin_, path);
    io::info("Backup saved to " + path + ". Copy it to a USB stick or another PC.");
}

void AdminMenu::restore() {
    io::heading("Restore data");
    io::info("WARNING: this replaces ALL current data with the backup.");
    const std::string path = io::readNonEmpty("Backup file to restore: ");
    if (!io::readYesNo("Are you sure?")) {
        io::info("Nothing was changed.");
        return;
    }
    admin.restore(admin_, path);
    io::info("Data restored. Everyone should log out and log in again.");
}

}  // namespace lms
