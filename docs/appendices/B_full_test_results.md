# Appendix B - Full Test Results (v1.1)

Run on 2 November 2026 on the v1.1 code (Ubuntu 24.04, GCC 13, Release build); the same tests run in CI on every push. **104 / 104 passed** (91 GoogleTest instances from 80 test functions + 13 system tests). The 13 system tests also passed against the Windows `lms.exe`.

Raw output: [`unit-integration-results.txt`](../test-results/unit-integration-results.txt), [`system-test-results.txt`](../test-results/system-test-results.txt), [system test transcripts](../test-results/system-outputs/), [coverage](../test-results/coverage/summary-business-logic.txt). Business-logic line coverage 96.4% (868 / 900 lines).

| ID | Test | What it checks | Result |
|---|---|---|---|
| UT-01 | `DaysLateBoundary.CountsEachStarted24Hours (12 cases)` | Boundary values for BR-02. | Pass |
| UT-02 | `PenaltyPolicy.SrsWorkedExample` | The worked example in BR-01. | Pass |
| UT-03 | `PenaltyPolicy.FinalMarkForEachClass` | One value from each equivalence class of days late. | Pass |
| UT-04 | `PenaltyPolicy.NeverBelowZero` | The final mark is never below 0 (BR-01). | Pass |
| UT-05 | `PenaltyPolicy.MaximumMarkBoundaries` | Boundaries of the maximum mark (1 and 100). | Pass |
| UT-10 | `SubmissionState.AllowedArrows` | Every arrow on the state machine is allowed. | Pass |
| UT-11 | `SubmissionState.ForbiddenArrows` | Moves that are not on the diagram are refused. | Pass |
| UT-12 | `SubmissionState.TransitionThrowsOnBadMove` | TransitionTo() throws and leaves the state unchanged. | Pass |
| UT-13 | `SubmissionVersions.LatestIsHighestVersion` | The newest version is the one marked (FR-11). | Pass |
| UT-14 | `AssignmentDeadline.EffectiveDeadline` | Effective deadline uses the extension when there is one. | Pass |
| UT-15 | `Users.FactoryAndPermissions` | The Factory builds the right subclass, and permissions follow the roles (FR-02). | Pass |
| UT-16 | `Users.LockAfterFiveWrongPasswords` | FR-03 boundary values - 4 wrong passwords do not lock, the 5th does, for 15 minutes. | Pass |
| UT-17 | `Types.CodesRoundTrip` | Text codes match the CHECK constraints in schema.sql and round-trip. | Pass |
| UT-18 | `Time.ParseDateTime` | Date input - valid and invalid partitions (used by the menus, NFR-07). | Pass |
| UT-19 | `ExtensionTest.WholeClassFlag` | An Extension with no student is a whole-class extension. | Pass |
| IT-01 | `AuthServiceTest.LoginReturnsRoleSubclass` | Correct login returns the right subclass (Factory) - UC-01 main flow. | Pass |
| IT-02 | `AuthServiceTest.UsernameCaseAndSpaces` | Usernames are not case-sensitive and spaces are trimmed. | Pass |
| IT-03 | `AuthServiceTest.WrongPasswordAndUnknownUserSameMessage` | Wrong password and unknown user give the same message (no username guessing). | Pass |
| IT-04 | `AuthServiceTest.SqlInjectionIsJustText` | SQL injection attempt in the username is treated as plain text (NFR-05). | Pass |
| IT-05 | `AuthServiceTest.LockoutIsSavedAndExpires` | FR-03 lock-out after 5 wrong passwords, saved in the database, 15 minutes. | Pass |
| IT-06 | `AuthServiceTest.GoodLoginResetsCount` | A good login resets the wrong-password count. | Pass |
| IT-07 | `AuthServiceTest.DeactivatedCannotLogIn` | A deactivated account cannot log in (FR-04). | Pass |
| IT-08 | `AuthServiceTest.ChangePassword` | Change password - wrong old password, too short (7/8 boundary), success (UC-02). | Pass |
| IT-09 | `AuthServiceTest.PasswordsAreSaltedArgon2id` | Passwords are stored as salted Argon2id hashes, never plain text (NFR-04). | Pass |
| IT-10 | `AdminServiceTest.UsernameBoundaries` | Username length boundaries 2 / 3 / 30 / 31 and allowed characters. | Pass |
| IT-11 | `AdminServiceTest.CreateUserRules` | Other create-user rules - blank name, duplicate (any case), short password. | Pass |
| IT-12 | `AdminServiceTest.EditResetDeactivate` | Edit, reset password (also unlocks) and deactivate; cannot deactivate yourself. | Pass |
| IT-13 | `AdminServiceTest.DepartmentRules` | Departments - blank/duplicate name, HoD must have HOD role, one department per HoD. | Pass |
| IT-14 | `AdminServiceTest.CourseRules` | Courses - blank fields, unknown department, duplicate code. | Pass |
| IT-15 | `AdminServiceTest.AssignAndEnrolRules` | Assign lecturers and enrol students - role checks and duplicates (FR-06). | Pass |
| IT-16 | `AdminServiceTest.BackupAndRestore` | Backup and restore round trip; bad paths are refused (FR-25). | Pass |
| IT-20 | `AssignmentServiceTest.CreateAssignmentRules` | Maximum mark boundaries 0 / 1 / 100 / 101 and other create rules (FR-07). | Pass |
| IT-21 | `AssignmentServiceTest.ExtensionInputRules` | Extension input rules (BR-03). | Pass |
| IT-22 | `AssignmentServiceTest.OnlyCourseLecturer` | Only a lecturer who teaches the course may give an extension. | Pass |
| IT-23 | `AssignmentServiceTest.ExpiredExtensionWithNoSubmission` | An extension that has already passed is refused when nothing was handed in. | Pass |
| IT-24 | `AssignmentServiceTest.ExtensionCoversLateWork` | Late -> Submitted when the extension covers the work (state machine, SDD Figure 3). | Pass |
| IT-25 | `AssignmentServiceTest.ExtensionAfterMarkingOrTooEarly` | No extension once the work is marked; none earlier than on-time work. | Pass |
| IT-26 | `AssignmentServiceTest.StudentOverview` | Student overview shows deadlines, extensions and states (FR-09); only own courses. | Pass |
| IT-30 | `SubmissionServiceTest.FileSizeBoundaries` | File size boundaries 0 / 1 byte / 10 MB / 10 MB + 1 (FR-13, UC-18). | Pass |
| IT-31 | `SubmissionServiceTest.FileTypes` | File type partitions - allowed types (any case) and rejected types; missing file. | Pass |
| IT-32 | `SubmissionServiceTest.OnTimeAtDeadlineBoundary` | UC-17 main flow and the deadline boundary (exactly at the deadline is on time). | Pass |
| IT-33 | `SubmissionServiceTest.ResubmitKeepsVersions` | A1 resubmission before the deadline keeps the old version (FR-11, BR-04). | Pass |
| IT-34 | `SubmissionServiceTest.LateOneSecondAfter` | A2 late submission one second after the deadline, with the days late (BR-02). | Pass |
| IT-35 | `SubmissionServiceTest.NoResubmitAfterDeadline` | E2 resubmission after the deadline is refused; late work cannot be resubmitted. | Pass |
| IT-36 | `SubmissionServiceTest.ExtensionChangesDeadline` | An extension moves the student's deadline (effective deadline). | Pass |
| IT-37 | `SubmissionServiceTest.Rejections` | Access rules - not enrolled, unknown assignment, marked work, released assignment. | Pass |
| IT-38 | `SubmissionServiceTest.FailedSaveRemovesCopiedFile` | E3 - if the database save fails, the copied file is deleted again. | Pass |
| IT-39 | `SubmissionServiceTest.OnlyReleasedOwnResults` | Students only see released marks, and only their own (FR-20, NFR-08). | Pass |
| IT-40 | `MarkingServiceTest.RawMarkBoundaries` | Raw mark boundaries -1 / 0 / max / max + 1 (FR-14). | Pass |
| IT-41 | `MarkingServiceTest.FeedbackRequired` | Written feedback is required (C8). | Pass |
| IT-42 | `MarkingServiceTest.ChangeNeedsReasonAndIsAudited` | First mark needs no reason; any change needs one (BR-05, FR-16); audit is written. | Pass |
| IT-43 | `MarkingServiceTest.PenaltyFromLatestVersion` | Late penalty is applied using the latest version's time (FR-15, UC-08 A2). | Pass |
| IT-44 | `MarkingServiceTest.OnlyCourseLecturer` | Only the course lecturer can mark; unknown submission refused. | Pass |
| IT-45 | `MarkingServiceTest.ModeratedAndReturned` | Moderated work cannot be re-marked; returned work can (with a reason). | Pass |
| IT-46 | `MarkingServiceTest.ListToMark` | List to mark shows Submitted, Late, Marked and Returned work with days late. | Pass |
| IT-50 | `ModerationServiceTest.OtherDepartmentRefused` | E1 - an HoD of another department, or an HoD with no department, is refused. | Pass |
| IT-51 | `ModerationServiceTest.ListShowsEverything` | The HoD sees raw mark, penalty, final mark, feedback and extension (FR-17). | Pass |
| IT-52 | `ModerationServiceTest.Approve` | Approve sets Moderated and records who approved (UC-11 main flow). | Pass |
| IT-53 | `ModerationServiceTest.Adjust` | Adjust - range boundaries, reason required, new final mark (UC-12, BR-05). | Pass |
| IT-54 | `ModerationServiceTest.ReturnNeedsComment` | Return for re-marking needs a comment (UC-13). | Pass |
| IT-55 | `ModerationServiceTest.ReleaseRules` | Release only when every submission is Moderated, and only once (FR-19, UC-14). | Pass |
| IT-56 | `ModerationServiceTest.AuditLogAccess` | Audit log access - HoD (own department), administrator yes; others no (FR-22). | Pass |
| IT-60 | `RepositoryTest.FailedSaveIsRolledBack` | A failed save is fully undone (NFR-07). The audit insert fails (a REMARK with no reason breaks the CHECK constraint), so the mark and the state change must not be kept. | Pass |
| IT-61 | `RepositoryTest.TriggerStopsMarkAboveMaximum` | The raw mark trigger stops a mark above max_mark even if a service forgot to check. | Pass |
| IT-62 | `RepositoryTest.AuditLogIsAppendOnly` | The audit log cannot be edited or deleted, even with direct SQL (FR-21). | Pass |
| IT-63 | `RepositoryTest.ConstraintsEnforced` | Foreign keys and the HoD-role trigger are enforced in the database itself. | Pass |
| IT-64 | `RepositoryTest.PersistsToFileInWalMode` | Data persists in a real file, in WAL mode, and the schema is created only once. | Pass |
| IT-65 | `RepositoryTest.BadPathThrows` | A database file that cannot be opened gives a DatabaseException, not a crash. | Pass |
| IT-70 | `EndToEnd.SubmissionToReleasedMark` | The full activity diagram, including a late submission, a return and an adjustment. | Pass |
| IT-80 | `ClassExtensionTest.AppliesToEveryStudent` | The new deadline applies to every enrolled student (FR-27). | Pass |
| IT-81 | `ClassExtensionTest.ShownOnStudentList` | The student's list shows the extension, its type and reason (CR1 second part, FR-09). | Pass |
| IT-82 | `ClassExtensionTest.LaterDeadlineWins` | Own and class extension together - the later one wins (BR-06). | Pass |
| IT-83 | `ClassExtensionTest.LateWorkBecomesOnTime` | Work handed in late becomes on time (Late -> Submitted) and gets no penalty. | Pass |
| IT-84 | `ClassExtensionTest.NewWorkUsesClassDeadline` | New work handed in after the old deadline but before the class one is on time. | Pass |
| IT-85 | `ClassExtensionTest.InputRules` | Input rules, with boundaries (BR-06). | Pass |
| IT-86 | `ClassExtensionTest.OnlyCourseLecturer` | Only a lecturer who teaches the course (access control, NFR-08). | Pass |
| IT-87 | `ClassExtensionTest.RefusedAfterMarkingStarts` | Refused once marking has started (the marks would change under the moderator). | Pass |
| IT-88 | `DatabaseUpgradeTest.OldDatabaseGetsNewTable` | A v1.0 database file is upgraded when v1.1 opens it, keeping its data. | Pass |
| AT-01 | `NonFunctional.Nfr01PerformanceWith800Students` | (NFR-01): load 800 students, 5 courses, 25 assignments and 20,000 submissions, then time login and the main list screens. | Pass |
| AT-02 | `NonFunctional.Nfr02TwentyConcurrentUsers` | (NFR-02): 20 users, each with their own connection, save 20 submissions at the same time. Every save must succeed and every row must be there afterwards. | Pass |
| ST-01 | `System.ST-01_login` | UC-01 Log in: main flow, E wrong password / unknown user, FR-03 lock-out | Pass |
| ST-02 | `System.ST-02_submit_resubmit` | UC-17 main flow + A1 resubmit (UC-19), UC-16 view states | Pass |
| ST-03 | `System.ST-03_submit_late` | UC-17 A2 submit late (UC-20) with penalty warning, BR-04 no resubmit of late work | Pass |
| ST-04 | `System.ST-04_bad_files` | UC-17 E1 / UC-18 validate file: wrong type, empty, over 10 MB, missing; cancel | Pass |
| ST-05 | `System.ST-05_mark` | UC-08 mark submission: E1 out-of-range mark and blank feedback, UC-09 penalty, A1 re-mark needs reason | Pass |
| ST-06 | `System.ST-06_extension` | UC-07 grant extension: invalid date, blank reason, BR-03 rules, student sees new deadline | Pass |
| ST-07 | `System.ST-07_moderate_release` | UC-11 moderate (approve), UC-13 return, UC-12 adjust, UC-14 release (FR-19), UC-15 audit, UC-21 view mark | Pass |
| ST-08 | `System.ST-08_admin` | UC-03/04/05 manage users, departments, courses, enrolment, deactivation (FR-04 to FR-06) | Pass |
| ST-09 | `System.ST-09_role_isolation` | FR-01/FR-02/NFR-08: each role sees only its own menu and data | Pass |
| ST-10 | `System.ST-10_change_password` | UC-02 change password (FR-26): wrong current, mismatch, too short, success | Pass |
| ST-11 | `System.ST-11_backup_restore` | FR-25 back up and restore; a file that is not a backup is refused | Pass |
| ST-12 | `System.ST-12_robustness` | NFR-07 bad input never crashes the program; input ending exits cleanly | Pass |
| ST-13 | `System.ST-13_class_extension` | CR1 (v1.1): UC-07 A2 whole-class extension - bad date, unknown assignment, blank reason, late work now on time, no earlier re-extension; both students see it on their list | Pass |

## Regression run for v1.1 (CR1)

| | v1.0 | v1.1 |
|---|---|---|
| GoogleTest (unit, integration, acceptance) | 81 / 81 | 91 / 91 |
| System tests | 12 / 12 | 13 / 13 |
| v1.0 tests still passing | – | 93 / 93 (IT-26 and ST-06 updated for the new list wording) |
| cppcheck / clang-tidy findings | 0 / 0 | 0 / 0 |

## Defect log

| ID | Found by | Severity | Problem | Fix |
|---|---|---|---|---|
| D-01 | Build | Low | Missing initialiser warning | Every field set explicitly |
| D-02 | Smoke test | Low | Test usernames broke the 3-character rule | Renamed |
| D-03 | Smoke test | Low | Lock-out test used 4 attempts, FR-03 says 5 | Test fixed |
| D-04 | Manual run | Low | Misleading restore error message | Clear message |
| D-05 | Manual run | Low | Extension footnote shown with no extension | Only shown when needed |
| D-06 | Manual run | Low | HoD's lecturer menu said "Log out" | "Exit this menu" |
| D-07 | Design review | Medium | Public methods not in the class diagram | Removed; gaps recorded |
| D-08 | Design review | Medium | Expired extension could be given | Refused (IT-23) |
| D-09 | CI, AT-02 | High | "database is locked" in ~2% of 20-user runs | Busy timeout set before the PRAGMAs; 0 / 300 failures |
