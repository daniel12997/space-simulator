# src/CMakeLists.txt — apsis::core, the Phase-1 propagator-core library.
#
# Phase 0 shipped only version.cc. Phase 1 grows the library by subdirectory
# (time/, frames/, force/, integrate/, ephemeris/, math/), each contributing
# sources via `target_sources()` on the same `apsis_core` target. The
# math/ directory is header-only (apsis::math aliases live in
# include/apsis/math/types.h) and therefore not listed below.

add_library(apsis_core STATIC
  version.cc
)

target_include_directories(apsis_core
  PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(apsis_core
  PUBLIC
    apsis::eigen
  PRIVATE
    apsis::sofa
)

# SOFA headers are C; consumers that include "sofa.h" need to know it lives
# under external/sofa/include/. Linking apsis::sofa already sets that up via
# its INTERFACE include directory, but we mark it SYSTEM so the strict warning
# flags applied to apsis_core don't fire on SOFA's K&R-flavour declarations.
target_include_directories(apsis_core SYSTEM PRIVATE
  $<TARGET_PROPERTY:apsis_sofa,INTERFACE_INCLUDE_DIRECTORIES>
)

apsis_apply_compile_options(apsis_core)

add_library(apsis::core ALIAS apsis_core)

# Subdirectories add their sources via target_sources(apsis_core ...).
add_subdirectory(time)
add_subdirectory(frames)
add_subdirectory(ephemeris)
add_subdirectory(force)
add_subdirectory(math)
add_subdirectory(integrate)
