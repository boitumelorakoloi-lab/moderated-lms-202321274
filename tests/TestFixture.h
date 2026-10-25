// TestFixture.h - shared set-up for the service and integration tests.
// Every test gets a fresh in-memory database with: an administrator, two HoDs (Sciences and
// Languages), a lecturer who teaches MATH, and two students enrolled in MATH.
// Times are fixed (kNow) so deadline rules can be tested exactly (SDD 2.1, risk R5).
#pragma once

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "data/Database.h"
#include "data/SqliteCourseRepository.h"
#include "data/SqliteMarkRepository.h"
#include "data/SqliteSubmissionRepository.h"
#include "data/SqliteUserRepository.h"
#include "domain/Exceptions.h"
#include "domain/TenPercentPerDayPolicy.h"
#include "domain/Time.h"
#include "domain/UserFactory.h"
#include "services/AdminService.h"
#include "services/AssignmentService.h"
#include "services/AuthService.h"
#include "services/MarkingService.h"
#include "services/ModerationService.h"
#include "services/SubmissionService.h"

namespace lms::test {

namespace fs = std::filesystem;
using std::chrono::hours;
using std::chrono::minutes;
using std::chrono::seconds;

inline const TimePoint kNow = fromUnix(1800000000);  // a fixed "current time"
inline const char* const kPassword = "Password1";

// Argon2id is slow on purpose, so the hash is made once and reused for test users.
inline const std::string& testHash() {
    static const std::string hash = AuthService::hashPassword(kPassword);
    return hash;
}

class LmsTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    SqliteUserRepository users{db};
    SqliteCourseRepository courses{db};
    SqliteSubmissionRepository submissions{db};
    SqliteMarkRepository marks{db};
    TenPercentPerDayPolicy policy;
    fs::path folder;

    AuthService auth{users};
    AdminService admin{users, courses, db};
    AssignmentService assignments{courses, submissions};
    std::unique_ptr<SubmissionService> submitPtr;
    MarkingService marking{courses, submissions, marks, policy};
    ModerationService moderation{courses, submissions, marks, policy};

    std::unique_ptr<User> adminUser, hodUser, otherHodUser, lectUser, stu1, stu2;
    int sciences = 0, languages = 0, course = 0, otherCourse = 0;
    std::string goodFile;

    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        folder = fs::temp_directory_path() /
                 (std::string("lms_") + info->test_suite_name() + "_" + info->name());
        fs::remove_all(folder);
        fs::create_directories(folder);
        submitPtr = std::make_unique<SubmissionService>(courses, submissions, marks, policy,
                                                        (folder / "store").string());
        goodFile = makeFile("work.txt", 100);

        const int a = addUser(Role::Administrator, "admin", "Admin");
        const int hod = addUser(Role::HeadOfDepartment, "hod", "Hod One");
        const int hod2 = addUser(Role::HeadOfDepartment, "hod2", "Hod Two");
        const int lect = addUser(Role::Lecturer, "lect", "Lect One");
        const int s1 = addUser(Role::Student, "stu1", "Student One");
        const int s2 = addUser(Role::Student, "stu2", "Student Two");
        adminUser = users.findById(a);
        sciences = admin.createDepartment(ad(), "Sciences", hod);
        languages = admin.createDepartment(ad(), "Languages", hod2);
        course = admin.createCourse(ad(), "MATH", "Maths", sciences);
        otherCourse = admin.createCourse(ad(), "ENG", "English", languages);
        admin.assignLecturer(ad(), lect, course);
        admin.enrolStudent(ad(), s1, course);
        admin.enrolStudent(ad(), s2, course);
        hodUser = users.findById(hod);  // loaded after the department so departmentId is set
        otherHodUser = users.findById(hod2);
        lectUser = users.findById(lect);
        stu1 = users.findById(s1);
        stu2 = users.findById(s2);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(folder, ec);
    }

    int addUser(Role role, const std::string& username, const std::string& name) {
        return users.add(*UserFactory::create(role, 0, username, name, testHash()));
    }

    std::string makeFile(const std::string& name, std::uintmax_t bytes) {
        const fs::path p = folder / name;
        std::ofstream out(p, std::ios::binary);
        out << std::string(static_cast<std::size_t>(bytes), 'x');
        return p.string();
    }

    SubmissionService& submit() { return *submitPtr; }
    Administrator& ad() { return static_cast<Administrator&>(*adminUser); }
    Lecturer& lecturer() { return static_cast<Lecturer&>(*lectUser); }
    HeadOfDepartment& hod() { return static_cast<HeadOfDepartment&>(*hodUser); }
    HeadOfDepartment& otherHod() { return static_cast<HeadOfDepartment&>(*otherHodUser); }
    Student& s1() { return static_cast<Student&>(*stu1); }
    Student& s2() { return static_cast<Student&>(*stu2); }

    // An assignment in MATH with its deadline at kNow.
    int makeAssignment(int maxMark = 50) {
        return assignments.createAssignment(lecturer(), course, "Test", kNow, maxMark);
    }
    // A marked submission for stu1 (on time).
    int markedSubmission(int assignmentId, int raw = 40) {
        const int sub = submit().submit(s1(), assignmentId, goodFile, kNow - hours(1));
        marking.mark(lecturer(), sub, raw, "Good work", std::nullopt, kNow);
        return sub;
    }
};

}  // namespace lms::test
