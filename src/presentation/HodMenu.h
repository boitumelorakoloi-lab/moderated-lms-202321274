// HodMenu.h - an HoD is also a lecturer, so this menu can open the lecturer menu too.
#pragma once

#include "presentation/RoleMenu.h"
#include "services/AssignmentService.h"
#include "services/MarkingService.h"
#include "services/ModerationService.h"

namespace lms {

class HodMenu final : public RoleMenu {
public:
    HodMenu(HeadOfDepartment& hod, AuthService& auth, ModerationService& moderation,
            AssignmentService& assignments, MarkingService& marking);

protected:
    void showOptions() const override;
    bool handle(int choice) override;

private:
    void listDepartmentAssignments();
    void moderate();         // UC-11, UC-12, UC-13
    void releaseMarks();     // UC-14
    void viewAuditLog();     // UC-15
    void openLecturerMenu();
    void printRows(const std::vector<MarkView>& rows) const;

    HeadOfDepartment& hod_;
    AuthService& auth_;
    ModerationService& moderation;
    AssignmentService& assignments_;
    MarkingService& marking_;
};

}  // namespace lms
