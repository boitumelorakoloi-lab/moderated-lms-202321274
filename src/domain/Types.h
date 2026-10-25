// Types.h - shared types and enumerations for the Moderated LMS (SDD Figure 2).
#pragma once

#include <chrono>
#include <string>

namespace lms {

// All times inside the program are system_clock time points (SDD section 1).
using TimePoint = std::chrono::system_clock::time_point;

enum class Role { Student, Lecturer, HeadOfDepartment, Administrator };

// The six states of the Submission life cycle (SDD Figure 3, SRS 3.3).
enum class SubmissionStatus { Submitted, Late, Marked, Returned, Moderated, Released };

// Result of checking a submission before the student confirms (SDD Figure 5).
enum class SubmissionCheck { OnTime, Resubmit, Late, Rejected };

enum class Permission {
    SubmitWork,
    ViewOwnMarks,
    CreateAssignment,
    GrantExtension,
    MarkSubmission,
    ExportMarks,
    ModerateMarks,
    ReleaseMarks,
    ViewAuditLog,
    ManageUsers,
    ManageCourses,
    BackupRestore
};

// Text codes stored in the database (CHECK constraints in schema.sql).
std::string roleCode(Role role);
Role roleFromCode(const std::string& code);
std::string roleName(Role role);

std::string statusCode(SubmissionStatus status);
SubmissionStatus statusFromCode(const std::string& code);
std::string statusName(SubmissionStatus status);

}  // namespace lms
