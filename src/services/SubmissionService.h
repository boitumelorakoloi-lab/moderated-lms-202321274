// SubmissionService.h - submit, resubmit and submit late (UC-17 to UC-20), and the
// student's released results (UC-21).
#pragma once

#include <string>
#include <vector>

#include "data/Repositories.h"
#include "domain/ILatePenaltyPolicy.h"
#include "domain/User.h"
#include "services/MarkView.h"

namespace lms {

// What check() found, so the menu can warn about a penalty before the student confirms.
struct CheckResult {
    SubmissionCheck result = SubmissionCheck::Rejected;
    TimePoint deadline;
    int daysLate = 0;
    int nextVersion = 1;
    std::string message;
};

// A released mark as the student sees it (FR-20).
struct StudentResult {
    Assignment assignment;
    MarkView view;
};

class SubmissionService {
public:
    static constexpr std::int64_t kMaxFileBytes = 10LL * 1024 * 1024;  // FR-13: 10 MB

    SubmissionService(ICourseRepository& courses, ISubmissionRepository& submissions,
                      IMarkRepository& marks, const ILatePenaltyPolicy& policy,
                      std::string storageFolder);

    CheckResult check(const Student& s, int assignmentId, const std::string& path, TimePoint now);
    int submit(const Student& s, int assignmentId, const std::string& path, TimePoint now);

    std::vector<StudentResult> myResults(const Student& s);

private:
    void validateFile(const std::string& path) const;

    ICourseRepository& courses_;
    ISubmissionRepository& submissions_;
    IMarkRepository& marks_;
    const ILatePenaltyPolicy& policy_;
    std::string storageFolder_;
};

}  // namespace lms
