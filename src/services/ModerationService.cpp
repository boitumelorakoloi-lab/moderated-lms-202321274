// ModerationService.cpp - follows the moderation and release sequence diagrams
// (SDD Figures 6 and 7).
#include "services/ModerationService.h"

#include "domain/Exceptions.h"

namespace lms {

ModerationService::ModerationService(ICourseRepository& courses, ISubmissionRepository& submissions,
                                     IMarkRepository& marks, const ILatePenaltyPolicy& policy)
    : courses_(courses), submissions_(submissions), marks_(marks), policy_(policy) {}

// E1 of UC-11: an HoD may only moderate courses in their own department (NFR-08).
Assignment ModerationService::checkDepartment(const HeadOfDepartment& h, int assignmentId) {
    if (!h.can(Permission::ModerateMarks)) {
        throw AccessDeniedException("You are not allowed to moderate marks.");
    }
    const auto assignment = courses_.findAssignment(assignmentId);
    if (!assignment) {
        throw ValidationException("There is no assignment with ID " + std::to_string(assignmentId) + ".");
    }
    const auto course = courses_.findCourse(assignment->courseId());
    if (!course || h.departmentId() == 0 || course->departmentId() != h.departmentId()) {
        throw AccessDeniedException("Not in your department.");
    }
    return *assignment;
}

std::vector<MarkView> ModerationService::listForModeration(const HeadOfDepartment& h,
                                                           int assignmentId) {
    const Assignment assignment = checkDepartment(h, assignmentId);
    std::vector<MarkView> rows;
    for (const Submission& s : submissions_.listForAssignment(assignmentId)) {
        rows.push_back(buildMarkView(s, assignment,
                                     courses_.findExtension(assignmentId, s.studentId()),
                                     marks_.find(s.id()), policy_));
    }
    return rows;
}

ModerationService::Loaded ModerationService::loadMarked(const HeadOfDepartment& h, int submissionId) {
    const auto submission = submissions_.findById(submissionId);
    if (!submission) {
        throw ValidationException("There is no submission with ID " + std::to_string(submissionId) + ".");
    }
    const Assignment assignment = checkDepartment(h, submission->assignmentId());
    const auto mark = marks_.find(submissionId);
    if (!mark || submission->status() != SubmissionStatus::Marked) {
        throw InvalidTransitionException("Only work that is Marked can be moderated (this one is " +
                                         statusName(submission->status()) + ").");
    }
    return {*submission, assignment, *mark};
}

void ModerationService::approve(const HeadOfDepartment& h, int submissionId, TimePoint now) {
    Loaded d = loadMarked(h, submissionId);
    d.submission.transitionTo(SubmissionStatus::Moderated);
    d.mark.setModerated(h.id(), now);
    marks_.saveChange(d.mark, SubmissionStatus::Moderated,
                      AuditEntry(submissionId, h.id(), now, "APPROVE", d.mark.rawMark(),
                                 d.mark.rawMark(), std::nullopt));
}

double ModerationService::adjust(const HeadOfDepartment& h, int submissionId, int newRaw,
                                 const std::string& reason, TimePoint now) {
    Loaded d = loadMarked(h, submissionId);
    if (newRaw < 0 || newRaw > d.assignment.maxMark()) {
        throw ValidationException("The mark must be between 0 and " +
                                  std::to_string(d.assignment.maxMark()) + ".");
    }
    if (isBlank(reason)) {
        throw ValidationException("A reason is required when adjusting a mark (BR-05).");
    }
    const int oldRaw = d.mark.rawMark();
    d.submission.transitionTo(SubmissionStatus::Moderated);
    d.mark.setRawMark(newRaw);
    d.mark.setModerated(h.id(), now);
    marks_.saveChange(d.mark, SubmissionStatus::Moderated,
                      AuditEntry(submissionId, h.id(), now, "HOD_ADJUST", oldRaw, newRaw, trim(reason)));
    return buildMarkView(d.submission, d.assignment,
                         courses_.findExtension(d.assignment.id(), d.submission.studentId()), d.mark,
                         policy_)
        .finalMark;
}

void ModerationService::returnForRemarking(const HeadOfDepartment& h, int submissionId,
                                           const std::string& comment, TimePoint now) {
    Loaded d = loadMarked(h, submissionId);
    if (isBlank(comment)) {
        throw ValidationException("Please write a comment for the lecturer.");
    }
    d.submission.transitionTo(SubmissionStatus::Returned);
    d.mark.setHodComment(trim(comment));
    marks_.saveChange(d.mark, SubmissionStatus::Returned,
                      AuditEntry(submissionId, h.id(), now, "RETURN", d.mark.rawMark(),
                                 d.mark.rawMark(), trim(comment)));
}

// FR-19: release only when every submission for the assignment is Moderated.
void ModerationService::release(const HeadOfDepartment& h, int assignmentId, TimePoint now) {
    if (!h.can(Permission::ReleaseMarks)) {
        throw AccessDeniedException("You are not allowed to release marks.");
    }
    const Assignment assignment = checkDepartment(h, assignmentId);
    if (assignment.releasedAt().has_value()) {
        throw ValidationException("These marks were already released.");
    }
    if (submissions_.listForAssignment(assignmentId).empty()) {
        throw ValidationException("No work has been handed in for this assignment yet.");
    }
    const int notReady = submissions_.countNotIn(assignmentId, SubmissionStatus::Moderated);
    if (notReady > 0) {
        throw ValidationException(std::to_string(notReady) + " mark(s) still need approval.");
    }
    submissions_.releaseAll(assignmentId, h.id(), now);
}

std::vector<AuditEntry> ModerationService::viewAuditLog(const User& u, int submissionId) {
    if (!u.can(Permission::ViewAuditLog)) {
        throw AccessDeniedException("You are not allowed to view the audit log.");
    }
    const auto submission = submissions_.findById(submissionId);
    if (!submission) {
        throw ValidationException("There is no submission with ID " + std::to_string(submissionId) + ".");
    }
    // An HoD only sees their own department; the administrator sees everything (FR-22).
    if (const auto* hod = dynamic_cast<const HeadOfDepartment*>(&u)) {
        checkDepartment(*hod, submission->assignmentId());
    }
    return marks_.listAudit(submissionId);
}

std::vector<Assignment> ModerationService::listAssignmentsInDepartment(const HeadOfDepartment& h) {
    if (!h.can(Permission::ModerateMarks)) {
        throw AccessDeniedException("You are not allowed to moderate marks.");
    }
    std::vector<Assignment> result;
    for (const Course& c : courses_.listCourses()) {
        if (h.departmentId() != 0 && c.departmentId() == h.departmentId()) {
            for (const Assignment& a : courses_.listAssignmentsForCourse(c.id())) {
                result.push_back(a);
            }
        }
    }
    return result;
}

}  // namespace lms
