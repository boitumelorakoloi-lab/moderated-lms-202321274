// Repositories.h - Repository pattern interfaces (SDD Figure 2, "Data access").
// Services only see these interfaces; the SQLite classes behind them are chosen in main().
// There is deliberately no method to edit or delete an audit entry (FR-21).
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "domain/Assignment.h"
#include "domain/Entities.h"
#include "domain/Mark.h"
#include "domain/Submission.h"
#include "domain/User.h"

namespace lms {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual std::unique_ptr<User> findByUsername(const std::string& name) = 0;
    virtual std::unique_ptr<User> findById(int id) = 0;
    virtual std::vector<std::unique_ptr<User>> listAll() = 0;
    virtual int add(const User& u) = 0;
    virtual void update(const User& u) = 0;
};

class ICourseRepository {
public:
    virtual ~ICourseRepository() = default;
    virtual std::optional<Course> findCourse(int id) = 0;
    virtual std::optional<Assignment> findAssignment(int id) = 0;
    virtual std::optional<Extension> findExtension(int assignmentId, int studentId) = 0;
    virtual bool isEnrolled(int studentId, int courseId) = 0;
    virtual bool teaches(int lecturerId, int courseId) = 0;
    virtual std::vector<Assignment> listAssignmentsFor(int studentId) = 0;
    virtual int addCourse(const Course& c) = 0;
    virtual int addAssignment(const Assignment& a) = 0;
    virtual void saveExtension(const Extension& e) = 0;
    virtual void enrol(int studentId, int courseId) = 0;
    virtual void assignLecturer(int lecturerId, int courseId) = 0;

    // Added in Phase 4 for the list screens and FR-05 (see Implementation Report, section 9).
    virtual int addDepartment(const Department& d) = 0;
    virtual void setDepartmentHod(int departmentId, int hodUserId) = 0;
    virtual std::optional<Department> findDepartment(int id) = 0;
    virtual std::vector<Department> listDepartments() = 0;
    virtual std::vector<Course> listCourses() = 0;
    virtual std::vector<Course> listCoursesForLecturer(int lecturerId) = 0;
    virtual std::vector<Course> listCoursesForStudent(int studentId) = 0;
    virtual std::vector<Assignment> listAssignmentsForCourse(int courseId) = 0;
};

class ISubmissionRepository {
public:
    virtual ~ISubmissionRepository() = default;
    virtual std::optional<Submission> find(int assignmentId, int studentId) = 0;
    virtual std::optional<Submission> findById(int id) = 0;
    virtual std::vector<Submission> listForAssignment(int assignmentId) = 0;
    virtual int addSubmission(const Submission& s) = 0;
    virtual void addVersion(int submissionId, const SubmissionVersion& v) = 0;
    virtual void updateStatus(int id, SubmissionStatus st) = 0;
    virtual int countNotIn(int assignmentId, SubmissionStatus st) = 0;
    virtual void releaseAll(int assignmentId, int by, TimePoint at) = 0;
};

class IMarkRepository {
public:
    virtual ~IMarkRepository() = default;
    virtual std::optional<Mark> find(int submissionId) = 0;
    // Saves the mark, the new submission state and the audit entry in ONE transaction.
    virtual void saveChange(const Mark& m, SubmissionStatus st, const AuditEntry& e) = 0;
    virtual std::vector<AuditEntry> listAudit(int submissionId) = 0;
};

// Backup and restore of the whole database file (FR-25).
class IBackupStore {
public:
    virtual ~IBackupStore() = default;
    virtual void backupTo(const std::string& path) = 0;
    virtual void restoreFrom(const std::string& path) = 0;
};

}  // namespace lms
