# cmake/apsis_dependencies.cmake
#
# Single point of truth for CPM-fetched third-party dependencies (ADR-006).
# Vendored upstreams (SOFA, CSPICE) are handled separately in
# cmake/fetch_external.cmake + external/<lib>/CMakeLists.txt — see ADR-008.

include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

# Eigen 3.4.0 — header-only linear algebra (ADR-012).
# DOWNLOAD_ONLY because Eigen's own CMake config emits noisy targets we don't
# need for the apsis::math aliases pattern; we just want the headers.
CPMAddPackage(
  NAME            Eigen
  GITLAB_REPOSITORY libeigen/eigen
  GIT_TAG         3.4.0
  DOWNLOAD_ONLY   YES
)
add_library(apsis_eigen INTERFACE)
target_include_directories(apsis_eigen SYSTEM INTERFACE ${Eigen_SOURCE_DIR})
add_library(apsis::eigen ALIAS apsis_eigen)

# GoogleTest v1.15.2 (ADR-007). Skipped when tests are disabled.
if(APSIS_BUILD_TESTS)
  CPMAddPackage(
    NAME            GoogleTest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG         v1.15.2
    OPTIONS
      "INSTALL_GTEST OFF"
      "BUILD_GMOCK ON"
      "gtest_force_shared_crt ON"
  )
endif()
