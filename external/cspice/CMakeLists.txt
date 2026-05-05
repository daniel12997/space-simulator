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

# CSPICE's f2c complex-math wrappers (c_cos.c, c_exp.c, c_log.c, c_sin.c,
# c_sqrt.c) include <math.h> after f2c.h. On MSVC, <math.h> transitively
# pulls in <complex.h> which defines `complex` as a macro, shadowing
# f2c.h's `typedef struct { real r, i; } complex;` and breaking these
# files at the function-signature level. The wrappers have zero callers
# within the rest of CSPICE (FORTRAN-style complex intrinsics aren't used
# by the astrodynamics code we depend on), so excluding them on MSVC is
# the narrowest fix. NAIF ships a different CSPICE tarball for Windows
# that addresses this differently; switching to that is the long-term
# correct path (tracked as a Phase 1 followup).
if(MSVC)
  list(FILTER APSIS_CSPICE_SOURCES EXCLUDE REGEX
    "/(c_cos|c_exp|c_log|c_sin|c_sqrt)\\.c$")
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
  # -w suppresses warnings, but clang-15+ promotes several K&R-era diagnostics
  # to errors by default; CSPICE's f2c-translated source has implicit-int
  # function declarations (e.g. src/dfe.c) that fail clang's stricter default.
  # Demote those back to warnings (which -w then silences). These three are
  # accepted by both gcc and clang.
  target_compile_options(apsis_cspice PRIVATE
    -w
    -Wno-error=implicit-int
    -Wno-error=implicit-function-declaration
    -Wno-error=int-conversion)
  # -Wincompatible-function-pointer-types is clang-only — gcc errors on the
  # name itself ('no option -Wincompatible-function-pointer-types'). Apply
  # only to clang.
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(apsis_cspice PRIVATE
      -Wno-error=incompatible-function-pointer-types)
  endif()
endif()

# CSPICE expects NON_UNIX_STDIO unset on POSIX; on Windows the upstream
# build flips that. Track if needed; default Linux build is happy without.
if(WIN32)
  target_compile_definitions(apsis_cspice PRIVATE NON_UNIX_STDIO MSDOS)
endif()

add_library(apsis::cspice ALIAS apsis_cspice)
