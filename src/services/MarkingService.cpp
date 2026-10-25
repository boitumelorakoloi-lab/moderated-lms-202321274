// MarkingService.cpp
#include "services/MarkingService.h"

#include "domain/Exceptions.h"

namespace lms {

MarkingService::MarkingService(ICourseRepository& courses, ISubmissionRepository& submissions,
                               IMarkRepository& marks, const ILatePenaltyPolicy& policy)
    : courses_(courses), submissions_(submissions), marks_(marks), policy_(policy) {}

std::vector<MarkView> MarkingService::listToMark(const Lecturer& l, int assignmentId) {
    if (!l.can(Permission::MarkSubmission)) {
        throw AccessDeniedException("You are not allowed to mark work.");
    }
    const auto assignment = courses_.findAssignment(assignmentId);
    if (!assignment) {
        throw ValidationException("There is no assignment with ID " + std::to_string(assignmentId) + ".");
    }
    if (!courses_.teaches(l.id(), assignment->courseId())) {
        throw AccessDeniedException("You do not teach the course this assignment belongs to.");
    }
    std::vector<MarkView> rows;
    for (const Submission& s : submissions_.listForAssignment(assignmentId)) {
        const SubmissionStatus st = s.status();
        if (st == SubmissionStatus::Submitted || st == SubmissionStatus::Late ||
            st == SubmissionStatus::Marked || st == SubmissionStatus::Returned) {
            rows.push_back(buildMarkView(s, *assignment,
                                         courses_.findExtension(assignmentId, s.studentId()),
                                         marks_.find(s.id()), policy_));
        }
    }
    return rows;
}

double MarkingService::mark(const Lecturer& l, int submissionId, int raw,
                            const std::string& feedback, const std::optional<std::string>& reason,
                            TimePoint now) {
    if (!l.can(Permission::MarkSubmission)) {
        throw AccessDeniedException("You are not allowed to mark work.");
    }
    auto submission = submissions_.findById(submissionId);
    if (!submission) {
        throw ValidationException("There is no submission with ID " + std::to_string(submissionId) + ".");
    }
    const auto assignment = courses_.findAssignment(submission->assignmentId());
    if (!assignment || !courses_.teaches(l.id(), assignment->courseId())) {
        throw AccessDeniedException("You do not teach the course this submission belongs to.");
    }

    // E1: check the input before anything is saved (FR-14, C8).
    if (raw < 0 || raw > assignment->maxMark()) {
        throw ValidationException("The mark must be between 0 and " +
                                  std::to_string(assignment->maxMark()) + ".");
    }
    if (isBlank(feedback)) {
        throw ValidationException("Written feedback is required.");
    }

    const auto existing = marks_.find(submissionId);
    const bool isChange = existing.has_value();
    if (isChange && (!reason.has_value() || isBlank(*reason))) {
        throw ValidationException("A reason is required when a mark is changed (BR-05).");  // FR-16
    }

    // Throws InvalidTransitionException if the work is already Moderated or Released.
    submission->transitionTo(SubmissionStatus::Marked);

    Mark m(submissionId, raw, trim(feedback), policy_.code(), l.id(), now);
    if (existing) {
        m.setHodComment(existing->hodComment());  // keep the HoD's comment for the record
    }
    const AuditEntry entry(submissionId, l.id(), now, isChange ? "REMARK" : "MARK",
                           isChange ? std::optional<int>(existing->rawMark()) : std::nullopt, raw,
                           isChange ? std::optional<std::string>(trim(*reason)) : std::nullopt);
    marks_.saveChange(m, SubmissionStatus::Marked, entry);

    const MarkView view = buildMarkView(*submission, *assignment,
                                        courses_.findExtension(assignment->id(), submission->studentId()),
                                        m, policy_);
    return view.finalMark;
}

}  // namespace lms
