// Mark.h - the raw mark and feedback for one submission (FR-14).
// The final mark is NOT stored: it is worked out by the late-penalty policy (SDD 5.2).
#pragma once

#include <optional>
#include <string>
#include <utility>

#include "domain/Types.h"

namespace lms {

class Mark {
public:
    Mark(int submissionId, int rawMark, std::string feedback, std::string policyCode, int markedBy,
         TimePoint markedAt)
        : submissionId_(submissionId),
          rawMark_(rawMark),
          feedback_(std::move(feedback)),
          policyCode_(std::move(policyCode)),
          markedBy_(markedBy),
          markedAt_(markedAt) {}

    int submissionId() const { return submissionId_; }
    int rawMark() const { return rawMark_; }
    const std::string& feedback() const { return feedback_; }
    const std::string& policyCode() const { return policyCode_; }
    int markedBy() const { return markedBy_; }
    TimePoint markedAt() const { return markedAt_; }
    const std::optional<int>& moderatedBy() const { return moderatedBy_; }
    const std::optional<TimePoint>& moderatedAt() const { return moderatedAt_; }
    const std::optional<std::string>& hodComment() const { return hodComment_; }

    void setRawMark(int raw) { rawMark_ = raw; }
    void setModerated(int hodId, TimePoint at) {
        moderatedBy_ = hodId;
        moderatedAt_ = at;
    }
    void setHodComment(const std::optional<std::string>& comment) { hodComment_ = comment; }

private:
    int submissionId_;
    int rawMark_;
    std::string feedback_;
    std::string policyCode_;
    int markedBy_;
    TimePoint markedAt_;
    std::optional<int> moderatedBy_;
    std::optional<TimePoint> moderatedAt_;
    std::optional<std::string> hodComment_;
};

}  // namespace lms
