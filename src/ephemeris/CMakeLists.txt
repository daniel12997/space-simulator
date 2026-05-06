# src/ephemeris/CMakeLists.txt — Phase 1 §4: SPICE-mediated ephemeris seam.
#
# This is the ONLY directory in the project that calls CSPICE. The
# tools/lint/cspice_seam.py CI lint enforces it.

target_sources(apsis_core
  PRIVATE
    spice_ephemeris.cc
)

# Pull in CSPICE as a private link dep on apsis_core. Headers are in
# external/cspice/include/.
target_link_libraries(apsis_core PRIVATE apsis::cspice)
target_include_directories(apsis_core SYSTEM PRIVATE
  $<TARGET_PROPERTY:apsis_cspice,INTERFACE_INCLUDE_DIRECTORIES>
)
