// Entities.h - small domain classes that map one-to-one to tables (SDD 5.5):
// Department, Course, Extension and AuditEntry.
#pragma once

#include <optional>
#include <string>
#include <utility>

#include "domain/Types.h"

namespace lms {

class Department {
public:
    Department(int id, std::string name, std::optional<int> hodUserId)
        : id_(id), name_(std::move(name)), hodUserId_(hodUserId) {}

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::optional<int>& hodUserId() const { return hodUserId_; }

private:
    int id_;
    std::string name_;
    std::optional<int> hodUserId_;
};

class Course {
public:
    Course(int id, std::string code, std::string title, int departmentId)
        : id_(id), code_(std::move(code)), title_(std::move(title)), departmentId_(departmentId) {}

    int id() const { return id_; }
    const std::string& code() const { return code_; }
    const std::string& title() const { return title_; }
    int departmentId() const { return departmentId_; }

private:
    int id_;
    std::string code_;
    std::string title_;
    int departmentId_;
};

// An individual extension for one student on one assignment (FR-08, BR-03).
class Extension {
public:
    Extension(int assignmentId, int studentId, TimePoint newDeadline, std::string reason,
              int grantedBy)
        : assignmentId_(assignmentId),
          studentId_(studentId),
          newDeadline_(newDeadline),
          reason_(std::move(reason)),
          grantedBy_(grantedBy) {}

    int assignmentId() const { return assignmentId_; }
    int studentId() const { return studentId_; }
    TimePoint newDeadline() const { return newDeadline_; }
    const std::string& reason() const { return reason_; }
    int grantedBy() const { return grantedBy_; }

private:
    int assignmentId_;
    int studentId_;
    TimePoint newDeadline_;
    std::string reason_;
    int grantedBy_;
};

// One row of the append-only audit log (FR-21): who, when, old value, new value, why.
class AuditEntry {
public:
    AuditEntry(int submissionId, int changedBy, TimePoint changedAt, std::string action,
               std::optional<int> oldMark, std::optional<int> newMark,
               std::optional<std::string> reason)
        : submissionId_(submissionId),
          changedBy_(changedBy),
          changedAt_(changedAt),
          action_(std::move(action)),
          oldMark_(oldMark),
          newMark_(newMark),
          reason_(std::move(reason)) {}

    int submissionId() const { return submissionId_; }
    int changedBy() const { return changedBy_; }
    TimePoint changedAt() const { return changedAt_; }
    const std::string& action() const { return action_; }
    const std::optional<int>& oldMark() const { return oldMark_; }
    const std::optional<int>& newMark() const { return newMark_; }
    const std::optional<std::string>& reason() const { return reason_; }

private:
    int submissionId_;
    int changedBy_;
    TimePoint changedAt_;
    std::string action_;  // MARK, REMARK, HOD_ADJUST, APPROVE, RETURN, RELEASE
    std::optional<int> oldMark_;
    std::optional<int> newMark_;
    std::optional<std::string> reason_;
};

}  // namespace lms
