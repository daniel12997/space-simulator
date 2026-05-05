# external/cspice/CMakeLists.txt — wrapper for vendored JPL CSPICE (ADR-008).
# Upstream tarball is not committed; cmake/fetch_external.cmake populates
# src/ and include/ on first configure from the URL + SHA-256 in PINNED_VERSION.
#
# CSPICE is f2c-translated FORTRAN: thread-unsafe by design. Per ADR-008, all
# CSPICE call sites must live in src/ephemeris/ behind a process-wide mutex.
# The lint scaffold for that rule lives in tools/lint/cspice_seam.py and is
# wired up in Phase 1.

file(GLOB APSIS_CSPICE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
if(NOT APSIS_CSPICE_SOURCES)
  message(FATAL_ERROR
    "CSPICE upstream source missing under external/cspice/src/. "
    "Reconfigure to trigger cmake/fetch_external.cmake, or check "
    "external/cspice/PINNED_VERSION.")
endif()

add_library(apsis_cspice STATIC ${APSIS_CSPICE_SOURCES})
target_include_directories(apsis_cspice
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# CSPICE is C89-style f2c output; do not apply Apsis warning baseline. As
# with SOFA, suppress upstream warnings rather than asserting them clean.
set_target_properties(apsis_cspice PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  target_compile_options(apsis_cspice PRIVATE /w)
  # CSPICE's f2c-output trips on MSVC's secure-CRT lints if WX is added; do
  # not add /WX here. The default _CRT_SECURE_NO_WARNINGS keeps things quiet.
  target_compile_definitions(apsis_cspice PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(apsis_cspice PRIVATE -w)
endif()

# CSPICE expects NON_UNIX_STDIO unset on POSIX; on Windows the upstream
# build flips that. Track if needed; default Linux build is happy without.
if(WIN32)
  target_compile_definitions(apsis_cspice PRIVATE NON_UNIX_STDIO MSDOS)
endif()

add_library(apsis::cspice ALIAS apsis_cspice)
