// SqliteSubmissionRepository.h - submission and submission_version tables.
#pragma once

#include "data/Database.h"
#include "data/Repositories.h"

namespace lms {

class SqliteSubmissionRepository final : public ISubmissionRepository {
public:
    explicit SqliteSubmissionRepository(Database& db) : db_(db) {}

    std::optional<Submission> find(int assignmentId, int studentId) override;
    std::optional<Submission> findById(int id) override;
    std::vector<Submission> listForAssignment(int assignmentId) override;
    int addSubmission(const Submission& s) override;
    void addVersion(int submissionId, const SubmissionVersion& v) override;
    void updateStatus(int id, SubmissionStatus st) override;
    int countNotIn(int assignmentId, SubmissionStatus st) override;
    void releaseAll(int assignmentId, int by, TimePoint at) override;

private:
    void loadVersions(Submission& s);
    void insertVersion(int submissionId, const SubmissionVersion& v);

    Database& db_;
};

}  // namespace lms
