apsis_add_test(ephemeris_spice_test
  LABEL   unit
  SOURCES spice_test.cc
)

# Two-thread / two-instance test that exercises the *process-wide* CSPICE
# mutex (ADR-008). A per-instance mutex would let two SpiceEphemeris
# instances race on CSPICE's global error stack; this test fails if the
# seam regresses to per-instance scope.
apsis_add_test(ephemeris_spice_concurrent_test
  LABEL   unit
  SOURCES spice_concurrent_test.cc
)
