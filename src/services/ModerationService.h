// ModerationService.h - HoD moderation, release and the audit log (UC-11 to UC-15).
#pragma once

#include <string>
#include <vector>

#include "data/Repositories.h"
#include "domain/ILatePenaltyPolicy.h"
#include "domain/User.h"
#include "services/MarkView.h"

namespace lms {

class ModerationService {
public:
    ModerationService(ICourseRepository& courses, ISubmissionRepository& submissions,
                      IMarkRepository& marks, const ILatePenaltyPolicy& policy);

    std::vector<MarkView> listForModeration(const HeadOfDepartment& h, int assignmentId);
    void approve(const HeadOfDepartment& h, int submissionId, TimePoint now);
    double adjust(const HeadOfDepartment& h, int submissionId, int newRaw,
                  const std::string& reason, TimePoint now);
    void returnForRemarking(const HeadOfDepartment& h, int submissionId,
                            const std::string& comment, TimePoint now);
    void release(const HeadOfDepartment& h, int assignmentId, TimePoint now);
    std::vector<AuditEntry> viewAuditLog(const User& u, int submissionId);

    // Added in Phase 4 for the list screens.
    std::vector<Assignment> listAssignmentsInDepartment(const HeadOfDepartment& h);

private:
    Assignment checkDepartment(const HeadOfDepartment& h, int assignmentId);

    struct Loaded {
        Submission submission;
        Assignment assignment;
        Mark mark;
    };
    Loaded loadMarked(const HeadOfDepartment& h, int submissionId);

    ICourseRepository& courses_;
    ISubmissionRepository& submissions_;
    IMarkRepository& marks_;
    const ILatePenaltyPolicy& policy_;
};

}  // namespace lms
