// SqliteMarkRepository.h - mark and audit_entry tables.
#pragma once

#include "data/Database.h"
#include "data/Repositories.h"

namespace lms {

class SqliteMarkRepository final : public IMarkRepository {
public:
    explicit SqliteMarkRepository(Database& db) : db_(db) {}

    std::optional<Mark> find(int submissionId) override;
    void saveChange(const Mark& m, SubmissionStatus st, const AuditEntry& e) override;
    std::vector<AuditEntry> listAudit(int submissionId) override;

private:
    Database& db_;
};

}  // namespace lms
