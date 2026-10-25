// RoleMenu.cpp
#include "presentation/RoleMenu.h"

#include <iostream>

#include "domain/Exceptions.h"
#include "presentation/ConsoleIO.h"

namespace lms {

RoleMenu::RoleMenu(User& u, AuthService& auth) : user(u), auth_(auth) {}

void RoleMenu::run() {
    while (true) {
        showOptions();
        std::cout << "  P. Change my password\n"
                  << "  0. Exit this menu\n";
        const std::string choice = io::readLine("Choose an option: ");
        if (choice == "0") {
            return;
        }
        try {
            if (choice == "p" || choice == "P") {
                changePassword();
                continue;
            }
            int number = 0;
            try {
                std::size_t used = 0;
                number = std::stoi(choice, &used);
                if (used != choice.size()) number = -1;
            } catch (const std::exception&) {
                number = -1;
            }
            if (number <= 0 || !handle(number)) {
                io::error("That is not one of the options.");
            }
        } catch (const DatabaseException& e) {
            io::error("The database action failed - nothing was changed. Please try again.");
            io::info("(Details: " + std::string(e.what()) + ")");
        } catch (const LmsException& e) {
            io::error(e.what());
        }
    }
}

void RoleMenu::changePassword() {
    io::heading("Change password");
    const std::string oldPw = io::readPassword("Current password: ");
    const std::string newPw = io::readPassword("New password (at least 8 characters): ");
    const std::string again = io::readPassword("Type the new password again: ");
    if (newPw != again) {
        io::error("The two new passwords are different. Nothing was changed.");
        return;
    }
    auth_.changePassword(user, oldPw, newPw);
    io::info("Your password has been changed.");
}

}  // namespace lms
