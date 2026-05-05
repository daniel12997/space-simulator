cmake_minimum_required(VERSION 3.25)
project(apsis VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Phase 0 ships an empty library skeleton plus a smoke test that links it.
# Per ADR-006 (CMake + CPM), ADR-007 (GoogleTest), ADR-008 (vendor SOFA/CSPICE),
# ADR-011 (large-data tier 2), ADR-012 (Eigen + apsis::math aliases).
option(APSIS_BUILD_TESTS       "Build the Apsis test suite"                    ON)
option(APSIS_FETCH_LARGE_DATA  "Fetch large reference data (DE441, full EGM2008)" OFF)
option(APSIS_ENABLE_SANITIZERS "Enable ASan+UBSan in non-Release builds"       OFF)
option(APSIS_ENABLE_COVERAGE   "Enable gcov coverage instrumentation (ADR-013)" OFF)

include(cmake/apsis_compile_options.cmake)
include(cmake/coverage.cmake)

# Pull SOFA / CSPICE upstream tarballs into external/_cache and extract them
# under external/<lib>/{src,include,...}. Idempotent: skipped when the
# _extracted_marker is already present and the SHA matches PINNED_VERSION.
include(cmake/fetch_external.cmake)

# CPM-fetched dependencies (Eigen, GoogleTest).
include(cmake/apsis_dependencies.cmake)

# Phase 1+ adds DE-kernel and EGM-coefficient fetches behind APSIS_FETCH_LARGE_DATA.
include(cmake/fetch_large_data.cmake)

add_subdirectory(external/sofa)
add_subdirectory(external/cspice)
add_subdirectory(src)

if(APSIS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
