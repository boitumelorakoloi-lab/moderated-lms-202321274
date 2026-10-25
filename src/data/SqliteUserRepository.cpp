// SqliteUserRepository.cpp - app_user table (single-table inheritance, SDD 5.5).
#include "data/SqliteUserRepository.h"

#include <chrono>

#include "domain/UserFactory.h"

namespace lms {

namespace {
// The HoD's department is stored once, as department.hod_user_id (SDD 5.5),
// so it is joined in here rather than kept in app_user.
const char* const kSelectUser =
    "SELECT u.user_id, u.username, u.full_name, u.password_hash, u.role, u.is_active, "
    "       u.failed_attempts, u.locked_until, d.department_id "
    "FROM app_user u LEFT JOIN department d ON d.hod_user_id = u.user_id ";

std::unique_ptr<User> readUser(const Statement& st) {
    auto user = UserFactory::create(roleFromCode(st.getText(4)), st.getInt(0), st.getText(1),
                                    st.getText(2), st.getText(3),
                                    st.isNull(8) ? 0 : st.getInt(8));
    user->setActive(st.getInt(5) == 1);
    user->setLoginState(st.getInt(6), st.getOptionalTime(7));
    return user;
}
}  // namespace

std::unique_ptr<User> SqliteUserRepository::findByUsername(const std::string& name) {
    Statement st(db_, (std::string(kSelectUser) + "WHERE u.username = ?1").c_str());
    st.bind(1, name);
    return st.step() ? readUser(st) : nullptr;
}

std::unique_ptr<User> SqliteUserRepository::findById(int id) {
    Statement st(db_, (std::string(kSelectUser) + "WHERE u.user_id = ?1").c_str());
    st.bind(1, id);
    return st.step() ? readUser(st) : nullptr;
}

std::vector<std::unique_ptr<User>> SqliteUserRepository::listAll() {
    Statement st(db_, (std::string(kSelectUser) + "ORDER BY u.role, u.username").c_str());
    std::vector<std::unique_ptr<User>> users;
    while (st.step()) {
        users.push_back(readUser(st));
    }
    return users;
}

int SqliteUserRepository::add(const User& u) {
    Statement st(db_,
                 "INSERT INTO app_user (username, full_name, password_hash, role, is_active, "
                 "failed_attempts, locked_until, created_at) "
                 "VALUES (?1, ?2, ?3, ?4, ?5, 0, NULL, ?6)");
    st.bind(1, u.username())
        .bind(2, u.fullName())
        .bind(3, u.passwordHash())
        .bind(4, roleCode(u.role()))
        .bind(5, u.isActive() ? 1 : 0)
        .bindTime(6, std::chrono::system_clock::now());
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteUserRepository::update(const User& u) {
    Statement st(db_,
                 "UPDATE app_user SET full_name = ?1, password_hash = ?2, is_active = ?3, "
                 "failed_attempts = ?4, locked_until = ?5 WHERE user_id = ?6");
    st.bind(1, u.fullName())
        .bind(2, u.passwordHash())
        .bind(3, u.isActive() ? 1 : 0)
        .bind(4, u.failedAttempts())
        .bindOptionalTime(5, u.lockedUntil())
        .bind(6, u.id());
    st.run();
}

}  // namespace lms
