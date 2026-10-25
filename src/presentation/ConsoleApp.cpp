// ConsoleApp.cpp
#include "presentation/ConsoleApp.h"

#include <chrono>
#include <iostream>

#include "domain/Exceptions.h"
#include "domain/UserFactory.h"
#include "presentation/AdminMenu.h"
#include "presentation/ConsoleIO.h"
#include "presentation/HodMenu.h"
#include "presentation/LecturerMenu.h"
#include "presentation/StudentMenu.h"

namespace lms {

ConsoleApp::ConsoleApp(Services services, IUserRepository& users)
    : services_(services), users_(users) {}

int ConsoleApp::run() {
    try {
        std::cout << "Moderated Learning Management System v1.0\n";
        if (users_.listAll().empty()) {
            firstRunSetup();
        }
        while (true) {
            io::heading("Log in");
            const std::string username = io::readLine("Username (blank to quit): ");
            if (username.find_first_not_of(" \t") == std::string::npos) {
                std::cout << "Goodbye.\n";
                return 0;
            }
            const std::string password = io::readPassword("Password: ");
            std::unique_ptr<User> user;
            try {
                user = services_.auth.login(username, password, std::chrono::system_clock::now());
            } catch (const DatabaseException& e) {
                io::error(std::string("Database problem: ") + e.what());
                continue;
            } catch (const LmsException& e) {
                io::error(e.what());
                continue;
            }
            openMenuFor(*user);
            std::cout << "You have been logged out.\n";
        }
    } catch (const io::InputClosed&) {
        std::cout << "\nInput closed. Goodbye.\n";
        return 0;
    }
}

// FR-01: each role only ever sees its own menu.
void ConsoleApp::openMenuFor(User& user) {
    switch (user.role()) {
        case Role::Student: {
            StudentMenu menu(static_cast<Student&>(user), services_.auth, services_.submissions,
                             services_.assignments);
            menu.run();
            break;
        }
        case Role::Lecturer: {
            LecturerMenu menu(static_cast<Lecturer&>(user), services_.auth, services_.assignments,
                              services_.marking);
            menu.run();
            break;
        }
        case Role::HeadOfDepartment: {
            HodMenu menu(static_cast<HeadOfDepartment&>(user), services_.auth, services_.moderation,
                         services_.assignments, services_.marking);
            menu.run();
            break;
        }
        case Role::Administrator: {
            AdminMenu menu(static_cast<Administrator&>(user), services_.auth, services_.admin,
                           services_.moderation);
            menu.run();
            break;
        }
    }
}

// The very first time the program runs there are no users, so the technician creates
// the administrator account here (see the installation guide).
void ConsoleApp::firstRunSetup() {
    io::heading("First-time setup");
    io::info("There are no accounts yet. Create the administrator account now.");
    const std::string username = io::readNonEmpty("Administrator username: ");
    const std::string fullName = io::readNonEmpty("Full name: ");
    while (true) {
        const std::string pw = io::readPassword("Password (at least 8 characters): ");
        const std::string again = io::readPassword("Type it again: ");
        if (pw != again) {
            io::error("The passwords are different.");
            continue;
        }
        std::string hash;
        try {
            hash = AuthService::hashPassword(pw);  // also checks the 8-character rule
        } catch (const ValidationException& e) {
            io::error(e.what());
            continue;
        }
        auto admin = UserFactory::create(Role::Administrator, 0, username, fullName, hash);
        users_.add(*admin);
        io::info("Administrator account created. Please log in.");
        return;
    }
}

}  // namespace lms
