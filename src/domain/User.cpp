// User.cpp - login-state rules and the permission table for each role (FR-02, FR-03).
#include "domain/User.h"

#include <utility>

namespace lms {

User::User(int id, std::string username, std::string fullName, std::string passwordHash)
    : id_(id),
      username_(std::move(username)),
      fullName_(std::move(fullName)),
      passwordHash_(std::move(passwordHash)) {}

bool User::isLocked(TimePoint now) const {
    return lockedUntil_.has_value() && now < *lockedUntil_;
}

void User::recordFailedLogin(TimePoint now) {
    ++failedAttempts_;
    if (failedAttempts_ >= kMaxFailedAttempts) {
        lockedUntil_ = now + kLockDuration;
        failedAttempts_ = 0;  // start counting again after the lock ends
    }
}

void User::setLoginState(int failedAttempts, std::optional<TimePoint> lockedUntil) {
    failedAttempts_ = failedAttempts;
    lockedUntil_ = lockedUntil;
}

bool Student::can(Permission p) const {
    return p == Permission::SubmitWork || p == Permission::ViewOwnMarks;
}

bool Lecturer::can(Permission p) const {
    return p == Permission::CreateAssignment || p == Permission::GrantExtension ||
           p == Permission::MarkSubmission || p == Permission::ExportMarks;
}

HeadOfDepartment::HeadOfDepartment(int id, std::string username, std::string fullName,
                                   std::string passwordHash, int departmentId)
    : Lecturer(id, std::move(username), std::move(fullName), std::move(passwordHash)),
      departmentId_(departmentId) {}

bool HeadOfDepartment::can(Permission p) const {
    // An HoD keeps every lecturer permission and adds moderation (Liskov, SDD 3.3).
    if (p == Permission::ModerateMarks || p == Permission::ReleaseMarks ||
        p == Permission::ViewAuditLog) {
        return true;
    }
    return Lecturer::can(p);
}

bool Administrator::can(Permission p) const {
    return p == Permission::ManageUsers || p == Permission::ManageCourses ||
           p == Permission::BackupRestore || p == Permission::ViewAuditLog;
}

}  // namespace lms
