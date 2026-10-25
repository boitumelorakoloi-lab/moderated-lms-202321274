# run_system_test.cmake - runs one system test (Test Plan, section 5).
# It starts the real lms program on a fresh demo database, types the keystrokes in
# <name>.in, saves everything the program printed, and checks <name>.expect:
#   a normal line  = this text must appear in the output
#   a line with !  = this text must NOT appear
#   a line with #  = comment
# Usage: cmake -DLMS_EXE=... -DCASE_DIR=... -DCASE=... -DWORKDIR=... -P run_system_test.cmake

file(REMOVE_RECURSE "${WORKDIR}")
file(MAKE_DIRECTORY "${WORKDIR}")

# Test files used by the file-validation cases (UC-18).
file(WRITE "${WORKDIR}/bad.exe" "not allowed")
file(WRITE "${WORKDIR}/empty.pdf" "")
string(REPEAT "x" 10485761 tooBig)  # 10 MB + 1 byte
file(WRITE "${WORKDIR}/big.pdf" "${tooBig}")

execute_process(
    COMMAND "${LMS_EXE}" --seed-demo
    INPUT_FILE "${CASE_DIR}/${CASE}.in"
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
    RESULT_VARIABLE exitCode
    WORKING_DIRECTORY "${WORKDIR}"
    TIMEOUT 120)

file(WRITE "${WORKDIR}/output.txt" "${output}${errors}")

set(failed "")
if(NOT exitCode EQUAL 0)
    string(APPEND failed "\n  program exit code was ${exitCode} (expected 0)")
endif()

file(STRINGS "${CASE_DIR}/${CASE}.expect" lines)
foreach(line IN LISTS lines)
    if(line STREQUAL "" OR line MATCHES "^#")
        continue()
    endif()
    if(line MATCHES "^!(.*)$")
        string(FIND "${output}" "${CMAKE_MATCH_1}" pos)
        if(NOT pos EQUAL -1)
            string(APPEND failed "\n  should NOT appear: ${CMAKE_MATCH_1}")
        endif()
    else()
        string(FIND "${output}" "${line}" pos)
        if(pos EQUAL -1)
            string(APPEND failed "\n  missing: ${line}")
        endif()
    endif()
endforeach()

if(failed)
    message(FATAL_ERROR "${CASE} FAILED (output saved in ${WORKDIR}/output.txt):${failed}")
endif()
message(STATUS "${CASE} passed")
