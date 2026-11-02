# Appendix A - Requirements Traceability Matrix (v1.1)

Each functional requirement linked to its use case, design classes, SDD figures (2 class, 3 state machine, 4 activity, 5-7 sequence, 8 ERD), database tables, source files (under `src/`) and test cases (UT unit, IT integration, AT acceptance, ST system). FR-23 and FR-24 are *Could* requirements left out of the release.

| FR | Use case | Design classes | Fig. | Tables | Source files | Tests |
|---|---|---|---|---|---|---|
| FR-01 | UC-01 | AuthService::login, User, UserFactory, IUserRepository | 2 | app_user | services/AuthService.cpp<br>domain/UserFactory.cpp<br>data/SqliteUserRepository.cpp<br>presentation/ConsoleApp.cpp | IT-01, IT-02, ST-01, ST-09 |
| FR-02 | UC-01 | User::can, Permission | 2, 6, 7 | app_user.role | domain/User.cpp<br>(can() checks in every service) | UT-15, IT-22, IT-44, IT-50, IT-56, ST-09 |
| FR-03 | UC-01 | User::recordFailedLogin, isLocked | 2 | app_user | domain/User.cpp<br>services/AuthService.cpp | UT-16, IT-05, IT-06, ST-01 |
| FR-04 | UC-03 | AdminService::createUser, deactivateUser, UserFactory | 1, 2 | app_user | services/AdminService.cpp<br>presentation/AdminMenu.cpp<br>data/SqliteUserRepository.cpp | IT-07, IT-10 to IT-12, ST-08 |
| FR-05 | UC-04 | AdminService::createCourse, Department, Course | 2 | department, course | services/AdminService.cpp<br>data/SqliteCourseRepository.cpp | IT-13, IT-14, ST-08 |
| FR-06 | UC-05 | AdminService::assignLecturer, enrolStudent | 2 | enrolment, course_lecturer | services/AdminService.cpp<br>data/SqliteCourseRepository.cpp | IT-15, ST-08 |
| FR-07 | UC-06 | AssignmentService::createAssignment, Assignment | 2 | assignment | services/AssignmentService.cpp<br>presentation/LecturerMenu.cpp | IT-20, ST-09 |
| FR-08 | UC-07 | AssignmentService::grantExtension, Extension | 2, 3 | extension | services/AssignmentService.cpp<br>data/SqliteCourseRepository.cpp | IT-21 to IT-25, ST-06, IT-82 |
| FR-09 | UC-16 | AssignmentService::overviewForStudent, StudentAssignmentRow (extension), StudentMenu::printExtensions (v1.1) | 2 | assignment, extension, class_extension, submission | services/AssignmentService.cpp<br>presentation/StudentMenu.cpp | IT-26, IT-81, ST-02, ST-06, ST-13 |
| FR-10 | UC-17 | SubmissionService::check, submit, Submission, applicableExtension (v1.1) | 2, 3, 4, 5 | submission, submission_version | services/SubmissionService.cpp<br>data/SqliteSubmissionRepository.cpp<br>presentation/StudentMenu.cpp | IT-32, ST-02, IT-83, IT-84 |
| FR-11 | UC-19 | SubmissionService, SubmissionVersion | 3, 5 | submission_version | services/SubmissionService.cpp<br>domain/Submission.cpp | UT-13, IT-33, ST-02 |
| FR-12 | UC-20 | SubmissionService, ILatePenaltyPolicy::daysLate, applicableExtension (v1.1) | 3, 4, 5 | submission | services/SubmissionService.cpp<br>domain/TenPercentPerDayPolicy.cpp | UT-01, IT-34, IT-35, ST-03, IT-83, IT-84 |
| FR-13 | UC-18 | SubmissionService::validateFile | 4, 5 | submission_version | services/SubmissionService.cpp (validateFile) | IT-30, IT-31, ST-04 |
| FR-14 | UC-08 | MarkingService::mark, Mark | 2, 3, 4 | mark | services/MarkingService.cpp<br>data/SqliteMarkRepository.cpp<br>presentation/LecturerMenu.cpp | IT-40, IT-41, IT-61, ST-05 |
| FR-15 | UC-09 | ILatePenaltyPolicy, TenPercentPerDayPolicy, applicableExtension (v1.1) | 2, 4 | mark.policy_code | domain/TenPercentPerDayPolicy.cpp<br>services/MarkView.cpp | UT-01 to UT-05, IT-43, IT-51, ST-05, IT-83 |
| FR-16 | UC-08 | MarkingService::mark (reason) | 3 | mark, audit_entry | services/MarkingService.cpp | IT-42, IT-45, ST-05 |
| FR-17 | UC-11 | ModerationService::listForModeration | 6 | mark, extension | services/ModerationService.cpp<br>presentation/HodMenu.cpp | IT-51, ST-07, ST-13 |
| FR-18 | UC-11/12/13 | ModerationService approve, adjust, returnForRemarking | 3, 4, 6 | mark, submission, audit_entry | services/ModerationService.cpp<br>data/SqliteMarkRepository.cpp | IT-52 to IT-54, ST-07 |
| FR-19 | UC-14 | ModerationService::release, ISubmissionRepository::releaseAll | 3, 7 | assignment, submission | services/ModerationService.cpp<br>data/SqliteSubmissionRepository.cpp | IT-55, ST-07 |
| FR-20 | UC-21 | Submission state, User::can | 3, 4 | submission.status | services/SubmissionService.cpp (myResults)<br>presentation/StudentMenu.cpp | IT-39, ST-04, ST-07 |
| FR-21 | UC-10 | IMarkRepository::saveChange, AuditEntry | 6, 7 | audit_entry + triggers | data/SqliteMarkRepository.cpp<br>db/schema.sql (triggers) | IT-42, IT-53, IT-55, IT-60, IT-62, IT-70, ST-07 |
| FR-22 | UC-15 | ModerationService::viewAuditLog | 2 | audit_entry | services/ModerationService.cpp<br>presentation/HodMenu.cpp, AdminMenu.cpp | IT-56, ST-07 |
| FR-23 | UC-13 | Not in the v1.0 design (Could; see 3.3) | – | – | – | – |
| FR-24 | – | Not in the v1.0 design (Could; see 3.3) | – | – | – | – |
| FR-25 | – | AdminService::backup, restore | 2 | whole database file | services/AdminService.cpp<br>data/Database.cpp | IT-16, ST-11 |
| FR-26 | UC-02 | AuthService::changePassword | 2 | app_user.password_hash | services/AuthService.cpp<br>presentation/RoleMenu.cpp | IT-08, ST-10 |
| **FR-27** (v1.1) | UC-07 (A2) | AssignmentService::extendForClass, Extension (empty studentId = whole class), ICourseRepository::findClassExtension / saveClassExtension, LecturerMenu::extendForClass, applicableExtension | 2, 3, 8 | class_extension, submission | services/AssignmentService.cpp<br>services/MarkView.cpp<br>data/SqliteCourseRepository.cpp<br>data/Database.cpp<br>presentation/LecturerMenu.cpp | UT-19, IT-80 to IT-88, ST-13 |

## Non-functional requirements

| NFR | Requirement (short) | Verified by | Result |
|---|---|---|---|
| NFR-01 | Login ≤ 2 s, lists ≤ 1 s with 800 students and 20,000 submissions | AT-01 (measured) | Pass (0.15 s / 0.04 s) |
| NFR-02 | 20 users at once, no lost data or lock errors | AT-02 (20 threads, 400 saves); D-09 fixed | Pass |
| NFR-03 | Works with no internet | AT-03: no network code; all tests offline | Pass |
| NFR-04 | Argon2id password hashes only | AT-04, IT-09 | Pass |
| NFR-05 | Parameterised queries only | AT-05, IT-04 | Pass |
| NFR-06 | 4 of 5 users finish their main task in 3 min after 15 min training | Usability test sheet | Not yet run |
| NFR-07 | Bad input or database errors never crash; failed saves undone | AT-06: ST-12, IT-38, IT-60, IT-65, IT-87 | Pass |
| NFR-08 | Users see only what their role allows | AT-07: ST-09, IT-39, IT-50, IT-56, IT-86 | Pass |
| NFR-09 | ≥ 70% business-logic line coverage | AT-08: gcovr in CI | Pass (96.4%) |
| NFR-10 | Keyboard only, nothing shown by colour alone | AT-09 | Pass |
| NFR-11 | C++17, CMake, separate data-access layer | AT-10 | Pass |
