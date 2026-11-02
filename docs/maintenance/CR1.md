# Change Request CR1 - record

| Item | Value |
|---|---|
| Request | "A lecturer should be able to extend the deadline for a whole class, not just an individual student, and extensions should be shown on the student's assignment list." |
| Raised by | The school, after release v1.0 |
| Classification | **Perfective** maintenance (new functionality asked for by users; v1.0 had no fault and its environment did not change) |
| Priority | Must (FR-27) |
| Release | v1.1.0 (branch `feature/cr1-class-extension`, tag `v1.1`) |
| Full analysis | Final report, section 9 |

## Decisions

1. A whole-class extension is a deadline for **every student in the course** on one assignment,
   stored once in a new table `class_extension` (one row per assignment). Copying a row into
   `extension` for every student was rejected: students enrolled later would be missed, and the
   lecturer could not change it in one step.
2. **BR-06**: course lecturer only; reason required; new deadline after the normal deadline and
   after any earlier class extension (it can only move later); refused once marking has started;
   a past deadline is refused unless it turns late work into on-time work.
3. When a student has a personal and a class extension, the **later** one counts.
4. Late work covered by the new deadline moves `Late → Submitted` in the same transaction.
5. The student list shows, under the table, each extended deadline: personal or whole-class,
   from, to, and the reason (FR-09 as changed).
6. Existing v1.0 databases are upgraded in place (`CREATE TABLE IF NOT EXISTS`); no data changes.

## Impact

| Kind | Affected |
|---|---|
| Requirements | FR-27 (new), BR-06 (new), FR-09 (changed), FR-08/BR-03 (unchanged, now combined with BR-06), FR-12/BR-02 and FR-15/BR-01 (use the effective deadline), FR-17 (HoD sees class extensions), NFR-07, NFR-08 |
| Use cases | UC-07 (new alternative flow A2: whole class), UC-16 (shows extensions), UC-17/UC-20 (deadline check), UC-08, UC-11 (lists show the extension) |
| Diagrams | Class diagram, ERD, state machine (Late → Submitted trigger), deployment (version only). Unchanged: component, activity, sequence diagrams (they call `findExtension`, now wrapped by `applicableExtension`) |
| Classes | `Extension`, `ICourseRepository`, `SqliteCourseRepository`, `Database`, `AssignmentService`, `StudentAssignmentRow`, `MarkView.h` (`applicableExtension`), `SubmissionService`, `MarkingService`, `ModerationService`, `LecturerMenu`, `StudentMenu`, `HodMenu`, `ConsoleApp` (version) |
| Tables | `class_extension` (new), `submission` (status update) |
| Tests | New: UT-19, IT-80 to IT-88, ST-13. Changed: IT-26 (`hasExtension` became `extension`), ST-06 (new list wording). All other v1.0 tests unchanged |

## Regression result

104 / 104 tests pass on v1.1 (91 GoogleTest, 13 system). The 93 v1.0 tests all still pass.
cppcheck 0, clang-tidy 0, business-logic coverage 96.4%. Results: `docs/test-results/`.
