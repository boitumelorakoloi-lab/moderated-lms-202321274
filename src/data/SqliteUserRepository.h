// SqliteUserRepository.h
#pragma once

#include "data/Database.h"
#include "data/Repositories.h"

namespace lms {

class SqliteUserRepository final : public IUserRepository {
public:
    explicit SqliteUserRepository(Database& db) : db_(db) {}

    std::unique_ptr<User> findByUsername(const std::string& name) override;
    std::unique_ptr<User> findById(int id) override;
    std::vector<std::unique_ptr<User>> listAll() override;
    int add(const User& u) override;
    void update(const User& u) override;

private:
    Database& db_;
};

}  // namespace lms
