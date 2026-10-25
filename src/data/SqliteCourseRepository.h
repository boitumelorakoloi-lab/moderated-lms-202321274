// SqliteCourseRepository.h - departments, courses, enrolments, lecturers, assignments, extensions.
#pragma once

#include "data/Database.h"
#include "data/Repositories.h"

namespace lms {

class SqliteCourseRepository final : public ICourseRepository {
public:
    explicit SqliteCourseRepository(Database& db) : db_(db) {}

    std::optional<Course> findCourse(int id) override;
    std::optional<Assignment> findAssignment(int id) override;
    std::optional<Extension> findExtension(int assignmentId, int studentId) override;
    bool isEnrolled(int studentId, int courseId) override;
    bool teaches(int lecturerId, int courseId) override;
    std::vector<Assignment> listAssignmentsFor(int studentId) override;
    int addCourse(const Course& c) override;
    int addAssignment(const Assignment& a) override;
    void saveExtension(const Extension& e) override;
    void enrol(int studentId, int courseId) override;
    void assignLecturer(int lecturerId, int courseId) override;

    int addDepartment(const Department& d) override;
    void setDepartmentHod(int departmentId, int hodUserId) override;
    std::optional<Department> findDepartment(int id) override;
    std::vector<Department> listDepartments() override;
    std::vector<Course> listCourses() override;
    std::vector<Course> listCoursesForLecturer(int lecturerId) override;
    std::vector<Course> listCoursesForStudent(int studentId) override;
    std::vector<Assignment> listAssignmentsForCourse(int courseId) override;

private:
    Database& db_;
};

}  // namespace lms
