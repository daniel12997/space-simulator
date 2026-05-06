# cmake/coverage_check.cmake — script-mode helper invoked by the
# `coverage` target to enforce the threshold gate.
#
# Reads SUMMARY_FILE (lcov --summary output), greps the line-coverage
# percentage, asserts it >= THRESHOLD, and prints both to stdout.

if(NOT DEFINED SUMMARY_FILE)
  message(FATAL_ERROR "coverage_check.cmake: SUMMARY_FILE must be defined")
endif()
if(NOT DEFINED THRESHOLD)
  set(THRESHOLD 80)
endif()
if(NOT EXISTS "${SUMMARY_FILE}")
  message(FATAL_ERROR "coverage_check.cmake: ${SUMMARY_FILE} does not exist")
endif()

file(READ "${SUMMARY_FILE}" _summary)
message(STATUS "Coverage summary:\n${_summary}")

# lcov summary line looks like:
#   lines......: 87.3% (1234 of 1414 lines)
# We grep for that and extract the percentage.
string(REGEX MATCH "lines\\.+: ([0-9]+\\.?[0-9]*)%" _match "${_summary}")
if(NOT _match)
  message(FATAL_ERROR
    "coverage_check.cmake: could not parse lcov summary at ${SUMMARY_FILE}\n"
    "Content was:\n${_summary}")
endif()
set(_pct "${CMAKE_MATCH_1}")
message(STATUS "First-party statement coverage: ${_pct}% (threshold ${THRESHOLD}%)")

if(_pct LESS THRESHOLD)
  message(FATAL_ERROR
    "Coverage gate FAILED: ${_pct}% < ${THRESHOLD}% threshold "
    "(NPR 7150.2D Class D minimum per ADR-013).")
endif()

message(STATUS "Coverage gate PASSED.")
