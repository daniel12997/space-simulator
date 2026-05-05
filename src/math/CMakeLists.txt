# src/math/CMakeLists.txt — Phase 1 §7: internal-only math helpers.

target_sources(apsis_core
  PRIVATE
    f_and_g_series.cc
)

# f_and_g_series.h is internal — Encke needs it but no public consumer
# does. We include directly via relative path from src/integrate/.
