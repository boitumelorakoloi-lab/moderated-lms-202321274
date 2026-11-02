# make_schema_header.cmake - does the same as the configure_file() call in CMakeLists.txt,
# for builds that do not use CMake to compile (packaging/build-windows.sh).
file(READ "${SQL}" LMS_SCHEMA_SQL)
configure_file("${IN}" "${OUT}" @ONLY)
