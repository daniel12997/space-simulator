# Apsis Software Test Plan (STP)

> **Class D** under NPR 7150.2D. Companion to the
> [Software Management Plan](software-management-plan.md) and the
> [Software Development Plan](software-development-plan.md). Where the
> SMP describes *how the project is run* and the SDP describes *how
> development happens*, this document describes *how testing happens*:
> the strategy, the categories, the coverage targets, the regression
> cases, and the reporting cadence.
>
> Skeleton document — sections marked **TBD** are deferred to the
> triggering event named inline.

## 1. Test strategy

The Apsis test surface follows the design overview's pattern:
**conformance via the interface, not past it.** Every seam (`IForceModel`,
`IIntegrator`, `IBroadPhase`, `IAttitudeEstimator`, …) has a single
parameterised conformance test that runs against every adapter that
claims to implement it. Adapters that do not pass the conformance test
are not admitted to the seam.

This conformance discipline is the spine of test rigor; coverage and
regression cases are the safety net.

## 2. Test categories

| Label         | What                                                                              | When executed                  |
| ------------- | --------------------------------------------------------------------------------- | ------------------------------ |
| `smoke`       | Library skeleton compiles and links; `apsis::version()` returns non-empty.        | every CI run                   |
| `unit`        | Per-component tests in `tests/unit/<module>/`; pure logic, no I/O.                | every CI run                   |
| `conformance` | Parameterised tests against every adapter behind a seam interface.                | every CI run                   |
| `regression`  | End-to-end propagation / screening / GNC scenarios against published references. | every CI run + nightly         |
| `sanitizer`   | All of the above under ASan + UBSan (clang).                                      | every CI run (dedicated job)   |
| `bench`       | Performance benchmarks against perf budgets in `docs/REQUIREMENTS.md`.            | nightly + on-demand            |

CTest labels are the selection mechanism (`ctest -L conformance`
etc.).

## 3. Coverage targets

- **First-party code only** (`src/`, `include/apsis/`). Vendored
  SOFA, CSPICE, Eigen, GoogleTest are excluded from coverage
  measurement.
- **Statement coverage**: ≥ **80%** as the Class D minimum.
- **Branch coverage**: tracked but not gated. Class D does not require
  MC/DC.
- **Tooling**: gcov + lcov in the gcc-13 CI job; coverage report
  uploaded as a CI artefact and rendered in PR comments via a small
  bot. **TBD** — bot wiring; lands as part of the Phase 1 followup that
  introduces the gcov gate.
- **Trend reporting**: per-PR coverage delta visible alongside the
  diff. **TBD**.
- **Coverage gate** (Phase 1 followup): CI fails the gcc-13 job if
  first-party statement coverage drops below 80% on `main` or below
  the merge-base coverage on a PR.

## 4. Reference regression cases

Per `docs/structure.md` and the per-phase plans:

| Case                                       | Phase | Tolerance                                       | Source data                                    |
| ------------------------------------------ | ----- | ----------------------------------------------- | ---------------------------------------------- |
| Two-body Keplerian (energy / momentum)     | 1     | conservation invariants to integrator tolerance | synthetic (computed at test runtime)           |
| JPL DE round-trip (Earth, 10 yr)            | 1     | 100 km position, 1 mm/s velocity (relaxed)     | `data/de440_phase1.bsp` (Tier 1)               |
| ISS state-vector forward 24 h              | 1     | 50 m / 5 cm/s (or 5 km / 5 m/s if drag dominates) | `data/iss_ref_vectors.json`                  |
| 50 k synthetic catalog conjunction screen   | 2     | candidate-pair set agrees across broad-phases   | synthetic                                      |
| Historical conjunction reproduction         | 2     | miss distance within published tolerance        | historical TLE pair                            |
| Articulated-body angular momentum           | 3     | conservation to integrator tolerance            | synthetic                                      |
| Closed-loop attitude slew                   | 4     | converges within spec                           | synthetic                                      |
| 1 k-trial MC dispersion (deterministic)     | 5     | bit-identical across thread counts              | synthetic + fixed campaign seed                |
| Six example-scenario E2E                    | 6     | each scenario completes                         | scenarios under `examples/`                    |

Each regression case fails the build if its tolerance is not met
(except where the plan documents an acceptable widening — e.g. ISS
24h drift if drag absence dominates the residual).

## 5. Test environment

- Local developer machine: same toolchain as the gcc-13 CI job
  suffices; clang-17 + sanitizers if exercising the sanitizer build.
- CI: GitHub Actions, ubuntu-24.04 runners, three-job matrix
  (gcc-13, clang-17, clang-17 sanitizers).
- Test data: small Tier 1 vendored under `data/`; large Tier 2
  fetched at configure time when `APSIS_FETCH_LARGE_DATA=ON`. Tier 2
  tests are tagged `requires_large_data` and are skipped when not
  fetched.

## 6. Test admission and review

- **New seam adapter** is admitted only if it passes the seam's
  conformance test. The conformance test itself is reviewed
  separately when the seam is introduced and is updated only via PR
  with explicit reviewer signoff (it is the load-bearing contract).
- **New regression case** lands alongside the feature it regresses
  against; tolerance is justified in the test file's header comment
  with reference to a published source (e.g. `// JPL DE440 Earth
  ephemeris reference; tolerance per design overview §"Long-arc
  precision"`).
- **Test failures** triage: a red test is investigated before being
  skipped, suppressed, or relaxed. If a tolerance must be widened,
  the widening lands as a PR with justification in the test file
  comment and a corresponding line in `docs/wiki/log.md` recording
  the rationale.

## 7. Requirements traceability

Each `REQ-*` ID in `docs/REQUIREMENTS.md` is traced to a covering
test by:

- **Frontmatter / comment** in the test file: `// requirements:
  REQ-INT-001, REQ-INT-006`.
- A `tools/lint/req_traceability.py` script (Phase 1 followup) that
  scans tests, builds a REQ → test map, and reports REQ IDs without
  coverage.
- The traceability matrix is regenerated on each PR and made
  available as a CI artefact. **TBD** — script wiring; Phase 1
  followup tracked in [issue TBD].

ADR frontmatter (`requirements: [REQ-*]`) provides the
decision-to-requirement direction. The traceability script closes
the loop by tying REQs to tests.

## 8. Reporting

- **Per CI run**: PR comment summarises green / red counts per label,
  coverage delta, sanitizer-issue count.
- **Per phase boundary**: phase plan document checkboxes reflect
  test-pass status. The phase isn't complete until every checkbox is
  ticked.
- **Per release** (Phase 6+): `docs/RELEASE_NOTES.md` summarises
  passing reference cases and any tolerance widenings since the
  prior release. **TBD** — release process at Phase 6.

## 9. Known limitations

- **Class D does not require MC/DC** coverage. Apsis does not measure
  it. If a downstream consumer requires MC/DC on a derived
  component, that component is reclassified per ADR-013's
  reclassification clause.
- **No formal methods.** Apsis does not employ formal verification
  tools (Frama-C, TLA+, etc.) at any layer. Conservation invariants
  and conformance tests are the validation surface.
- **Independent V&V is not part of the Apsis process.** Internal
  review by the project lead and SME reviewers per PR is the
  validation step. Class D does not require IV&V.

## 10. References

- [`software-management-plan.md`](software-management-plan.md) — companion management plan
- [`software-development-plan.md`](software-development-plan.md) — companion development plan
- [`decisions/013-class-d-software-classification`](../wiki/decisions/013-class-d-software-classification.md) — classification justification
- [`docs/REQUIREMENTS.md`](../REQUIREMENTS.md) — authoritative requirements (REQ-* IDs)
- [`docs/structure.md`](../structure.md) — phase-level test gates
- [`docs/00-design-overview.md`](../00-design-overview.md) — long-arc precision claims under test
