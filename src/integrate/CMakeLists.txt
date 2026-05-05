# src/integrate/CMakeLists.txt — Phase 1 §6: integrator seam + adapters.

target_sources(apsis_core
  PRIVATE
    dop853.cc
    yoshida4.cc
    gauss_jackson_8.cc
    encke_wrapper.cc
)
