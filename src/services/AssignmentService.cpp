// AssignmentService.cpp
#include "services/AssignmentService.h"

#include "domain/Exceptions.h"
#include "domain/Time.h"
#include "services/MarkView.h"

namespace lms {

AssignmentService::AssignmentService(ICourseRepository& courses, ISubmissionRepository& submissions)
    : courses_(courses), submissions_(submissions) {}

int AssignmentService::createAssignment(const Lecturer& l, int courseId, const std::string& title,
                                        TimePoint deadline, int maxMark) {
    if (!l.can(Permission::CreateAssignment)) {
        throw AccessDeniedException("You are not allowed to create assignments.");
    }
    if (!courses_.findCourse(courseId)) {
        throw ValidationException("There is no course with ID " + std::to_string(courseId) + ".");
    }
    if (!courses_.teaches(l.id(), courseId)) {
        throw AccessDeniedException("You do not teach this course.");
    }
    if (isBlank(title)) {
        throw ValidationException("The title cannot be blank.");
    }
    if (maxMark < 1 || maxMark > 100) {
        throw ValidationException("The maximum mark must be between 1 and 100.");  // FR-07
    }
    return courses_.addAssignment(Assignment(0, courseId, trim(title), deadline, maxMark, l.id()));
}

Assignment AssignmentService::getAssignmentITeach(const Lecturer& l, int assignmentId) {
    auto assignment = courses_.findAssignment(assignmentId);
    if (!assignment) {
        throw ValidationException("There is no assignment with ID " + std::to_string(assignmentId) +
                                  ".");
    }
    if (!courses_.teaches(l.id(), assignment->courseId())) {
        throw AccessDeniedException("You do not teach the course this assignment belongs to.");
    }
    return *assignment;
}

// BR-03: only the course lecturer, to one student, with a reason, before the work is marked.
void AssignmentService::grantExtension(const Lecturer& l, int assignmentId, int studentId,
                                       TimePoint newDeadline, const std::string& reason,
                                       TimePoint now) {
    if (!l.can(Permission::GrantExtension)) {
        throw AccessDeniedException("You are not allowed to grant extensions.");
    }
    const Assignment assignment = getAssignmentITeach(l, assignmentId);
    if (!courses_.isEnrolled(studentId, assignment.courseId())) {
        throw ValidationException("Student " + std::to_string(studentId) +
                                  " is not enrolled in this course.");
    }
    if (isBlank(reason)) {
        throw ValidationException("A reason is required for an extension.");
    }
    if (newDeadline <= assignment.deadline()) {
        throw ValidationException("The new deadline must be after the normal deadline.");
    }

    auto submission = submissions_.find(assignmentId, studentId);
    if (submission) {
        const SubmissionStatus st = submission->status();
        if (st != SubmissionStatus::Submitted && st != SubmissionStatus::Late) {
            throw ValidationException(
                "This work has already been marked, so no extension can be given.");
        }
        const bool coversWork = submission->latest().submittedAt() <= newDeadline;
        if (st == SubmissionStatus::Submitted && !coversWork) {
            throw ValidationException(
                "That deadline is earlier than the work that was already handed in.");
        }
    } else if (newDeadline <= now) {
        // Nothing handed in yet, so a deadline in the past would be of no use to the student.
        throw ValidationException("The new deadline has already passed.");
    }

    courses_.saveExtension(Extension(assignmentId, studentId, newDeadline, trim(reason), l.id()));

    // State machine: Late -> Submitted when the extension covers the submission.
    if (submission && submission->status() == SubmissionStatus::Late &&
        submission->latest().submittedAt() <= newDeadline) {
        submission->transitionTo(SubmissionStatus::Submitted);
        submissions_.updateStatus(submission->id(), SubmissionStatus::Submitted);
    }
}

// BR-06 (CR1): only the course lecturer, with a reason, to a deadline after the normal one and
// after any existing class extension, only before marking starts, and not in the past unless it
// turns late work into on-time work.
int AssignmentService::extendForClass(const Lecturer& l, int assignmentId, TimePoint newDeadline,
                                      const std::string& reason, TimePoint now) {
    if (!l.can(Permission::GrantExtension)) {
        throw AccessDeniedException("You are not allowed to grant extensions.");
    }
    const Assignment assignment = getAssignmentITeach(l, assignmentId);
    if (isBlank(reason)) {
        throw ValidationException("A reason is required for an extension.");
    }
    if (newDeadline <= assignment.deadline()) {
        throw ValidationException("The new deadline must be after the normal deadline.");
    }
    const auto current = courses_.findClassExtension(assignmentId);
    if (current && newDeadline <= current->newDeadline()) {
        // Students may already be planning around the class deadline, so it can only move later.
        throw ValidationException("The class already has an extension to " +
                                  formatTime(current->newDeadline()) +
                                  ". A new one must be later.");
    }

    const std::vector<int> nowOnTime = lateWorkCoveredBy(assignmentId, newDeadline);
    if (newDeadline <= now && nowOnTime.empty()) {
        // A deadline in the past is only of use if it forgives work that was handed in late.
        throw ValidationException("The new deadline has already passed.");
    }
    courses_.saveClassExtension(
        Extension(assignmentId, std::nullopt, newDeadline, trim(reason), l.id()), nowOnTime);
    return static_cast<int>(nowOnTime.size());
}

// The Late submissions a class deadline would make on time. Throws if marking has started.
std::vector<int> AssignmentService::lateWorkCoveredBy(int assignmentId, TimePoint newDeadline) {
    std::vector<int> ids;
    for (const Submission& sub : submissions_.listForAssignment(assignmentId)) {
        const SubmissionStatus st = sub.status();
        if (st != SubmissionStatus::Submitted && st != SubmissionStatus::Late) {
            throw ValidationException(
                "Marking has already started, so the class deadline can no longer be moved. "
                "Give individual extensions instead.");
        }
        // State machine: Late -> Submitted when the new deadline covers the work.
        if (st == SubmissionStatus::Late && sub.latest().submittedAt() <= newDeadline) {
            ids.push_back(sub.id());
        }
    }
    return ids;
}

std::optional<Extension> AssignmentService::classExtension(const Lecturer& l, int assignmentId) {
    return courses_.findClassExtension(getAssignmentITeach(l, assignmentId).id());
}

std::vector<Assignment> AssignmentService::listForStudent(const Student& s) {
    if (!s.can(Permission::SubmitWork)) {
        throw AccessDeniedException("Only students have an assignment list.");
    }
    return courses_.listAssignmentsFor(s.id());
}

std::vector<StudentAssignmentRow> AssignmentService::overviewForStudent(const Student& s) {
    std::vector<StudentAssignmentRow> rows;
    for (const Assignment& a : listForStudent(s)) {
        const auto ext = applicableExtension(courses_, a.id(), s.id());
        const auto course = courses_.findCourse(a.courseId());
        StudentAssignmentRow row{a, course ? course->code() : "?", a.effectiveDeadline(ext), ext,
                                 std::nullopt};
        if (auto sub = submissions_.find(a.id(), s.id())) {
            row.status = sub->status();
        }
        rows.push_back(row);
    }
    return rows;
}

std::vector<Course> AssignmentService::listCoursesForStudent(const Student& s) {
    if (!s.can(Permission::SubmitWork)) {
        throw AccessDeniedException("Only students have a course list.");
    }
    return courses_.listCoursesForStudent(s.id());
}

std::vector<Course> AssignmentService::listMyCourses(const Lecturer& l) {
    if (!l.can(Permission::CreateAssignment)) {
        throw AccessDeniedException("Only lecturers have a teaching list.");
    }
    return courses_.listCoursesForLecturer(l.id());
}

std::vector<Assignment> AssignmentService::listForCourse(const Lecturer& l, int courseId) {
    if (!l.can(Permission::CreateAssignment) || !courses_.teaches(l.id(), courseId)) {
        throw AccessDeniedException("You do not teach this course.");
    }
    return courses_.listAssignmentsForCourse(courseId);
}

}  // namespace lms
