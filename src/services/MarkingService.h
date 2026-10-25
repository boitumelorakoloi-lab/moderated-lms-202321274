// MarkingService.h - mark and re-mark submissions (UC-08, UC-09, UC-10).
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "data/Repositories.h"
#include "domain/ILatePenaltyPolicy.h"
#include "domain/User.h"
#include "services/MarkView.h"

namespace lms {

class MarkingService {
public:
    MarkingService(ICourseRepository& courses, ISubmissionRepository& submissions,
                   IMarkRepository& marks, const ILatePenaltyPolicy& policy);

    // Submissions the lecturer can (re)mark: Submitted, Late, Marked or Returned.
    std::vector<MarkView> listToMark(const Lecturer& l, int assignmentId);

    // Returns the final mark after the late penalty.
    double mark(const Lecturer& l, int submissionId, int raw, const std::string& feedback,
                const std::optional<std::string>& reason, TimePoint now);

private:
    ICourseRepository& courses_;
    ISubmissionRepository& submissions_;
    IMarkRepository& marks_;
    const ILatePenaltyPolicy& policy_;
};

}  // namespace lms
