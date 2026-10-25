// ConsoleApp.h - the log-in screen. After log-in it opens the menu for the user's role.
#pragma once

#include "data/Repositories.h"
#include "services/AdminService.h"
#include "services/AssignmentService.h"
#include "services/AuthService.h"
#include "services/MarkingService.h"
#include "services/ModerationService.h"
#include "services/SubmissionService.h"

namespace lms {

struct Services {
    AuthService& auth;
    AdminService& admin;
    AssignmentService& assignments;
    SubmissionService& submissions;
    MarkingService& marking;
    ModerationService& moderation;
};

class ConsoleApp {
public:
    ConsoleApp(Services services, IUserRepository& users);
    int run();

private:
    void firstRunSetup();
    void openMenuFor(User& user);

    Services services_;
    IUserRepository& users_;
};

}  // namespace lms
