apsis_add_test(frames_transform_test
  LABEL   unit
  SOURCES transform_test.cc
)

apsis_add_test(frames_round_trip_test
  LABEL   unit
  SOURCES round_trip_test.cc
)

# Compile-fail test for cross-frame mixing. Implemented via try_compile
# with `EXPECT_FAIL` semantics (mirroring tests/unit/time/scale_mixing):
# we configure a tiny project that instantiates `transform<ICRF, ICRF>`
# but passes a `State<ITRS>&` argument and assert that compilation FAILS.
# If it ever starts compiling, the type-level frame-mixing pillar of
# ADR-010 has been broken.
set(_frame_mix_src "${CMAKE_CURRENT_SOURCE_DIR}/frame_mixing_compile_fail.cc")

try_compile(
  APSIS_FRAME_MIX_COMPILES
  "${CMAKE_CURRENT_BINARY_DIR}/frame_mixing_try"
  SOURCES "${_frame_mix_src}"
  CMAKE_FLAGS
    "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR}/include;${Eigen_SOURCE_DIR}"
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  OUTPUT_VARIABLE APSIS_FRAME_MIX_OUTPUT
)
if(APSIS_FRAME_MIX_COMPILES)
  message(FATAL_ERROR
    "frame_mixing_compile_fail.cc was expected to FAIL to compile "
    "(transform<ICRF, ICRF>(State<ITRS>&, ...) must be a compile-time "
    "error), but it succeeded. Check include/apsis/frames/transform.h "
    "for an inadvertently overgeneral overload. Compiler output follows:\n"
    "${APSIS_FRAME_MIX_OUTPUT}")
else()
  message(STATUS
    "[apsis] frame_mixing_compile_fail.cc rejected by compiler as expected.")
endif()

# Register a synthetic gtest that records the (compile-time) outcome so it
# shows up in ctest output with the unit label. The test itself just records
# a passing result; the actual gate ran at configure time above.
apsis_add_test(frames_frame_mixing_guard
  LABEL   unit
  SOURCES frame_mixing_guard.cc
)
