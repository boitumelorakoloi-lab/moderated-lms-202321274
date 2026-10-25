// MarkView.h - one row of a marking / moderation / results list: the submission with its
// raw mark, days late, penalty, final mark, feedback and any extension (UC-08, UC-11, FR-17).
#pragma once

#include <optional>
#include <string>

#include "domain/Assignment.h"
#include "domain/Entities.h"
#include "domain/ILatePenaltyPolicy.h"
#include "domain/Mark.h"
#include "domain/Submission.h"

namespace lms {

struct MarkView {
    Submission submission;
    std::optional<Mark> mark;
    std::optional<Extension> extension;
    TimePoint effectiveDeadline;
    int daysLate = 0;
    int maxMark = 0;
    double penalty = 0.0;     // raw - final (0 when not marked)
    double finalMark = 0.0;   // worked out by the Strategy, never stored (SDD 5.2)
};

// Builds a row. Used by MarkingService, ModerationService and SubmissionService so the
// penalty is always worked out the same way.
MarkView buildMarkView(const Submission& submission, const Assignment& assignment,
                       const std::optional<Extension>& extension, const std::optional<Mark>& mark,
                       const ILatePenaltyPolicy& policy);

// Shared input checks used by several services.
std::string trim(const std::string& text);
bool isBlank(const std::string& text);

}  // namespace lms
