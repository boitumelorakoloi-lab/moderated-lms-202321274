// main.cpp - builds the objects and wires them together (dependency inversion, SDD 3.3):
// this is the only place that knows the services use SQLite and the 10%-per-day rule.
#include <exception>
#include <iostream>
#include <string>

#include "data/Database.h"
#include "data/SqliteCourseRepository.h"
#include "data/SqliteMarkRepository.h"
#include "data/SqliteSubmissionRepository.h"
#include "data/SqliteUserRepository.h"
#include "domain/TenPercentPerDayPolicy.h"
#include "presentation/ConsoleApp.h"
#include "presentation/DemoData.h"

namespace {
void printHelp() {
    std::cout << "Usage: lms [--db FILE] [--files FOLDER] [--seed-demo]\n"
              << "  --db FILE        database file (default: lms.db)\n"
              << "  --files FOLDER   where submitted files are kept (default: submissions)\n"
              << "  --seed-demo      fill an empty database with demo accounts and courses\n";
}
}  // namespace

int main(int argc, char* argv[]) {
    std::string dbPath = "lms.db";
    std::string filesFolder = "submissions";
    bool seedDemo = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--db" && i + 1 < argc) {
            dbPath = argv[++i];
        } else if (arg == "--files" && i + 1 < argc) {
            filesFolder = argv[++i];
        } else if (arg == "--seed-demo") {
            seedDemo = true;
        } else if (arg == "--help" || arg == "-h") {
            printHelp();
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printHelp();
            return 2;
        }
    }

    try {
        lms::Database db(dbPath);
        lms::SqliteUserRepository users(db);
        lms::SqliteCourseRepository courses(db);
        lms::SqliteSubmissionRepository submissions(db);
        lms::SqliteMarkRepository marks(db);
        lms::TenPercentPerDayPolicy policy;

        lms::AuthService auth(users);
        lms::AdminService admin(users, courses, db);
        lms::AssignmentService assignmentService(courses, submissions);
        lms::SubmissionService submissionService(courses, submissions, marks, policy, filesFolder);
        lms::MarkingService marking(courses, submissions, marks, policy);
        lms::ModerationService moderation(courses, submissions, marks, policy);

        lms::Services services{auth, admin, assignmentService, submissionService, marking, moderation};

        if (seedDemo && !lms::DemoData::seed(services, users, "demo_files")) {
            std::cout << "The database already has accounts, so no demo data was added.\n";
        }

        lms::ConsoleApp app(services, users);
        return app.run();
    } catch (const std::exception& e) {
        // Last safety net: report the problem instead of crashing (NFR-07).
        std::cerr << "The program could not continue: " << e.what() << "\n";
        return 1;
    }
}
