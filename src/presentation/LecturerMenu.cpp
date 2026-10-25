// LecturerMenu.cpp
#include "presentation/LecturerMenu.h"

#include <chrono>
#include <iostream>

#include "domain/Time.h"
#include "presentation/ConsoleIO.h"

namespace lms {

LecturerMenu::LecturerMenu(Lecturer& lecturer, AuthService& auth, AssignmentService& assignmentService,
                           MarkingService& markingService)
    : RoleMenu(lecturer, auth),
      lecturer_(lecturer),
      assignments(assignmentService),
      marking(markingService) {}

void LecturerMenu::showOptions() const {
    io::heading("Lecturer menu - " + user.fullName());
    std::cout << "  1. My courses\n"
              << "  2. Create an assignment\n"
              << "  3. List assignments for a course\n"
              << "  4. Give a student an extension\n"
              << "  5. Mark submissions\n";
}

bool LecturerMenu::handle(int choice) {
    switch (choice) {
        case 1: listMyCourses(); return true;
        case 2: createAssignment(); return true;
        case 3: listAssignments(); return true;
        case 4: grantExtension(); return true;
        case 5: markSubmissions(); return true;
        default: return false;
    }
}

void LecturerMenu::listMyCourses() {
    io::heading("My courses");
    const auto courses = assignments.listMyCourses(lecturer_);
    if (courses.empty()) {
        io::info("You have not been given any courses yet.");
        return;
    }
    std::cout << "  " << io::col("ID", 5) << io::col("Code", 12) << "Title\n";
    for (const Course& c : courses) {
        std::cout << "  " << io::col(std::to_string(c.id()), 5) << io::col(c.code(), 12) << c.title()
                  << '\n';
    }
}

void LecturerMenu::createAssignment() {
    io::heading("Create an assignment");
    const int courseId = io::readInt("Course ID: ", 1, 1000000000);
    const std::string title = io::readNonEmpty("Title: ");
    const TimePoint deadline = io::readDateTime("Deadline");
    const int maxMark = io::readInt("Maximum mark (1-100): ", 1, 100);
    const int id = assignments.createAssignment(lecturer_, courseId, title, deadline, maxMark);
    io::info("Assignment " + std::to_string(id) + " created.");
}

void LecturerMenu::listAssignments() {
    io::heading("Assignments");
    const int courseId = io::readInt("Course ID: ", 1, 1000000000);
    const auto list = assignments.listForCourse(lecturer_, courseId);
    if (list.empty()) {
        io::info("No assignments for this course yet.");
        return;
    }
    std::cout << "  " << io::col("ID", 5) << io::col("Title", 28) << io::col("Deadline", 18)
              << io::col("Max", 5) << "Released\n";
    for (const Assignment& a : list) {
        std::cout << "  " << io::col(std::to_string(a.id()), 5) << io::col(a.title(), 28)
                  << io::col(formatTime(a.deadline()), 18) << io::col(std::to_string(a.maxMark()), 5)
                  << (a.releasedAt().has_value() ? "Yes" : "No") << '\n';
    }
}

void LecturerMenu::grantExtension() {
    io::heading("Give an extension");
    const int assignmentId = io::readInt("Assignment ID: ", 1, 1000000000);
    const int studentId = io::readInt("Student ID: ", 1, 1000000000);
    const TimePoint newDeadline = io::readDateTime("New deadline for this student");
    const std::string reason = io::readNonEmpty("Reason: ");
    assignments.grantExtension(lecturer_, assignmentId, studentId, newDeadline, reason,
                               std::chrono::system_clock::now());
    io::info("Extension saved. The student's deadline is now " + formatTime(newDeadline) + ".");
}

void LecturerMenu::markSubmissions() {
    io::heading("Mark submissions");
    const int assignmentId = io::readInt("Assignment ID: ", 1, 1000000000);
    const auto rows = marking.listToMark(lecturer_, assignmentId);
    if (rows.empty()) {
        io::info("There is no work to mark for this assignment.");
        return;
    }
    std::cout << "  " << io::col("Sub", 5) << io::col("Student", 9) << io::col("State", 11)
              << io::col("Late", 6) << io::col("Raw", 6) << io::col("Final", 7) << "File\n";
    for (const MarkView& v : rows) {
        std::cout << "  " << io::col(std::to_string(v.submission.id()), 5)
                  << io::col(std::to_string(v.submission.studentId()), 9)
                  << io::col(statusName(v.submission.status()), 11)
                  << io::col(v.daysLate > 0 ? std::to_string(v.daysLate) + "d" : "-", 6)
                  << io::col(v.mark ? std::to_string(v.mark->rawMark()) : "-", 6)
                  << io::col(v.mark ? io::oneDecimal(v.finalMark) : "-", 7)
                  << v.submission.latest().storedPath() << '\n';
        if (v.mark && v.mark->hodComment() && v.submission.status() == SubmissionStatus::Returned) {
            std::cout << "        HoD comment: " << *v.mark->hodComment() << '\n';
        }
    }

    const auto chosen = io::readOptionalInt("Submission to mark (blank to go back): ", 1, 1000000000);
    if (!chosen) {
        return;
    }
    const MarkView* picked = nullptr;
    for (const MarkView& v : rows) {
        if (v.submission.id() == *chosen) picked = &v;
    }
    if (picked == nullptr) {
        io::error("That submission is not in the list.");
        return;
    }
    const int raw = io::readInt("Raw mark (0-" + std::to_string(picked->maxMark) + "): ", 0,
                                picked->maxMark);
    const std::string feedback = io::readNonEmpty("Feedback: ");
    std::optional<std::string> reason;
    if (picked->mark) {
        reason = io::readNonEmpty("Reason for changing the mark: ");  // BR-05
    }
    if (picked->daysLate > 3) {
        io::info("Note: this work is more than 3 days late, so the final mark will be 0.");
    }
    if (!io::readYesNo("Save this mark?")) {
        io::info("Nothing was saved.");
        return;
    }
    const double finalMark = marking.mark(lecturer_, *chosen, raw, feedback, reason,
                                          std::chrono::system_clock::now());
    io::info("Saved. Final mark after any late penalty: " + io::oneDecimal(finalMark) + " / " +
             std::to_string(picked->maxMark) + ".");
}

}  // namespace lms
