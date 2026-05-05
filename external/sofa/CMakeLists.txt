# external/sofa/CMakeLists.txt — wrapper for vendored IAU SOFA (ADR-008).
# Upstream tarball is not committed; cmake/fetch_external.cmake populates
# src/ and include/ on first configure from the URL + SHA-256 in PINNED_VERSION.

file(GLOB APSIS_SOFA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
if(NOT APSIS_SOFA_SOURCES)
  message(FATAL_ERROR
    "SOFA upstream source missing under external/sofa/src/. "
    "Reconfigure to trigger cmake/fetch_external.cmake, or check "
    "external/sofa/PINNED_VERSION.")
endif()

add_library(apsis_sofa STATIC ${APSIS_SOFA_SOURCES})
target_include_directories(apsis_sofa
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Vendored upstream — do NOT apply Apsis warning baseline. SOFA is C89-style
# and would not survive -Wconversion / -Wsign-conversion. We mark its include
# directory SYSTEM via PUBLIC consumers (handled at the consumer site if a
# warning leak appears).
set_target_properties(apsis_sofa PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON)

# Suppress non-actionable upstream warnings without applying the project's
# warnings-as-errors. Apsis sources (which DO see -Werror) never include
# SOFA headers transitively without the SYSTEM marker on the consumer side.
if(MSVC)
  target_compile_options(apsis_sofa PRIVATE /w)
else()
  # See cspice/CMakeLists.txt for rationale: clang-15+ promotes K&R-era C
  # diagnostics to errors by default; -w doesn't help. Demote back to warnings
  # (silenced by -w). SOFA is modern enough to not currently trip these, but
  # apply the same defensive flags so future SOFA releases or alt toolchains
  # don't surprise the build. The -Wno-error=incompatible-function-pointer-types
  # form is clang-only (gcc errors on the name); gate accordingly.
  target_compile_options(apsis_sofa PRIVATE
    -w
    -Wno-error=implicit-int
    -Wno-error=implicit-function-declaration
    -Wno-error=int-conversion)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(apsis_sofa PRIVATE
      -Wno-error=incompatible-function-pointer-types)
  endif()
endif()

add_library(apsis::sofa ALIAS apsis_sofa)
