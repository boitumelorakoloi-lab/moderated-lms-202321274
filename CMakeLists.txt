cmake_minimum_required(VERSION 3.18)
project(ModeratedLMS VERSION 1.0.0 LANGUAGES CXX)

# ---- Language and warnings (Coding Standard, rule B1) ------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # used by clang-tidy

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(LMS_BUILD_TESTS "Build the GoogleTest unit tests" ON)
option(LMS_COVERAGE "Build with gcov coverage flags (GCC/Clang only)" OFF)
if(LMS_COVERAGE AND NOT MSVC)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# ---- Dependencies ------------------------------------------------------------
find_package(SQLite3 REQUIRED)

# libsodium: vcpkg gives a CMake package; Linux/MSYS2 give pkg-config; else search by hand.
find_package(unofficial-sodium CONFIG QUIET)
if(unofficial-sodium_FOUND)
    set(LMS_SODIUM unofficial-sodium::sodium)
else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SODIUM QUIET IMPORTED_TARGET libsodium)
    endif()
    if(SODIUM_FOUND)
        set(LMS_SODIUM PkgConfig::SODIUM)
    else()
        find_path(SODIUM_INCLUDE_DIR sodium.h REQUIRED)
        find_library(SODIUM_LIBRARY NAMES sodium libsodium REQUIRED)
        add_library(lms_sodium UNKNOWN IMPORTED)
        set_target_properties(lms_sodium PROPERTIES
            IMPORTED_LOCATION "${SODIUM_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_INCLUDE_DIR}")
        set(LMS_SODIUM lms_sodium)
    endif()
endif()

# ---- Embed db/schema.sql so the program can create a new database ------------
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/db/schema.sql" LMS_SCHEMA_SQL)
configure_file(src/data/SchemaSql.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/SchemaSql.h" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS db/schema.sql)

# ---- Core library: domain + services + data access (everything but the UI) ---
add_library(lms_core STATIC
    src/domain/Types.cpp
    src/domain/Time.cpp
    src/domain/User.cpp
    src/domain/UserFactory.cpp
    src/domain/Assignment.cpp
    src/domain/Submission.cpp
    src/domain/TenPercentPerDayPolicy.cpp
    src/data/Database.cpp
    src/data/SqliteUserRepository.cpp
    src/data/SqliteCourseRepository.cpp
    src/data/SqliteSubmissionRepository.cpp
    src/data/SqliteMarkRepository.cpp
    src/services/MarkView.cpp
    src/services/AuthService.cpp
    src/services/AdminService.cpp
    src/services/AssignmentService.cpp
    src/services/SubmissionService.cpp
    src/services/MarkingService.cpp
    src/services/ModerationService.cpp
)
target_include_directories(lms_core PUBLIC src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(lms_core PUBLIC SQLite::SQLite3 ${LMS_SODIUM})

# ---- Console program -----------------------------------------------------------
add_executable(lms
    src/main.cpp
    src/presentation/ConsoleIO.cpp
    src/presentation/RoleMenu.cpp
    src/presentation/StudentMenu.cpp
    src/presentation/LecturerMenu.cpp
    src/presentation/HodMenu.cpp
    src/presentation/AdminMenu.cpp
    src/presentation/ConsoleApp.cpp
    src/presentation/DemoData.cpp
)
target_link_libraries(lms PRIVATE lms_core)

# ---- Tests ---------------------------------------------------------------------
if(LMS_BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    add_executable(lms_tests
        tests/test_penalty_policy.cpp
        tests/test_domain.cpp
        tests/test_auth_service.cpp
        tests/test_admin_service.cpp
        tests/test_assignment_service.cpp
        tests/test_submission_service.cpp
        tests/test_marking_service.cpp
        tests/test_moderation_service.cpp
        tests/test_repositories.cpp
        tests/test_end_to_end.cpp
        tests/test_nonfunctional.cpp
    )
    target_link_libraries(lms_tests PRIVATE lms_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(lms_tests PROPERTIES LABELS "unit;integration")

    # System tests: run the real program with scripted keystrokes (tests/system/*.in)
    # and check its output (tests/system/*.expect). One test per use-case scenario.
    file(STRINGS tests/system/cases.txt LMS_SYSTEM_CASES)
    foreach(entry IN LISTS LMS_SYSTEM_CASES)
        string(REGEX REPLACE "\\|.*$" "" caseName "${entry}")
        add_test(NAME "System.${caseName}"
            COMMAND ${CMAKE_COMMAND}
                -DLMS_EXE=$<TARGET_FILE:lms>
                -DCASE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/system
                -DCASE=${caseName}
                -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/system-tests/${caseName}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/system/run_system_test.cmake)
        set_tests_properties("System.${caseName}" PROPERTIES LABELS "system" TIMEOUT 180)
    endforeach()
endif()
