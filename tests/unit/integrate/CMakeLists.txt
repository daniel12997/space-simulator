apsis_add_test(integrate_encke_test
  LABEL   unit
  SOURCES encke_test.cc
)

# encke_test.cc pulls the internal f-and-g header from src/math/. Add the
# src/ root to its private include path so the relative include resolves.
target_include_directories(integrate_encke_test PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)
