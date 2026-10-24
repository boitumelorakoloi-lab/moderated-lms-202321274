// Types.cpp - conversions between enums and their database / display text.
#include "domain/Types.h"

#include "domain/Exceptions.h"

namespace lms {

std::string roleCode(Role role) {
    switch (role) {
        case Role::Student: return "STUDENT";
        case Role::Lecturer: return "LECTURER";
        case Role::HeadOfDepartment: return "HOD";
        case Role::Administrator: return "ADMIN";
    }
    return "STUDENT";
}

Role roleFromCode(const std::string& code) {
    if (code == "STUDENT") return Role::Student;
    if (code == "LECTURER") return Role::Lecturer;
    if (code == "HOD") return Role::HeadOfDepartment;
    if (code == "ADMIN") return Role::Administrator;
    throw DatabaseException("Unknown role code in database: " + code);
}

std::string roleName(Role role) {
    switch (role) {
        case Role::Student: return "Student";
        case Role::Lecturer: return "Lecturer";
        case Role::HeadOfDepartment: return "Head of Department";
        case Role::Administrator: return "Administrator";
    }
    return "";
}

std::string statusCode(SubmissionStatus status) {
    switch (status) {
        case SubmissionStatus::Submitted: return "SUBMITTED";
        case SubmissionStatus::Late: return "LATE";
        case SubmissionStatus::Marked: return "MARKED";
        case SubmissionStatus::Returned: return "RETURNED";
        case SubmissionStatus::Moderated: return "MODERATED";
        case SubmissionStatus::Released: return "RELEASED";
    }
    return "SUBMITTED";
}

SubmissionStatus statusFromCode(const std::string& code) {
    if (code == "SUBMITTED") return SubmissionStatus::Submitted;
    if (code == "LATE") return SubmissionStatus::Late;
    if (code == "MARKED") return SubmissionStatus::Marked;
    if (code == "RETURNED") return SubmissionStatus::Returned;
    if (code == "MODERATED") return SubmissionStatus::Moderated;
    if (code == "RELEASED") return SubmissionStatus::Released;
    throw DatabaseException("Unknown submission status in database: " + code);
}

std::string statusName(SubmissionStatus status) {
    switch (status) {
        case SubmissionStatus::Submitted: return "Submitted";
        case SubmissionStatus::Late: return "Late";
        case SubmissionStatus::Marked: return "Marked";
        case SubmissionStatus::Returned: return "Returned";
        case SubmissionStatus::Moderated: return "Moderated";
        case SubmissionStatus::Released: return "Released";
    }
    return "";
}

}  // namespace lms
