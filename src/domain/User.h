// User.h - abstract User and its four role subclasses (SDD Figure 2, "Users").
// Polymorphism: every role answers role() and can() differently.
#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "domain/Types.h"

namespace lms {

class User {
public:
    static constexpr int kMaxFailedAttempts = 5;                     // FR-03
    static constexpr std::chrono::minutes kLockDuration{15};         // FR-03

    User(int id, std::string username, std::string fullName, std::string passwordHash);
    virtual ~User() = default;

    virtual Role role() const = 0;
    virtual bool can(Permission p) const = 0;

    bool isLocked(TimePoint now) const;
    void recordFailedLogin(TimePoint now);

    int id() const { return id_; }
    const std::string& username() const { return username_; }
    const std::string& fullName() const { return fullName_; }
    const std::string& passwordHash() const { return passwordHash_; }
    bool isActive() const { return active_; }
    int failedAttempts() const { return failedAttempts_; }
    const std::optional<TimePoint>& lockedUntil() const { return lockedUntil_; }

    void setId(int id) { id_ = id; }
    void setFullName(const std::string& name) { fullName_ = name; }
    void setPasswordHash(const std::string& hash) { passwordHash_ = hash; }
    void setActive(bool active) { active_ = active; }
    void setLoginState(int failedAttempts, std::optional<TimePoint> lockedUntil);

protected:
    int id_;
    std::string username_;
    std::string fullName_;
    std::string passwordHash_;
    bool active_ = true;
    int failedAttempts_ = 0;
    std::optional<TimePoint> lockedUntil_;
};

class Student final : public User {
public:
    using User::User;
    Role role() const override { return Role::Student; }
    bool can(Permission p) const override;
};

// Not final: HeadOfDepartment is a kind of Lecturer (use case generalisation).
class Lecturer : public User {
public:
    using User::User;
    Role role() const override { return Role::Lecturer; }
    bool can(Permission p) const override;
};

class HeadOfDepartment final : public Lecturer {
public:
    HeadOfDepartment(int id, std::string username, std::string fullName, std::string passwordHash,
                     int departmentId);
    Role role() const override { return Role::HeadOfDepartment; }
    bool can(Permission p) const override;
    int departmentId() const { return departmentId_; }  // 0 = not yet assigned

private:
    int departmentId_;
};

class Administrator final : public User {
public:
    using User::User;
    Role role() const override { return Role::Administrator; }
    bool can(Permission p) const override;
};

}  // namespace lms
