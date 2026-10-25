// Acceptance tests for the measurable non-functional requirements:
//   NFR-01 - with 800 students and 20,000 submissions, login <= 2 s and list screens <= 1 s.
//   NFR-02 - 20 people saving at once lose no data and see no "database is locked" errors.
// These use a real database file (not in-memory) so WAL and file locking are exercised.
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "TestFixture.h"

namespace lms::test {

namespace {
double secondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}
}  // namespace

class NonFunctional : public ::testing::Test {
protected:
    fs::path folder;
    std::string path;

    void SetUp() override {
        folder = fs::temp_directory_path() /
                 (std::string("lms_nfr_") + ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(folder);
        fs::create_directories(folder);
        path = (folder / "load.db").string();
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(folder, ec);
    }
};

// AT-01 (NFR-01): load 800 students, 5 courses, 25 assignments and 20,000 submissions,
// then time login and the main list screens.
TEST_F(NonFunctional, Nfr01PerformanceWith800Students) {
    Database db(path);
    SqliteUserRepository users(db);
    SqliteCourseRepository courses(db);
    SqliteSubmissionRepository submissions(db);
    SqliteMarkRepository marks(db);
    TenPercentPerDayPolicy policy;

    const int hodId = users.add(*UserFactory::create(Role::HeadOfDepartment, 0, "hod", "H", testHash()));
    const int lectId = users.add(*UserFactory::create(Role::Lecturer, 0, "lect", "L", testHash()));
    const int dept = courses.addDepartment(Department(0, "Sciences", hodId));
    std::vector<int> courseIds, assignmentIds, studentIds;
    {
        Transaction tx(db);
        for (int c = 0; c < 5; ++c) {
            courseIds.push_back(courses.addCourse(Course(0, "C" + std::to_string(c), "Course", dept)));
            courses.assignLecturer(lectId, courseIds.back());
            for (int k = 0; k < 5; ++k) {
                assignmentIds.push_back(courses.addAssignment(
                    Assignment(0, courseIds.back(), "A", kNow, 50, lectId)));
            }
        }
        for (int s = 0; s < 800; ++s) {
            studentIds.push_back(users.add(*UserFactory::create(
                Role::Student, 0, "student" + std::to_string(s), "Student", testHash())));
            for (int c : courseIds) courses.enrol(studentIds.back(), c);
        }
        for (int a : assignmentIds) {
            for (int s : studentIds) {
                Statement ins(db, "INSERT INTO submission (assignment_id, student_id, status) VALUES (?1, ?2, 'MARKED')");
                ins.bind(1, a).bind(2, s);
                ins.run();
                const int subId = static_cast<int>(db.lastInsertId());
                Statement ver(db, "INSERT INTO submission_version (submission_id, version_no, stored_path, "
                                  "original_name, size_bytes, submitted_at) VALUES (?1, 1, ?2, 'w.pdf', 100, ?3)");
                ver.bind(1, subId).bind(2, "p" + std::to_string(subId)).bindTime(3, kNow);
                ver.run();
                Statement mk(db, "INSERT INTO mark (submission_id, raw_mark, feedback, marked_by, marked_at) "
                                 "VALUES (?1, 30, 'Ok', ?2, ?3)");
                mk.bind(1, subId).bind(2, lectId).bindTime(3, kNow);
                mk.run();
            }
        }
        tx.commit();
    }
    {
        Statement count(db, "SELECT COUNT(*) FROM submission");
        count.step();
        ASSERT_EQ(count.getInt(0), 20000);
    }

    AuthService auth(users);
    AssignmentService assignmentService(courses, submissions);
    MarkingService marking(courses, submissions, marks, policy);
    ModerationService moderation(courses, submissions, marks, policy);

    auto t = std::chrono::steady_clock::now();
    auto student = auth.login("student400", kPassword, kNow);
    const double loginTime = secondsSince(t);

    t = std::chrono::steady_clock::now();
    const auto overview = assignmentService.overviewForStudent(static_cast<Student&>(*student));
    const double overviewTime = secondsSince(t);
    EXPECT_EQ(overview.size(), 25u);

    auto hod = users.findById(hodId);
    t = std::chrono::steady_clock::now();
    const auto rows = moderation.listForModeration(static_cast<HeadOfDepartment&>(*hod), assignmentIds[0]);
    const double moderationTime = secondsSince(t);
    EXPECT_EQ(rows.size(), 800u);

    auto lect = users.findById(lectId);
    t = std::chrono::steady_clock::now();
    const auto toMark = marking.listToMark(static_cast<Lecturer&>(*lect), assignmentIds[0]);
    const double markingTime = secondsSince(t);
    EXPECT_EQ(toMark.size(), 800u);

    std::cout << "[ NFR-01 ] login " << loginTime << " s, student list " << overviewTime
              << " s, moderation list (800 rows) " << moderationTime
              << " s, marking list (800 rows) " << markingTime << " s\n";
    EXPECT_LE(loginTime, 2.0);
    EXPECT_LE(overviewTime, 1.0);
    EXPECT_LE(moderationTime, 1.0);
    EXPECT_LE(markingTime, 1.0);
}

// AT-02 (NFR-02): 20 users, each with their own connection, save 20 submissions at the same
// time. Every save must succeed and every row must be there afterwards.
TEST_F(NonFunctional, Nfr02TwentyConcurrentUsers) {
    constexpr int kUsers = 20;
    constexpr int kSavesEach = 20;
    std::vector<int> studentIds;
    std::vector<int> assignmentIds;
    {
        Database db(path);
        SqliteUserRepository users(db);
        SqliteCourseRepository courses(db);
        const int lect = users.add(*UserFactory::create(Role::Lecturer, 0, "lect", "L", testHash()));
        const int dept = courses.addDepartment(Department(0, "D", std::nullopt));
        const int c = courses.addCourse(Course(0, "C", "Course", dept));
        for (int i = 0; i < kSavesEach; ++i) {
            assignmentIds.push_back(courses.addAssignment(Assignment(0, c, "A", kNow, 50, lect)));
        }
        for (int i = 0; i < kUsers; ++i) {
            studentIds.push_back(users.add(*UserFactory::create(
                Role::Student, 0, "s" + std::to_string(i) + "x", "S", testHash())));
            courses.enrol(studentIds.back(), c);
        }
    }

    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    for (int u = 0; u < kUsers; ++u) {
        threads.emplace_back([&, u]() {
            try {
                Database db(path);  // each "PC" has its own connection
                SqliteSubmissionRepository repo(db);
                for (int a : assignmentIds) {
                    Submission s(0, a, studentIds[u], SubmissionStatus::Submitted);
                    s.addVersion(SubmissionVersion(1, "p_" + std::to_string(u) + "_" + std::to_string(a),
                                                   "w.pdf", 100, kNow));
                    repo.addSubmission(s);
                }
            } catch (const std::exception& e) {
                std::cerr << "thread " << u << ": " << e.what() << "\n";
                ++failures;
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(failures.load(), 0);
    Database db(path);
    Statement count(db, "SELECT COUNT(*) FROM submission_version");
    count.step();
    EXPECT_EQ(count.getInt(0), kUsers * kSavesEach);
    std::cout << "[ NFR-02 ] " << kUsers << " concurrent users, " << count.getInt(0)
              << " saves, " << failures.load() << " failures\n";
}

}  // namespace lms::test
