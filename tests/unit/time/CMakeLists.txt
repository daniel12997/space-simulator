apsis_add_test(time_test
  LABEL   unit
  SOURCES time_test.cc
)

# Compile-fail test for cross-scale arithmetic. Implemented via try_compile
# with `EXPECT_FAIL` semantics: we configure a tiny project that uses two
# Time<> instantiations of different scales in an arithmetic context and
# assert that compilation FAILS. If it ever starts compiling, an integrity
# pillar of ADR-003/010 has been broken.
#
# NOTE: per the Phase 1 plan's "Out of scope" section, if try_compile-with-
# EXPECT_FAIL proves brittle on Windows, this test is allowed to be Linux-only.
# Phase 1's CI matrix is Linux-only already; we apply no platform guard here.
set(_compile_fail_src "${CMAKE_CURRENT_SOURCE_DIR}/scale_mixing_compile_fail.cc")

try_compile(
  APSIS_SCALE_MIX_COMPILES
  "${CMAKE_CURRENT_BINARY_DIR}/scale_mixing_try"
  SOURCES "${_compile_fail_src}"
  CMAKE_FLAGS
    "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR}/include"
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  OUTPUT_VARIABLE APSIS_SCALE_MIX_OUTPUT
)
if(APSIS_SCALE_MIX_COMPILES)
  message(FATAL_ERROR
    "scale_mixing_compile_fail.cc was expected to FAIL to compile (Time<TAI> "
    "+ Time<UTC> must be a compile-time error), but it succeeded. "
    "Check include/apsis/time/time.h for an inadvertently overgeneral operator. "
    "Compiler output follows:\n${APSIS_SCALE_MIX_OUTPUT}")
else()
  message(STATUS
    "[apsis] scale_mixing_compile_fail.cc rejected by compiler as expected.")
endif()

# Register a synthetic gtest that records the (compile-time) outcome so it
# shows up in ctest output with the unit label. The test itself just records
# a passing result; the actual gate ran at configure time above.
apsis_add_test(time_scale_mixing_guard
  LABEL   unit
  SOURCES scale_mixing_guard.cc
)
