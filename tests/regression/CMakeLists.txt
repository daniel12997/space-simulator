# tests/regression/CMakeLists.txt — Phase-1 regression tests.

apsis_add_test(jpl_de_roundtrip
  LABEL   regression
  SOURCES jpl_de_roundtrip.cc
)
target_compile_definitions(jpl_de_roundtrip PRIVATE
  APSIS_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

# iss_vector regression deleted in the GJ8-stub-removal commit and
# replaced in the immediately following commit by a LEO Kepler closed-
# form comparison (leo_kepler_24h). Real ISS state-vector reproduction
# against published NASA reference data is deferred to Phase 7.
