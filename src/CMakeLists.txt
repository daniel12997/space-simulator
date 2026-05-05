# src/CMakeLists.txt — apsis::core skeleton.
#
# Phase 0 ships only version.cc. Phase 1 will replace this with proper
# subdirectory structure (math/, time/, frames/, force/, integrators/).

add_library(apsis_core STATIC
  version.cc
)

target_include_directories(apsis_core
  PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(apsis_core
  PUBLIC
    apsis::eigen
)

apsis_apply_compile_options(apsis_core)

add_library(apsis::core ALIAS apsis_core)
