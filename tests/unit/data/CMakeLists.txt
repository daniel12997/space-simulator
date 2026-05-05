apsis_add_test(data_integrity_test
  LABEL   unit
  SOURCES integrity_test.cc
)

# Pass the data dir as a compile-time constant so the test can find the
# files without depending on cwd.
target_compile_definitions(data_integrity_test PRIVATE
  APSIS_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)
