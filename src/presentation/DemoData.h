// DemoData.h - fills an EMPTY database with sample users, courses and assignments so the
// system can be demonstrated (run: lms --seed-demo). Uses the real services, so all the
// normal checks still apply.
#pragma once

#include <string>

#include "presentation/ConsoleApp.h"

namespace lms {

class DemoData {
public:
    static constexpr const char* kPassword = "Password1";

    // Returns false (and changes nothing) if the database already has users.
    static bool seed(Services services, IUserRepository& users, const std::string& demoFolder);
};

}  // namespace lms
