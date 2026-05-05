# cmake/apsis_compile_options.cmake
#
# Apsis first-party compile flags. Vendored upstreams (SOFA, CSPICE) and CPM
# dependencies (Eigen, GoogleTest) MUST NOT receive these flags — their code
# does not honour our warning baseline (legacy C, third-party templates).
# Only call apsis_apply_compile_options(target) on first-party targets.

function(apsis_apply_compile_options target)
  target_compile_features(${target} PUBLIC cxx_std_17)
  if(MSVC)
    target_compile_options(${target} PRIVATE
      /W4 /WX /permissive- /Zc:__cplusplus
      /utf-8)
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Werror
      -Wshadow -Wnon-virtual-dtor -Wold-style-cast
      -Wcast-align -Woverloaded-virtual -Wconversion
      -Wsign-conversion -Wdouble-promotion -Wformat=2)
  endif()
  if(APSIS_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()
