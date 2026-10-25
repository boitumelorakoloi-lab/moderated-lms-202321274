// StudentMenu.cpp
#include "presentation/StudentMenu.h"

#include <chrono>
#include <iostream>

#include "domain/Time.h"
#include "presentation/ConsoleIO.h"

namespace lms {

StudentMenu::StudentMenu(Student& student, AuthService& auth, SubmissionService& submissionService,
                         AssignmentService& assignmentService)
    : RoleMenu(student, auth),
      student_(student),
      submissions(submissionService),
      assignments(assignmentService) {}

void StudentMenu::showOptions() const {
    io::heading("Student menu - " + user.fullName());
    std::cout << "  1. My courses and assignments\n"
              << "  2. Submit an assignment\n"
              << "  3. My released marks and feedback\n";
}

bool StudentMenu::handle(int choice) {
    switch (choice) {
        case 1: viewCoursesAndAssignments(); return true;
        case 2: submitAssignment(); return true;
        case 3: viewReleasedMarks(); return true;
        default: return false;
    }
}

void StudentMenu::viewCoursesAndAssignments() {
    io::heading("My courses");
    const auto courses = assignments.listCoursesForStudent(student_);
    if (courses.empty()) {
        io::info("You are not enrolled in any course yet.");
        return;
    }
    for (const Course& c : courses) {
        std::cout << "  " << io::col(c.code(), 12) << c.title() << '\n';
    }

    io::heading("My assignments");
    const auto rows = assignments.overviewForStudent(student_);
    if (rows.empty()) {
        io::info("No assignments yet.");
        return;
    }
    std::cout << "  " << io::col("ID", 5) << io::col("Course", 10) << io::col("Title", 26)
              << io::col("Deadline", 20) << io::col("Max", 5) << "State\n";
    bool anyExtension = false;
    for (const auto& r : rows) {
        anyExtension = anyExtension || r.hasExtension;
        std::string deadline = formatTime(r.effectiveDeadline) + (r.hasExtension ? " *" : "");
        std::cout << "  " << io::col(std::to_string(r.assignment.id()), 5)
                  << io::col(r.courseCode, 10) << io::col(r.assignment.title(), 26)
                  << io::col(deadline, 20) << io::col(std::to_string(r.assignment.maxMark()), 5)
                  << (r.status ? statusName(*r.status) : "Not submitted") << '\n';
    }
    if (anyExtension) {
        io::info("  (* = your personal extended deadline)");
    }
}

void StudentMenu::submitAssignment() {
    io::heading("Submit an assignment");
    const int assignmentId = io::readInt("Assignment ID: ", 1, 1000000000);
    const std::string path = io::readNonEmpty("Path of your file (.pdf, .docx, .txt or .zip, max 10 MB): ");

    const CheckResult r =
        submissions.check(student_, assignmentId, path, std::chrono::system_clock::now());
    io::info("Your deadline: " + formatTime(r.deadline));
    if (r.result == SubmissionCheck::Rejected) {
        io::error(r.message);
        return;
    }
    if (r.result == SubmissionCheck::Late) {
        io::info("WARNING: " + r.message);
    } else {
        io::info(r.message);
    }
    if (!io::readYesNo("Hand in this file now?")) {
        io::info("Nothing was submitted.");
        return;
    }
    const auto now = std::chrono::system_clock::now();
    const int id = submissions.submit(student_, assignmentId, path, now);
    io::info("Receipt: submission " + std::to_string(id) + ", version " +
             std::to_string(r.nextVersion) + ", handed in at " + formatTime(now) + ".");
}

void StudentMenu::viewReleasedMarks() {
    io::heading("My released marks");
    const auto results = submissions.myResults(student_);
    if (results.empty()) {
        io::info("No marks have been released to you yet.");
        return;
    }
    for (const auto& r : results) {
        const MarkView& v = r.view;
        std::cout << "\n  " << r.assignment.title() << '\n';
        if (!v.mark) continue;
        std::cout << "    Raw mark:    " << v.mark->rawMark() << " / " << v.maxMark << '\n';
        if (v.daysLate > 0) {
            std::cout << "    Days late:   " << v.daysLate << " (penalty "
                      << io::oneDecimal(v.penalty) << ")\n";
        }
        std::cout << "    Final mark:  " << io::oneDecimal(v.finalMark) << " / " << v.maxMark << '\n'
                  << "    Feedback:    " << v.mark->feedback() << '\n';
    }
}

}  // namespace lms
