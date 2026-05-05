# cmake/coverage.cmake
#
# Phase-1 §11: gcov-based coverage gate per ADR-013 (Class D classification).
# Threshold: ≥ 80% statement coverage on first-party files
# (src/ + include/apsis/). Vendored upstreams (SOFA, CSPICE), CPM
# dependencies (Eigen, GoogleTest), and the test sources themselves
# are filtered out of the coverage report.
#
# Activation: configure with -DAPSIS_ENABLE_COVERAGE=ON. After running
# the test suite, build the `coverage` target:
#
#   cmake --build build --target coverage
#
# Output:
#   build/coverage/lcov.info        — lcov-format trace data
#   build/coverage/summary.txt      — human-readable percentage breakdown
#
# Exits non-zero on threshold miss (CI failure).

if(NOT APSIS_ENABLE_COVERAGE)
  return()
endif()

if(MSVC)
  message(WARNING
    "[apsis] APSIS_ENABLE_COVERAGE has no effect on MSVC; only the "
    "gcc-13 CI job runs the coverage gate per the Phase 1 plan §11.")
  return()
endif()

find_program(LCOV_EXEC lcov)
find_program(GENHTML_EXEC genhtml)
if(NOT LCOV_EXEC)
  message(WARNING
    "[apsis] APSIS_ENABLE_COVERAGE=ON but `lcov` not found on PATH. "
    "Install with `apt install lcov` (Ubuntu) or `brew install lcov` "
    "(macOS). The coverage target will be unavailable.")
  return()
endif()

set(APSIS_COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
set(APSIS_COVERAGE_THRESHOLD "80" CACHE STRING
    "Minimum first-party statement coverage percentage (NPR 7150.2D Class D)")

# Path filters (lcov --remove patterns).
set(_apsis_cov_excludes
    "${CMAKE_BINARY_DIR}/_deps/*"
    "${CMAKE_SOURCE_DIR}/external/*"
    "${CMAKE_SOURCE_DIR}/tests/*"
    "/usr/include/*"
    "/usr/lib/*"
    "*/c++/*"
    "*/eigen-src/*"
)

add_custom_target(coverage
  COMMENT "Generating gcov coverage report (Phase 1 §11)"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${APSIS_COVERAGE_DIR}"
  COMMAND ${LCOV_EXEC} --capture --directory "${CMAKE_BINARY_DIR}"
          --output-file "${APSIS_COVERAGE_DIR}/raw.info"
          --rc lcov_branch_coverage=0 --quiet
          --ignore-errors mismatch,gcov,inconsistent,unused,empty
  COMMAND ${LCOV_EXEC} --remove "${APSIS_COVERAGE_DIR}/raw.info"
          ${_apsis_cov_excludes}
          --output-file "${APSIS_COVERAGE_DIR}/lcov.info"
          --rc lcov_branch_coverage=0 --quiet
          --ignore-errors mismatch,gcov,inconsistent,unused,empty
  COMMAND ${LCOV_EXEC} --summary "${APSIS_COVERAGE_DIR}/lcov.info"
          --rc lcov_branch_coverage=0 --ignore-errors inconsistent,empty
          > "${APSIS_COVERAGE_DIR}/summary.txt" 2>&1 || true
  COMMAND ${CMAKE_COMMAND}
          -DSUMMARY_FILE=${APSIS_COVERAGE_DIR}/summary.txt
          -DTHRESHOLD=${APSIS_COVERAGE_THRESHOLD}
          -P ${CMAKE_SOURCE_DIR}/cmake/coverage_check.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  USES_TERMINAL
)
