# Moderated Learning Management System

CS4433 Software Engineering · Individual Project · Boitumelo Rakoloi (202321274)

A C++17 console application that lets a secondary school manage courses, assignments,
marking and Head of Department moderation. It works offline, stores its data in SQLite and
protects passwords with libsodium (Argon2id).

- Design documents: `docs/uml/` (draw.io sources) and the Software Design Document
- Coding standard: [`docs/CODING_STANDARD.md`](docs/CODING_STANDARD.md)
- Database schema: [`db/schema.sql`](db/schema.sql)

---

## Build guide

### What you need

| Tool | Version | Why |
|------|---------|-----|
| C++ compiler | GCC 9+, Clang 10+ or MSVC 2019+ | C++17 |
| CMake | 3.18 or newer | build |
| SQLite 3 (development files) | 3.24 or newer | database |
| libsodium (development files) | 1.0.18 or newer | password hashing |
| GoogleTest | 1.10 or newer (optional) | unit tests – downloaded automatically if not installed |

### Ubuntu 22.04 / 24.04

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libsqlite3-dev libsodium-dev libgtest-dev

git clone <repository-url> moderated-lms
cd moderated-lms
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The program is `build/lms`.

### Windows 10/11 (MSYS2 – recommended)

1. Install MSYS2 from https://www.msys2.org and open the **MSYS2 UCRT64** terminal.
2. Install the tools:

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
       mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf \
       mingw-w64-ucrt-x86_64-sqlite3 mingw-w64-ucrt-x86_64-libsodium mingw-w64-ucrt-x86_64-gtest git
   ```

3. Build:

   ```bash
   git clone <repository-url> moderated-lms
   cd moderated-lms
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

The program is `build/lms.exe`. To run it outside MSYS2, copy `libsqlite3-0.dll`,
`libsodium-26.dll`, `libstdc++-6.dll`, `libgcc_s_seh-1.dll` and `libwinpthread-1.dll` from
`C:\msys64\ucrt64\bin` next to `lms.exe`.

### Windows with Visual Studio (alternative)

Install the dependencies with vcpkg (`vcpkg install sqlite3 libsodium gtest`) and configure with
`-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake`. CMake finds libsodium through
vcpkg's `unofficial-sodium` package automatically.

### Build options

| Option | Default | Meaning |
|--------|---------|---------|
| `LMS_BUILD_TESTS` | `ON` | build `lms_tests` (turn off with `-DLMS_BUILD_TESTS=OFF`) |
| `CMAKE_BUILD_TYPE` | – | `Debug` or `Release` |

---

## Running the program

```bash
./build/lms                    # uses lms.db and the submissions/ folder in the current folder
./build/lms --db school.db     # a different database file
./build/lms --files D:/lms/files
./build/lms --help
```

**First run:** if the database has no accounts, the program asks you to create the
administrator account. Log in with it, then create departments, courses and users from the
administrator menu.

**Demo data:** on an empty database, `./build/lms --seed-demo` creates sample accounts
(password `Password1` for all of them), two courses and two assignments:

| Username | Role |
|----------|------|
| `admin` | Administrator |
| `hod.molapo` | Head of Department (Sciences), also teaches PHYS-F4 |
| `lect.mokoena` | Lecturer (MATH-F4) |
| `stu.thabo`, `stu.lerato`, `stu.palesa` | Students |

A sample file to submit is created at `demo_files/my_homework.txt`.

---

## Running the tests

```bash
ctest --test-dir build --output-on-failure            # all 93 tests
ctest --test-dir build -L "unit|integration"          # GoogleTest tests only (81)
ctest --test-dir build -L system                      # scripted console tests only (12)
```

The system tests in `tests/system/` run the real program: `<ID>.in` holds the keystrokes and
`<ID>.expect` lists text that must (or, with `!`, must not) appear in the output.

**Coverage** (GCC/Clang, needs `gcovr`):

```bash
cmake -S . -B build-cov -DLMS_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-cov
ctest --test-dir build-cov -L "unit|integration"
gcovr -r . build-cov --filter src/services/ --filter src/domain/ --print-summary
```

GitHub Actions (`.github/workflows/ci.yml`) builds the project and runs every test on each push
and pull request, and a second job checks that business-logic line coverage stays at or above
70%. Saved results are in `docs/test-results/`.

---

## Project structure

```
CMakeLists.txt            build script
db/schema.sql             SQLite schema (embedded into the program at build time)
docs/                     coding standard and UML sources
src/
  main.cpp                creates the objects and wires them together
  presentation/           ConsoleApp, RoleMenu and the four role menus, ConsoleIO
  services/               AuthService, AdminService, AssignmentService,
                          SubmissionService, MarkingService, ModerationService
  domain/                 User hierarchy, UserFactory, Assignment, Submission, Mark,
                          ILatePenaltyPolicy / TenPercentPerDayPolicy, exceptions
  data/                   Database (RAII), repository interfaces, SQLite repositories
tests/                    GoogleTest tests (unit, integration, acceptance) + system/ scripted tests
```
