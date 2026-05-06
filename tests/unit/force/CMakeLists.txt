apsis_add_test(force_point_mass_test
  LABEL   unit
  SOURCES point_mass_test.cc
)

apsis_add_test(force_spherical_harmonic_test
  LABEL   unit
  SOURCES spherical_harmonic_test.cc
)

apsis_add_test(force_third_body_test
  LABEL   unit
  SOURCES third_body_test.cc
)
