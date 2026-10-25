// AssignmentService.cpp
#include "services/AssignmentService.h"

#include "domain/Exceptions.h"
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
        throw ValidationException("There is no assignment with ID " + std::to_string(assignmentId) + ".");
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
            throw ValidationException("This work has already been marked, so no extension can be given.");
        }
        const bool coversWork = submission->latest().submittedAt() <= newDeadline;
        if (st == SubmissionStatus::Submitted && !coversWork) {
            throw ValidationException("That deadline is earlier than the work that was already handed in.");
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

std::vector<Assignment> AssignmentService::listForStudent(const Student& s) {
    if (!s.can(Permission::SubmitWork)) {
        throw AccessDeniedException("Only students have an assignment list.");
    }
    return courses_.listAssignmentsFor(s.id());
}

std::vector<StudentAssignmentRow> AssignmentService::overviewForStudent(const Student& s) {
    std::vector<StudentAssignmentRow> rows;
    for (const Assignment& a : listForStudent(s)) {
        const auto ext = courses_.findExtension(a.id(), s.id());
        const auto course = courses_.findCourse(a.courseId());
        StudentAssignmentRow row{a, course ? course->code() : "?", a.effectiveDeadline(ext),
                                 ext.has_value(), std::nullopt};
        if (auto sub = submissions_.find(a.id(), s.id())) {
            row.status = sub->status();
        }
        rows.push_back(row);
    }
    return rows;
}

std::vector<Course> AssignmentService::listCoursesForStudent(const Student& s) {
    return courses_.listCoursesForStudent(s.id());
}

std::vector<Course> AssignmentService::listMyCourses(const Lecturer& l) {
    return courses_.listCoursesForLecturer(l.id());
}

std::vector<Assignment> AssignmentService::listForCourse(const Lecturer& l, int courseId) {
    if (!courses_.teaches(l.id(), courseId)) {
        throw AccessDeniedException("You do not teach this course.");
    }
    return courses_.listAssignmentsForCourse(courseId);
}

}  // namespace lms
