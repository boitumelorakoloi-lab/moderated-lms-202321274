// MarkingService.cpp
#include "services/MarkingService.h"

#include "domain/Exceptions.h"

namespace lms {

MarkingService::MarkingService(ICourseRepository& courses, ISubmissionRepository& submissions,
                               IMarkRepository& marks, const ILatePenaltyPolicy& policy)
    : courses_(courses), submissions_(submissions), marks_(marks), policy_(policy) {}

namespace {
// E1 of UC-08: check the input before anything is saved (FR-14, C8).
void checkMarkInput(int raw, int maxMark, const std::string& feedback) {
    if (raw < 0 || raw > maxMark) {
        throw ValidationException("The mark must be between 0 and " + std::to_string(maxMark) +
                                  ".");
    }
    if (isBlank(feedback)) {
        throw ValidationException("Written feedback is required.");
    }
}
}  // namespace

Assignment MarkingService::assignmentITeach(const Lecturer& l, int assignmentId) {
    if (!l.can(Permission::MarkSubmission)) {
        throw AccessDeniedException("You are not allowed to mark work.");
    }
    const auto assignment = courses_.findAssignment(assignmentId);
    if (!assignment) {
        throw ValidationException("There is no assignment with ID " + std::to_string(assignmentId) +
                                  ".");
    }
    if (!courses_.teaches(l.id(), assignment->courseId())) {
        throw AccessDeniedException("You do not teach the course this work belongs to.");
    }
    return *assignment;
}

std::vector<MarkView> MarkingService::listToMark(const Lecturer& l, int assignmentId) {
    const Assignment assignment = assignmentITeach(l, assignmentId);
    std::vector<MarkView> rows;
    for (const Submission& s : submissions_.listForAssignment(assignmentId)) {
        const SubmissionStatus st = s.status();
        if (st == SubmissionStatus::Submitted || st == SubmissionStatus::Late ||
            st == SubmissionStatus::Marked || st == SubmissionStatus::Returned) {
            rows.push_back(buildMarkView(s, assignment,
                                         courses_.findExtension(assignmentId, s.studentId()),
                                         marks_.find(s.id()), policy_));
        }
    }
    return rows;
}

double MarkingService::mark(const Lecturer& l, int submissionId, int raw,
                            const std::string& feedback, const std::optional<std::string>& reason,
                            TimePoint now) {
    auto submission = submissions_.findById(submissionId);
    if (!submission) {
        throw ValidationException("There is no submission with ID " + std::to_string(submissionId) +
                                  ".");
    }
    const Assignment assignment = assignmentITeach(l, submission->assignmentId());
    checkMarkInput(raw, assignment.maxMark(), feedback);

    Mark m(submissionId, raw, trim(feedback), policy_.code(), l.id(), now);
    std::string action = "MARK";  // the first mark needs no reason
    std::optional<int> oldMark;
    std::optional<std::string> why;
    if (const auto existing = marks_.find(submissionId)) {
        if (!reason.has_value() || isBlank(*reason)) {
            throw ValidationException("A reason is required when a mark is changed (BR-05).");
        }
        action = "REMARK";  // FR-16
        oldMark = existing->rawMark();
        why = trim(*reason);
        m.setHodComment(existing->hodComment());  // keep the HoD's comment for the record
    }

    // Throws InvalidTransitionException if the work is already Moderated or Released.
    submission->transitionTo(SubmissionStatus::Marked);
    marks_.saveChange(m, SubmissionStatus::Marked,
                      AuditEntry(submissionId, l.id(), now, action, oldMark, raw, why));

    return buildMarkView(*submission, assignment,
                         courses_.findExtension(assignment.id(), submission->studentId()), m,
                         policy_)
        .finalMark;
}

}  // namespace lms
