# tests/conformance/CMakeLists.txt — Phase-1 conformance suites.

apsis_add_test(force_model_ve_contract
  LABEL   conformance
  SOURCES force_model_ve_contract.cc
)

apsis_add_test(integrator_kepler
  LABEL   conformance
  SOURCES integrator_kepler.cc
)

apsis_add_test(integrator_phi
  LABEL   conformance
  SOURCES integrator_phi.cc
)
