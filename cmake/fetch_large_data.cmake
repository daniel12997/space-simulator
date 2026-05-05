# cmake/fetch_large_data.cmake
#
# Tier-2 reference data fetch (ADR-011). Stub for Phase 0; populated when
# Phase 1 needs DE441 / full EGM2008 / live IERS finals. All entries here
# are gated on APSIS_FETCH_LARGE_DATA=ON and use file(DOWNLOAD ...
# EXPECTED_HASH SHA256=...) so an upstream re-issue does not silently
# change test outcomes.
#
# Tier-1 (small, byte-stable) reference data lives committed under data/
# and is verified by data/SHA256SUMS at test runtime; that integrity check
# is wired up in Phase 1 alongside the data files themselves.

if(NOT APSIS_FETCH_LARGE_DATA)
  return()
endif()

message(STATUS "[apsis] APSIS_FETCH_LARGE_DATA=ON, but no Tier-2 fetches are "
               "registered in Phase 0 yet. Add file(DOWNLOAD ...) entries "
               "here when Phase 1 lands.")
