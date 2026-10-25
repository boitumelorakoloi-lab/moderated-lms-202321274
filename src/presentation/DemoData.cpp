// DemoData.cpp
#include "presentation/DemoData.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "domain/UserFactory.h"

namespace lms {

bool DemoData::seed(Services s, IUserRepository& users, const std::string& demoFolder) {
    if (!users.listAll().empty()) {
        return false;
    }
    using std::chrono::hours;
    const auto now = std::chrono::system_clock::now();

    // The first administrator is added directly (there is nobody yet to create it).
    auto first = UserFactory::create(Role::Administrator, 0, "admin", "System Administrator",
                                     AuthService::hashPassword(kPassword));
    const int adminId = users.add(*first);
    auto adminUser = users.findById(adminId);
    const auto& admin = static_cast<const Administrator&>(*adminUser);

    const int hod = s.admin.createUser(admin, Role::HeadOfDepartment, "hod.molapo", "Mrs P. Molapo", kPassword);
    const int lect = s.admin.createUser(admin, Role::Lecturer, "lect.mokoena", "Mr T. Mokoena", kPassword);
    const int st1 = s.admin.createUser(admin, Role::Student, "stu.thabo", "Thabo Nthako", kPassword);
    const int st2 = s.admin.createUser(admin, Role::Student, "stu.lerato", "Lerato Sekhonyana", kPassword);
    const int st3 = s.admin.createUser(admin, Role::Student, "stu.palesa", "Palesa Mohapi", kPassword);

    const int sciences = s.admin.createDepartment(admin, "Sciences", hod);
    s.admin.createDepartment(admin, "Languages", std::nullopt);
    const int maths = s.admin.createCourse(admin, "MATH-F4", "Mathematics Form 4", sciences);
    const int physics = s.admin.createCourse(admin, "PHYS-F4", "Physical Science Form 4", sciences);
    s.admin.assignLecturer(admin, lect, maths);
    s.admin.assignLecturer(admin, hod, physics);  // the HoD also teaches
    for (int student : {st1, st2, st3}) {
        s.admin.enrolStudent(admin, student, maths);
        s.admin.enrolStudent(admin, student, physics);
    }

    auto lecturerUser = users.findById(lect);
    const auto& lecturer = static_cast<const Lecturer&>(*lecturerUser);
    s.assignments.createAssignment(lecturer, maths, "Algebra test 1", now + hours(48), 50);
    s.assignments.createAssignment(lecturer, maths, "Trigonometry homework", now - hours(30), 20);

    // A small file students can use to try submitting.
    std::filesystem::create_directories(demoFolder);
    std::ofstream(std::filesystem::path(demoFolder) / "my_homework.txt")
        << "Sample homework answer used for the demonstration.\n";

    std::cout << "Demo data created. Every demo account uses the password " << kPassword << "\n"
              << "  admin, hod.molapo, lect.mokoena, stu.thabo, stu.lerato, stu.palesa\n"
              << "  Sample file to submit: " << demoFolder << "/my_homework.txt\n";
    return true;
}

}  // namespace lms
