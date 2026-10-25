// HodMenu.cpp
#include "presentation/HodMenu.h"

#include <chrono>
#include <iostream>

#include "domain/Exceptions.h"
#include "domain/Time.h"
#include "presentation/ConsoleIO.h"
#include "presentation/LecturerMenu.h"

namespace lms {

HodMenu::HodMenu(HeadOfDepartment& hod, AuthService& auth, ModerationService& moderationService,
                 AssignmentService& assignments, MarkingService& marking)
    : RoleMenu(hod, auth),
      hod_(hod),
      auth_(auth),
      moderation(moderationService),
      assignments_(assignments),
      marking_(marking) {}

void HodMenu::showOptions() const {
    io::heading("Head of Department menu - " + user.fullName());
    if (hod_.departmentId() == 0) {
        io::info("  (You have not been made HoD of a department yet - ask the administrator.)");
    }
    std::cout << "  1. Assignments in my department\n"
              << "  2. Moderate marks for an assignment\n"
              << "  3. Release marks for an assignment\n"
              << "  4. View the audit log for a submission\n"
              << "  5. Open my lecturer menu (my own teaching; exit it to come back here)\n";
}

bool HodMenu::handle(int choice) {
    switch (choice) {
        case 1: listDepartmentAssignments(); return true;
        case 2: moderate(); return true;
        case 3: releaseMarks(); return true;
        case 4: viewAuditLog(); return true;
        case 5: openLecturerMenu(); return true;
        default: return false;
    }
}

void HodMenu::listDepartmentAssignments() {
    io::heading("Assignments in my department");
    const auto list = moderation.listAssignmentsInDepartment(hod_);
    if (list.empty()) {
        io::info("There are no assignments in your department yet.");
        return;
    }
    std::cout << "  " << io::col("ID", 5) << io::col("Title", 28) << io::col("Deadline", 18)
              << "Released\n";
    for (const Assignment& a : list) {
        std::cout << "  " << io::col(std::to_string(a.id()), 5) << io::col(a.title(), 28)
                  << io::col(formatTime(a.deadline()), 18) << (a.releasedAt().has_value() ? "Yes" : "No") << '\n';
    }
}

void HodMenu::printRows(const std::vector<MarkView>& rows) const {
    std::cout << "  " << io::col("Sub", 5) << io::col("Student", 9) << io::col("State", 11)
              << io::col("Late", 6) << io::col("Raw", 5) << io::col("Pen.", 6) << io::col("Final", 7)
              << "Extension\n";
    for (const MarkView& v : rows) {
        std::cout << "  " << io::col(std::to_string(v.submission.id()), 5)
                  << io::col(std::to_string(v.submission.studentId()), 9)
                  << io::col(statusName(v.submission.status()), 11)
                  << io::col(v.daysLate > 0 ? std::to_string(v.daysLate) + "d" : "-", 6)
                  << io::col(v.mark ? std::to_string(v.mark->rawMark()) : "-", 5)
                  << io::col(v.mark ? io::oneDecimal(v.penalty) : "-", 6)
                  << io::col(v.mark ? io::oneDecimal(v.finalMark) : "-", 7)
                  << (v.extension ? "to " + formatTime(v.extension->newDeadline()) + " (" +
                                        v.extension->reason() + ")"
                                  : "-")
                  << '\n';
        if (v.mark) {
            std::cout << "        Feedback: " << v.mark->feedback() << '\n';
        }
    }
}

void HodMenu::moderate() {
    io::heading("Moderate marks");
    const int assignmentId = io::readInt("Assignment ID: ", 1, 1000000000);
    while (true) {
        const auto rows = moderation.listForModeration(hod_, assignmentId);
        if (rows.empty()) {
            io::info("No work has been handed in for this assignment.");
            return;
        }
        printRows(rows);
        int waiting = 0;
        bool allModerated = true;
        for (const MarkView& v : rows) {
            if (v.submission.status() == SubmissionStatus::Marked) ++waiting;
            if (v.submission.status() != SubmissionStatus::Moderated) allModerated = false;
        }
        if (allModerated) {
            io::info("Every mark is approved. You can now release them (option 3).");
            return;
        }
        if (waiting == 0) {
            io::info("Nothing is waiting for you - the rest is still with the lecturer.");
            return;
        }
        const auto sub = io::readOptionalInt("Submission to review (blank to stop): ", 1, 1000000000);
        if (!sub) {
            return;
        }
        std::cout << "  1. Approve\n  2. Adjust the mark\n  3. Return to the lecturer\n  0. Cancel\n";
        const int action = io::readInt("Action: ", 0, 3);
        const auto now = std::chrono::system_clock::now();
        try {
            if (action == 1) {
                moderation.approve(hod_, *sub, now);
                io::info("Approved.");
            } else if (action == 2) {
                const int raw = io::readInt("New raw mark: ", 0, 100);
                const std::string reason = io::readNonEmpty("Reason for the change: ");
                const double finalMark = moderation.adjust(hod_, *sub, raw, reason, now);
                io::info("Adjusted and approved. New final mark: " + io::oneDecimal(finalMark));
            } else if (action == 3) {
                const std::string comment = io::readNonEmpty("Comment for the lecturer: ");
                moderation.returnForRemarking(hod_, *sub, comment, now);
                io::info("Returned to the lecturer.");
            }
        } catch (const DatabaseException&) {
            throw;  // handled by RoleMenu::run
        } catch (const LmsException& e) {
            io::error(e.what());  // stay in the moderation loop
        }
    }
}

void HodMenu::releaseMarks() {
    io::heading("Release marks");
    const int assignmentId = io::readInt("Assignment ID: ", 1, 1000000000);
    if (!io::readYesNo("Students will see these marks straight away. Release now?")) {
        io::info("Nothing was released.");
        return;
    }
    moderation.release(hod_, assignmentId, std::chrono::system_clock::now());
    io::info("Marks released.");
}

void HodMenu::viewAuditLog() {
    io::heading("Audit log");
    const int sub = io::readInt("Submission ID: ", 1, 1000000000);
    const auto entries = moderation.viewAuditLog(hod_, sub);
    if (entries.empty()) {
        io::info("There are no mark changes for this submission yet.");
        return;
    }
    std::cout << "  " << io::col("When", 18) << io::col("Who", 10) << io::col("Action", 12)
              << io::col("Old", 5) << io::col("New", 5) << "Reason\n";
    for (const AuditEntry& e : entries) {
        std::cout << "  " << io::col(formatTime(e.changedAt()), 18) << io::col("User " + std::to_string(e.changedBy()), 10)
                  << io::col(e.action(), 12)
                  << io::col(e.oldMark() ? std::to_string(*e.oldMark()) : "-", 5)
                  << io::col(e.newMark() ? std::to_string(*e.newMark()) : "-", 5)
                  << e.reason().value_or("") << '\n';
    }
}

void HodMenu::openLecturerMenu() {
    // Liskov substitution: a HeadOfDepartment is used where a Lecturer is expected.
    LecturerMenu menu(hod_, auth_, assignments_, marking_);
    menu.run();
}

}  // namespace lms
