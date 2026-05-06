# src/frames/CMakeLists.txt — Phase 1 §3: frame transforms.

target_sources(apsis_core
  PRIVATE
    icrs_itrs.cc
    frame_bias.cc
    teme.cc
)
