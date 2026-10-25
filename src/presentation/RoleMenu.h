// RoleMenu.h - abstract base for the four role menus (Template Method: run() is shared,
// showOptions() and handle() are different for each role).
#pragma once

#include "domain/User.h"
#include "services/AuthService.h"

namespace lms {

class RoleMenu {
public:
    RoleMenu(User& user, AuthService& auth);
    virtual ~RoleMenu() = default;

    // Shows the menu until the user logs out. Catches every LmsException so one bad
    // action never ends the session (NFR-07).
    void run();

protected:
    virtual void showOptions() const = 0;
    virtual bool handle(int choice) = 0;  // false = not a valid option

    User& user;

private:
    void changePassword();

    AuthService& auth_;
};

}  // namespace lms
