// AuthService.h - log in, lock-out and passwords (UC-01, UC-02).
// This is the only class that uses libsodium (component diagram, SDD Figure 1).
#pragma once

#include <memory>
#include <string>

#include "data/Repositories.h"
#include "domain/User.h"

namespace lms {

class AuthService {
public:
    explicit AuthService(IUserRepository& users);

    // Returns the logged-in user (the right subclass, via UserFactory).
    // Throws ValidationException / AccessDeniedException on failure.
    std::unique_ptr<User> login(const std::string& username, const std::string& password,
                                TimePoint now);

    void changePassword(User& u, const std::string& oldPw, const std::string& newPw);

    // Checks the password is at least 8 characters (FR-26), then returns a salted Argon2id
    // hash (NFR-04). Static so AdminService::createUser can hash a new user's first password
    // without using libsodium itself (see Implementation Report, section 9).
    static std::string hashPassword(const std::string& password);

private:
    IUserRepository& users_;
};

}  // namespace lms
