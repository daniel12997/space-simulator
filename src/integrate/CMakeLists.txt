# src/integrate/CMakeLists.txt — Phase 1 §6: integrator seam + adapters.

target_sources(apsis_core
  PRIVATE
    dp54.cc
    dop853.cc
    yoshida4.cc
    encke_wrapper.cc
    gauss_jackson_8.cc
    gj8_starter.cc
)
