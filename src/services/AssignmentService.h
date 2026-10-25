// AssignmentService.h - create assignments, grant extensions, list a student's work
// (UC-06, UC-07, UC-16).
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "data/Repositories.h"
#include "domain/User.h"

namespace lms {

// One line of the student's assignment list (FR-09).
struct StudentAssignmentRow {
    Assignment assignment;
    std::string courseCode;
    TimePoint effectiveDeadline;
    bool hasExtension = false;
    std::optional<SubmissionStatus> status;  // nullopt = not submitted yet
};

class AssignmentService {
public:
    AssignmentService(ICourseRepository& courses, ISubmissionRepository& submissions);

    int createAssignment(const Lecturer& l, int courseId, const std::string& title,
                         TimePoint deadline, int maxMark);
    void grantExtension(const Lecturer& l, int assignmentId, int studentId,
                        TimePoint newDeadline, const std::string& reason, TimePoint now);
    std::vector<Assignment> listForStudent(const Student& s);

    // Added in Phase 4 for the list screens.
    std::vector<StudentAssignmentRow> overviewForStudent(const Student& s);
    std::vector<Course> listCoursesForStudent(const Student& s);
    std::vector<Course> listMyCourses(const Lecturer& l);
    std::vector<Assignment> listForCourse(const Lecturer& l, int courseId);

private:
    Assignment getAssignmentITeach(const Lecturer& l, int assignmentId);

    ICourseRepository& courses_;
    ISubmissionRepository& submissions_;
};

}  // namespace lms
