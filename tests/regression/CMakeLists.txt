# tests/regression/CMakeLists.txt — Phase-1 regression tests.

apsis_add_test(jpl_de_roundtrip
  LABEL   regression
  SOURCES jpl_de_roundtrip.cc
)
target_compile_definitions(jpl_de_roundtrip PRIVATE
  APSIS_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

apsis_add_test(iss_vector
  LABEL   regression
  SOURCES iss_vector.cc
)
target_compile_definitions(iss_vector PRIVATE
  APSIS_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)
