// SqliteCourseRepository.cpp
#include "data/SqliteCourseRepository.h"

#include <chrono>

namespace lms {

namespace {
const char* const kAssignmentColumns =
    "a.assignment_id, a.course_id, a.title, a.deadline, a.max_mark, a.created_by, a.released_at ";

Assignment readAssignment(const Statement& st) {
    return Assignment(st.getInt(0), st.getInt(1), st.getText(2), st.getTime(3), st.getInt(4),
                      st.getInt(5), st.getOptionalTime(6));
}

Course readCourse(const Statement& st) {
    return Course(st.getInt(0), st.getText(1), st.getText(2), st.getInt(3));
}

std::vector<Course> readCourses(Statement& st) {
    std::vector<Course> result;
    while (st.step()) {
        result.push_back(readCourse(st));
    }
    return result;
}

std::vector<Assignment> readAssignments(Statement& st) {
    std::vector<Assignment> result;
    while (st.step()) {
        result.push_back(readAssignment(st));
    }
    return result;
}
}  // namespace

std::optional<Course> SqliteCourseRepository::findCourse(int id) {
    Statement st(db_, "SELECT course_id, code, title, department_id FROM course WHERE course_id = ?1");
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    return readCourse(st);
}

std::optional<Assignment> SqliteCourseRepository::findAssignment(int id) {
    Statement st(db_, (std::string("SELECT ") + kAssignmentColumns +
                       "FROM assignment a WHERE a.assignment_id = ?1").c_str());
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    return readAssignment(st);
}

std::optional<Extension> SqliteCourseRepository::findExtension(int assignmentId, int studentId) {
    Statement st(db_,
                 "SELECT assignment_id, student_id, new_deadline, reason, granted_by "
                 "FROM extension WHERE assignment_id = ?1 AND student_id = ?2");
    st.bind(1, assignmentId).bind(2, studentId);
    if (!st.step()) return std::nullopt;
    return Extension(st.getInt(0), st.getInt(1), st.getTime(2), st.getText(3), st.getInt(4));
}

bool SqliteCourseRepository::isEnrolled(int studentId, int courseId) {
    Statement st(db_, "SELECT 1 FROM enrolment WHERE student_id = ?1 AND course_id = ?2");
    st.bind(1, studentId).bind(2, courseId);
    return st.step();
}

bool SqliteCourseRepository::teaches(int lecturerId, int courseId) {
    Statement st(db_, "SELECT 1 FROM course_lecturer WHERE lecturer_id = ?1 AND course_id = ?2");
    st.bind(1, lecturerId).bind(2, courseId);
    return st.step();
}

std::vector<Assignment> SqliteCourseRepository::listAssignmentsFor(int studentId) {
    Statement st(db_, (std::string("SELECT ") + kAssignmentColumns +
                       "FROM assignment a JOIN enrolment e ON e.course_id = a.course_id "
                       "WHERE e.student_id = ?1 ORDER BY a.deadline").c_str());
    st.bind(1, studentId);
    return readAssignments(st);
}

int SqliteCourseRepository::addCourse(const Course& c) {
    Statement st(db_, "INSERT INTO course (code, title, department_id) VALUES (?1, ?2, ?3)");
    st.bind(1, c.code()).bind(2, c.title()).bind(3, c.departmentId());
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

int SqliteCourseRepository::addAssignment(const Assignment& a) {
    Statement st(db_,
                 "INSERT INTO assignment (course_id, title, deadline, max_mark, created_by, "
                 "created_at) VALUES (?1, ?2, ?3, ?4, ?5, ?6)");
    st.bind(1, a.courseId())
        .bind(2, a.title())
        .bindTime(3, a.deadline())
        .bind(4, a.maxMark())
        .bind(5, a.createdBy())
        .bindTime(6, std::chrono::system_clock::now());
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteCourseRepository::saveExtension(const Extension& e) {
    // At most one extension per student per assignment: a new one replaces the old one.
    Statement st(db_,
                 "INSERT INTO extension (assignment_id, student_id, new_deadline, reason, "
                 "granted_by, granted_at) VALUES (?1, ?2, ?3, ?4, ?5, ?6) "
                 "ON CONFLICT (assignment_id, student_id) DO UPDATE SET "
                 "new_deadline = excluded.new_deadline, reason = excluded.reason, "
                 "granted_by = excluded.granted_by, granted_at = excluded.granted_at");
    st.bind(1, e.assignmentId())
        .bind(2, e.studentId())
        .bindTime(3, e.newDeadline())
        .bind(4, e.reason())
        .bind(5, e.grantedBy())
        .bindTime(6, std::chrono::system_clock::now());  // granted_at (not kept in the class)
    st.run();
}

void SqliteCourseRepository::enrol(int studentId, int courseId) {
    Statement st(db_, "INSERT INTO enrolment (course_id, student_id, enrolled_at) VALUES (?1, ?2, ?3)");
    st.bind(1, courseId).bind(2, studentId).bindTime(3, std::chrono::system_clock::now());
    st.run();
}

void SqliteCourseRepository::assignLecturer(int lecturerId, int courseId) {
    Statement st(db_, "INSERT INTO course_lecturer (course_id, lecturer_id) VALUES (?1, ?2)");
    st.bind(1, courseId).bind(2, lecturerId);
    st.run();
}

int SqliteCourseRepository::addDepartment(const Department& d) {
    Statement st(db_, "INSERT INTO department (name, hod_user_id) VALUES (?1, ?2)");
    st.bind(1, d.name()).bindOptional(2, d.hodUserId());
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteCourseRepository::setDepartmentHod(int departmentId, int hodUserId) {
    Statement st(db_, "UPDATE department SET hod_user_id = ?1 WHERE department_id = ?2");
    st.bind(1, hodUserId).bind(2, departmentId);
    st.run();
}

std::optional<Department> SqliteCourseRepository::findDepartment(int id) {
    Statement st(db_, "SELECT department_id, name, hod_user_id FROM department WHERE department_id = ?1");
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    return Department(st.getInt(0), st.getText(1), st.getOptionalInt(2));
}

std::vector<Department> SqliteCourseRepository::listDepartments() {
    Statement st(db_, "SELECT department_id, name, hod_user_id FROM department ORDER BY name");
    std::vector<Department> result;
    while (st.step()) {
        result.emplace_back(st.getInt(0), st.getText(1), st.getOptionalInt(2));
    }
    return result;
}

std::vector<Course> SqliteCourseRepository::listCourses() {
    Statement st(db_, "SELECT course_id, code, title, department_id FROM course ORDER BY code");
    return readCourses(st);
}

std::vector<Course> SqliteCourseRepository::listCoursesForLecturer(int lecturerId) {
    Statement st(db_,
                 "SELECT c.course_id, c.code, c.title, c.department_id FROM course c "
                 "JOIN course_lecturer cl ON cl.course_id = c.course_id "
                 "WHERE cl.lecturer_id = ?1 ORDER BY c.code");
    st.bind(1, lecturerId);
    return readCourses(st);
}

std::vector<Course> SqliteCourseRepository::listCoursesForStudent(int studentId) {
    Statement st(db_,
                 "SELECT c.course_id, c.code, c.title, c.department_id FROM course c "
                 "JOIN enrolment e ON e.course_id = c.course_id "
                 "WHERE e.student_id = ?1 ORDER BY c.code");
    st.bind(1, studentId);
    return readCourses(st);
}

std::vector<Assignment> SqliteCourseRepository::listAssignmentsForCourse(int courseId) {
    Statement st(db_, (std::string("SELECT ") + kAssignmentColumns +
                       "FROM assignment a WHERE a.course_id = ?1 ORDER BY a.deadline").c_str());
    st.bind(1, courseId);
    return readAssignments(st);
}

}  // namespace lms
