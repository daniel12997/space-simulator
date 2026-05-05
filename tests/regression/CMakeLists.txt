# tests/regression/CMakeLists.txt — Phase-1 regression tests.

apsis_add_test(jpl_de_roundtrip
  LABEL   regression
  SOURCES jpl_de_roundtrip.cc
)
target_compile_definitions(jpl_de_roundtrip PRIVATE
  APSIS_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

# LEO Kepler 24-hour regression. Replaces the original "ISS state-vector
# reproduction" deliverable; the latter required real NASA-published
# reference vectors and is deferred to Phase 7. This test asserts Dp54
# numerical propagation matches the f-and-g closed-form Kepler oracle on
# a pure point-mass-Earth orbit, exercising the J2000 -> ICRF transform
# seam in the process. See the file header for the full deferral list.
apsis_add_test(leo_kepler_24h
  LABEL   regression
  SOURCES leo_kepler_24h.cc
)
# Pull the internal f-and-g header from src/math/.
target_include_directories(leo_kepler_24h PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)
