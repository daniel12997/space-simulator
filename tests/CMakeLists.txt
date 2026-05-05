# tests/CMakeLists.txt — Apsis test suite root.
#
# Provides apsis_add_test(name LABEL <label> SOURCES ...) which:
#   - declares the test executable
#   - links GoogleTest's gtest_main and apsis::core
#   - applies Apsis compile options (warnings-as-errors)
#   - registers with CTest under the given label
#
# Suites are organised under tests/{smoke,unit,conformance,regression}/ per
# ADR-007. Phase 0 ships the smoke suite only.

include(GoogleTest)

function(apsis_add_test name)
  set(_options)
  set(_one    LABEL)
  set(_multi  SOURCES)
  cmake_parse_arguments(APSIS_TEST "${_options}" "${_one}" "${_multi}" ${ARGN})

  if(NOT APSIS_TEST_SOURCES)
    message(FATAL_ERROR "apsis_add_test(${name}): SOURCES is required")
  endif()
  if(NOT APSIS_TEST_LABEL)
    message(FATAL_ERROR "apsis_add_test(${name}): LABEL is required")
  endif()

  add_executable(${name} ${APSIS_TEST_SOURCES})
  target_link_libraries(${name}
    PRIVATE
      apsis::core
      GTest::gtest
      GTest::gtest_main
  )
  apsis_apply_compile_options(${name})

  gtest_discover_tests(${name}
    PROPERTIES LABELS "${APSIS_TEST_LABEL}"
    DISCOVERY_MODE PRE_TEST
  )
endfunction()

add_subdirectory(smoke)
