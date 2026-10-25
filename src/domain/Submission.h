// Submission.h - one student's submission for one assignment, with every file version
// (FR-10, FR-11) and the life-cycle rules from the state machine (SDD Figure 3).
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "domain/Types.h"

namespace lms {

class SubmissionVersion {
public:
    SubmissionVersion(int versionNo, std::string storedPath, std::string originalName,
                      std::int64_t sizeBytes, TimePoint submittedAt)
        : versionNo_(versionNo),
          storedPath_(std::move(storedPath)),
          originalName_(std::move(originalName)),
          sizeBytes_(sizeBytes),
          submittedAt_(submittedAt) {}

    int versionNo() const { return versionNo_; }
    const std::string& storedPath() const { return storedPath_; }
    const std::string& originalName() const { return originalName_; }
    std::int64_t sizeBytes() const { return sizeBytes_; }
    TimePoint submittedAt() const { return submittedAt_; }

private:
    int versionNo_;
    std::string storedPath_;
    std::string originalName_;
    std::int64_t sizeBytes_;
    TimePoint submittedAt_;
};

class Submission {
public:
    Submission(int id, int assignmentId, int studentId, SubmissionStatus status);

    // The highest version number is the one that gets marked (FR-11).
    const SubmissionVersion& latest() const;
    void addVersion(const SubmissionVersion& v);

    // Throws InvalidTransitionException if the state machine does not allow the move.
    void transitionTo(SubmissionStatus next);
    static bool canTransition(SubmissionStatus from, SubmissionStatus to);

    int id() const { return id_; }
    int assignmentId() const { return assignmentId_; }
    int studentId() const { return studentId_; }
    SubmissionStatus status() const { return status_; }
    const std::vector<SubmissionVersion>& versions() const { return versions_; }
    void setId(int id) { id_ = id; }

private:
    int id_;
    int assignmentId_;
    int studentId_;
    SubmissionStatus status_;
    std::vector<SubmissionVersion> versions_;
};

}  // namespace lms
