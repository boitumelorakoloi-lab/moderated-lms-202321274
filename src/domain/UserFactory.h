// UserFactory.h - Factory pattern: the one place that decides which User subclass to build.
#pragma once

#include <memory>
#include <string>

#include "domain/User.h"

namespace lms {

class UserFactory {
public:
    static std::unique_ptr<User> create(Role role, int id, const std::string& username,
                                        const std::string& fullName, const std::string& hash,
                                        int deptId = 0);
};

}  // namespace lms
