// StudentMenu.h
#pragma once

#include "presentation/RoleMenu.h"
#include "services/AssignmentService.h"
#include "services/SubmissionService.h"

namespace lms {

class StudentMenu final : public RoleMenu {
public:
    StudentMenu(Student& student, AuthService& auth, SubmissionService& submissions,
                AssignmentService& assignments);

protected:
    void showOptions() const override;
    bool handle(int choice) override;

private:
    void viewCoursesAndAssignments();                                            // UC-16
    static void printExtensions(const std::vector<StudentAssignmentRow>& rows);  // CR1
    void submitAssignment();                                                     // UC-17 to UC-20
    void viewReleasedMarks();                                                    // UC-21

    Student& student_;
    SubmissionService& submissions;
    AssignmentService& assignments;
};

}  // namespace lms
