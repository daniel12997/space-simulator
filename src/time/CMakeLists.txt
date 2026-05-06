# src/time/CMakeLists.txt — Phase 1 §2: time-scale conversions.
# Phase 1A §B1: + EopTable.

target_sources(apsis_core
  PRIVATE
    convert.cc
    eop_table.cc
)
