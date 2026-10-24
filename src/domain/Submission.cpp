// Submission.cpp
#include "domain/Submission.h"

#include "domain/Exceptions.h"

namespace lms {

Submission::Submission(int id, int assignmentId, int studentId, SubmissionStatus status)
    : id_(id), assignmentId_(assignmentId), studentId_(studentId), status_(status) {}

const SubmissionVersion& Submission::latest() const {
    if (versions_.empty()) {
        throw ValidationException("This submission has no files.");
    }
    return versions_.back();
}

void Submission::addVersion(const SubmissionVersion& v) {
    versions_.push_back(v);
}

void Submission::transitionTo(SubmissionStatus next) {
    if (!canTransition(status_, next)) {
        throw InvalidTransitionException("A submission that is " + statusName(status_) +
                                         " cannot become " + statusName(next) + ".");
    }
    status_ = next;
}

// Arrows of the state machine diagram (SDD Figure 3).
bool Submission::canTransition(SubmissionStatus from, SubmissionStatus to) {
    switch (from) {
        case SubmissionStatus::Submitted:
            // resubmit before the deadline, or mark
            return to == SubmissionStatus::Submitted || to == SubmissionStatus::Marked;
        case SubmissionStatus::Late:
            // an extension that covers the submission, or mark (no resubmit: BR-04)
            return to == SubmissionStatus::Submitted || to == SubmissionStatus::Marked;
        case SubmissionStatus::Marked:
            // change mark (reason), return, approve or adjust
            return to == SubmissionStatus::Marked || to == SubmissionStatus::Returned ||
                   to == SubmissionStatus::Moderated;
        case SubmissionStatus::Returned:
            return to == SubmissionStatus::Marked;  // re-mark with a reason
        case SubmissionStatus::Moderated:
            return to == SubmissionStatus::Released;  // only when all are Moderated
        case SubmissionStatus::Released:
            return false;  // final state
    }
    return false;
}

}  // namespace lms
