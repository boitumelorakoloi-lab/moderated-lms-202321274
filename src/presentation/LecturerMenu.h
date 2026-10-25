// LecturerMenu.h
#pragma once

#include "presentation/RoleMenu.h"
#include "services/AssignmentService.h"
#include "services/MarkingService.h"

namespace lms {

class LecturerMenu final : public RoleMenu {
public:
    LecturerMenu(Lecturer& lecturer, AuthService& auth, AssignmentService& assignments,
                 MarkingService& marking);

protected:
    void showOptions() const override;
    bool handle(int choice) override;

private:
    void listMyCourses();
    void createAssignment();      // UC-06
    void listAssignments();
    void grantExtension();        // UC-07
    void markSubmissions();       // UC-08

    Lecturer& lecturer_;
    AssignmentService& assignments;
    MarkingService& marking;
};

}  // namespace lms
