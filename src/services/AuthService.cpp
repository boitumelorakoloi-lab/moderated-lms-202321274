// AuthService.cpp
#include "services/AuthService.h"

#include <sodium.h>

#include "domain/Exceptions.h"
#include "domain/Time.h"
#include "services/MarkView.h"

namespace lms {

namespace {
void ensureSodium() {
    // sodium_init() is safe to call many times; it returns -1 only if it cannot start.
    if (sodium_init() < 0) {
        throw LmsException("The password library (libsodium) could not start.");
    }
}

const char* const kWrongLogin = "Wrong username or password.";
const std::size_t kMinPasswordLength = 8;  // FR-26

bool verifyPassword(const std::string& hash, const std::string& password) {
    ensureSodium();
    return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.size()) == 0;
}
}  // namespace

AuthService::AuthService(IUserRepository& users) : users_(users) {
    ensureSodium();
}

std::unique_ptr<User> AuthService::login(const std::string& username, const std::string& password,
                                         TimePoint now) {
    auto user = users_.findByUsername(trim(username));
    if (!user) {
        throw ValidationException(kWrongLogin);  // same message, so usernames cannot be guessed
    }
    if (user->isLocked(now)) {
        throw AccessDeniedException("This account is locked after " +
                                    std::to_string(User::kMaxFailedAttempts) +
                                    " wrong passwords. Try again after " +
                                    formatTime(*user->lockedUntil()) + ".");
    }
    if (!verifyPassword(user->passwordHash(), password)) {
        user->recordFailedLogin(now);
        users_.update(*user);
        if (user->isLocked(now)) {
            throw AccessDeniedException("Too many wrong passwords. The account is locked for 15 minutes.");
        }
        throw ValidationException(kWrongLogin);
    }
    if (!user->isActive()) {
        throw AccessDeniedException("This account has been deactivated. Please see the administrator.");
    }
    user->setLoginState(0, std::nullopt);  // a good login clears the count
    users_.update(*user);
    return user;
}

void AuthService::changePassword(User& u, const std::string& oldPw, const std::string& newPw) {
    if (!verifyPassword(u.passwordHash(), oldPw)) {
        throw ValidationException("Your current password is not correct.");
    }
    u.setPasswordHash(hashPassword(newPw));
    users_.update(u);
}

std::string AuthService::hashPassword(const std::string& password) {
    if (password.size() < kMinPasswordLength) {
        throw ValidationException("The password must be at least " +
                                  std::to_string(kMinPasswordLength) + " characters long.");
    }
    ensureSodium();
    char hash[crypto_pwhash_STRBYTES];
    // crypto_pwhash_str uses Argon2id and makes a new random salt every time.
    // The salt and settings are stored inside the returned string.
    if (crypto_pwhash_str(hash, password.c_str(), password.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw LmsException("Not enough memory to hash the password.");
    }
    return std::string(hash);
}

}  // namespace lms
