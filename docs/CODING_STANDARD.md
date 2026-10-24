# Coding Standard – Moderated LMS

Version 1.0 · October 2026 · Boitumelo Rakoloi (202321274)

This is the coding standard I follow for the Moderated LMS. It is a short, project-sized
subset of the **C++ Core Guidelines** (Stroustrup and Sutter, 2024) with layout rules based on
the **Google C++ Style Guide**. Formatting is checked automatically with `.clang-format`.
Each rule has an ID so the code review checklist in Phase 6 can refer to it.

## 1. Language and build

| ID | Rule |
|----|------|
| B1 | C++17 only, no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`). |
| B2 | Code must build with no warnings at `-Wall -Wextra -Wpedantic` (GCC/Clang) or `/W4` (MSVC). |
| B3 | Every source file is listed by name in `CMakeLists.txt` (no file globbing). |

## 2. Layout

| ID | Rule |
|----|------|
| L1 | 4 spaces per indent, no tabs, lines at most 100 characters. |
| L2 | Opening brace on the same line. Always use braces for `if`/`for`/`while` bodies that take more than one line. |
| L3 | Each file starts with a one-line comment saying what it is for, and the SRS/SDD item it implements where there is one (e.g. `// ... (FR-14)`). |
| L4 | Headers use `#pragma once`. Includes are ordered: own header, standard library, third-party, project. |

## 3. Naming

| ID | Rule | Example |
|----|------|---------|
| N1 | Classes, structs and enums: `PascalCase`, same names as the class diagram. | `SubmissionService`, `ILatePenaltyPolicy` |
| N2 | Interfaces start with `I`. | `IMarkRepository` |
| N3 | Functions and variables: `camelCase`, same names as the class diagram. | `effectiveDeadline()`, `daysLate` |
| N4 | Private data members end with `_`. Members shown on the class diagram as references to services keep the diagram name. | `maxDays_`, `submissions` |
| N5 | Constants: `k` + `PascalCase`. | `kMaxFileBytes` |
| N6 | Everything lives in `namespace lms`. No `using namespace` in headers. | |

## 4. Design and classes

| ID | Rule |
|----|------|
| D1 | Follow the layers: presentation → services → domain / data access. A layer never calls the layer above it. |
| D2 | SQL is only written in `src/data`. Services use the repository interfaces, never `sqlite3` directly. |
| D3 | Services get the current time as a parameter (`TimePoint now`) so the rules can be tested with fixed times. |
| D4 | Mark functions that do not change the object `const`. Pass objects by `const&`. |
| D5 | Base classes with virtual functions have a virtual destructor; overriding functions are marked `override`. Leaf classes are `final`. |
| D6 | No raw `new`/`delete`. Use `std::unique_ptr`, values or references. Resources (database handles, statements, transactions) are owned by RAII classes. |
| D7 | Use `std::optional` for "may be missing" instead of magic values like `-1`. |

## 5. Errors, input and security

| ID | Rule |
|----|------|
| E1 | Errors are reported with exceptions derived from `LmsException`. Do not return error codes from services. |
| E2 | The menus catch `LmsException`; `main()` catches `std::exception`. No exception may end the program unexpectedly (NFR-07). |
| E3 | All keyboard input goes through `ConsoleIO`, which checks it and asks again. |
| E4 | Every SQL value is bound as a parameter (`?1`, `?2`). Building SQL by joining user text is not allowed (NFR-05). |
| E5 | Changes that touch several rows are done inside one `Transaction` (NFR-07). |
| E6 | Passwords are only ever handled by `AuthService` and stored as libsodium Argon2id hashes (NFR-04). Never print or log a password. |
| E7 | Every service method checks `user.can(...)` before doing anything (FR-02). |

## 6. Comments and tests

| ID | Rule |
|----|------|
| C1 | Comments explain *why*, not *what*. Refer to requirement IDs (FR, NFR, BR) where a rule comes from. |
| C2 | Every business rule (BR-01 to BR-05) has at least one automated test. |
| C3 | Tests use GoogleTest, one test file per area, names in the form `Area.WhatIsChecked`. |

## References

Google (2024) *Google C++ Style Guide*. Available at: https://google.github.io/styleguide/cppguide.html

Stroustrup, B. and Sutter, H. (eds.) (2024) *C++ Core Guidelines*. Available at:
https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines
