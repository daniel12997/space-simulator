# tests/unit/CMakeLists.txt — Phase-1 unit suites.
#
# Each per-subsystem subdirectory adds its own gtest binaries via
# apsis_add_test(... LABEL unit ...). Subdirectories are added as their
# corresponding source modules land; an unconditional add_subdirectory()
# of an empty directory would error.

add_subdirectory(math)
add_subdirectory(time)
add_subdirectory(frames)
add_subdirectory(ephemeris)
add_subdirectory(force)
add_subdirectory(integrate)
