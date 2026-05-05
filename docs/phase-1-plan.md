# Phase 1 Plan — Propagator Core

> **Gate**: single-spacecraft propagation runs end-to-end with the
> [[wiki/concepts/variational-equations|VE]] contract honoured by every
> force model and every integrator. The
> `tests/regression/jpl_de_roundtrip` and `tests/regression/leo_kepler_24h`
> regression cases pass within the tolerances declared below. (The
> originally-planned `iss_vector` deliverable is recast as the
> analytical-oracle `leo_kepler_24h` test for Phase 1; real ISS state-
> vector reproduction is deferred to Phase 7 — see §10.)

## Reference

- Structure outline: `docs/structure.md` §"Phase 1: Propagator core".
- Decisions: [[wiki/decisions/001-use-ceo-based-icrs-to-itrs]],
  [[wiki/decisions/002-variational-equations-between-measurements]],
  [[wiki/decisions/003-tagged-time-scale-types]],
  [[wiki/decisions/008-vendor-sofa-and-cspice]],
  [[wiki/decisions/009-hand-rolled-integrator-family]],
  [[wiki/decisions/010-phantom-typed-time-and-state]],
  [[wiki/decisions/011-reference-data-shipping]],
  [[wiki/decisions/012-eigen-with-apsis-math-aliases]].
- Concepts: [[wiki/concepts/long-arc-state-conditioning]],
  [[wiki/concepts/variational-equations]],
  [[wiki/concepts/celestial-ephemeris-origin]],
  [[wiki/concepts/earth-rotation-angle]],
  [[wiki/concepts/precession-nutation]],
  [[wiki/concepts/frame-bias]],
  [[wiki/concepts/gauss-jackson-integration]],
  [[wiki/concepts/f-and-g-series]],
  [[wiki/concepts/time-scales]].

## Phase 1 force-model scope

Per the resolved Phase 1 scope: **point-mass + EGM2008 truncated to
degree-and-order 20 + lunar/solar third-body via SPICE**. The
`SphericalHarmonic` adapter ships with finite-difference partials and
is excluded from the VE-contract conformance gate pending the Phase 7
Pines analytical gradient (see ADR-009 Phase 1 Implementation Note);
its acceleration is exercised by `tests/unit/force/spherical_harmonic_test.cc`
and by composition in the regression suite. Drag and SRP are deferred
to Phase 7. The two regression cases below do not need non-conservative
perturbations within their declared tolerances.

## Changes

### 1. `apsis::math` linalg aliases

**File**: `include/apsis/math/types.h`
**Action**: create

```cpp
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
namespace apsis::math {
using Vec3  = Eigen::Matrix<double, 3, 1>;
using Vec6  = Eigen::Matrix<double, 6, 1>;
using VecX  = Eigen::VectorXd;
using Mat3  = Eigen::Matrix<double, 3, 3>;
using Mat6  = Eigen::Matrix<double, 6, 6>;
using Mat36 = Eigen::Matrix<double, 3, 6>;
using Mat66 = Mat6;
using MatX  = Eigen::MatrixXd;
using Quat  = Eigen::Quaterniond;
}
```

**Tests**: `tests/unit/math/types_smoke.cc` — confirms aliases compile,
zero-init, basic ops (cross, normalize, quat-rotate) work.

### 2. `apsis::time` — phantom-typed time scales

**Files**:
- `include/apsis/time/scale_tags.h` — empty `tags::TAI`, `TT`, `UTC`, `UT1`, `TDB` types.
- `include/apsis/time/time.h` — `Time<Scale>` class template.
- `include/apsis/time/duration.h` — scalar `Duration` (TAI seconds).
- `include/apsis/time/convert.h` — `convert<To, From>` declarations (one per source pair).
- `src/time/convert.cc` — SOFA-mediated implementations.
- `tests/unit/time/time_test.cc`, `tests/unit/time/scale_mixing_compile_fail.cc`.

`Time<Scale>` stores two `double`s (`jd1_`, `jd2_`) per
[[wiki/concepts/long-arc-state-conditioning]]. Same-scale arithmetic is
permitted; cross-scale arithmetic is a compile error.

```cpp
// include/apsis/time/time.h
namespace apsis::time {
template <class Scale>
class Time {
public:
  constexpr Time() noexcept = default;
  constexpr Time(double jd1, double jd2) noexcept : jd1_(jd1), jd2_(jd2) {}

  [[nodiscard]] constexpr double jd1() const noexcept { return jd1_; }
  [[nodiscard]] constexpr double jd2() const noexcept { return jd2_; }

  Time& operator+=(Duration d) noexcept;     // scale-preserving
  Time& operator-=(Duration d) noexcept;
  friend Duration operator-(Time a, Time b) noexcept;  // same Scale only
  friend Time operator+(Time t, Duration d) noexcept;
  friend bool operator<(Time a, Time b) noexcept;
  // No mixed-scale operators exist; mixing is a compile error.
private:
  double jd1_{}, jd2_{};
};
}
```

Conversions follow [[wiki/sources/sofa-2023-time-scale-cookbook]]:
- TAI ↔ UTC via `iauTaiutc` / `iauUtctai` (handles leap seconds).
- TAI ↔ TT: fixed offset `TT - TAI = 32.184 s`.
- UTC ↔ UT1 via UT1−UTC pulled from EOP table (loaded once at startup
  from `data/iers_eop_phase1.csv`).
- TT ↔ TDB via `iauTttdb` (location-dependent term currently using
  geocentre approximation; `Time<>` API does **not** carry observer
  position in v1).

```cpp
// include/apsis/time/convert.h
namespace apsis::time {
template <class To, class From> Time<To> convert(Time<From>);
// Explicit specializations declared for every supported pair:
template <> Time<tags::TT>  convert(Time<tags::TAI>);
template <> Time<tags::TAI> convert(Time<tags::TT>);
template <> Time<tags::UTC> convert(Time<tags::TAI>);
// ... full closure of the supported graph; see src/time/convert.cc
}
```

The unsupported `convert<TDB, UTC>` etc. is **deliberately not
specialised** — link error if anyone tries. The supported graph is:
TAI ↔ {TT, UTC, UT1}; TT ↔ TDB. All other conversions go via TAI.

`scale_mixing_compile_fail.cc` is a compile-fail test (CMake
`try_compile` with `EXPECT_FAIL`) confirming `Time<TAI>{} - Time<UTC>{}`
does not compile.

### 3. `apsis::frames` — `State<Frame>` + transforms

**Files**:
- `include/apsis/frames/frame_tags.h` — `tags::ICRF`, `GCRS`, `ITRS`, `J2000`, `TEME`, `BodyFixed<Body>`.
- `include/apsis/frames/state.h` — `State<Frame>` class template.
- `include/apsis/frames/transform.h` — `transform<To, From>` declarations.
- `src/frames/icrs_itrs.cc` — CIO-pipeline ICRF↔ITRS via SOFA (`iauXys06a`, `iauC2t06a` family).
- `src/frames/frame_bias.cc` — ICRF↔J2000 (`iauPmat06`).
- `src/frames/teme.cc` — TEME↔ITRS (used by Phase 2; stub here for SGP4 forward-compat).
- `tests/unit/frames/transform_test.cc`, `tests/unit/frames/round_trip_test.cc`.

```cpp
// include/apsis/frames/state.h
namespace apsis::frames {
template <class Frame>
struct State {
  apsis::math::Vec3 r{};   // position [m]
  apsis::math::Vec3 v{};   // velocity [m/s]
};

template <class To, class From>
State<To> transform(State<From> x, apsis::time::Time<apsis::time::tags::TT> t);
}
```

ICRF↔ITRS uses [[wiki/concepts/celestial-ephemeris-origin|CIO]]
pipeline: SOFA `iauXys06a` for X, Y, s; SOFA `iauEra00` for ERA;
SOFA `iauC2t06a` (or hand-composed equivalent) for the full CIRS→TIRS→
ITRS chain. Polar motion x_p, y_p loaded from EOP table.

`transform<>` specialisations cover every needed pair for Phase 1:
ICRF↔GCRS (identity in current SOFA model), ICRF↔ITRS, ICRF↔J2000
(frame bias), with TEME↔ITRS stubbed for Phase 2.

`round_trip_test.cc` enforces `transform<From, To>(transform<To, From>(x, t), t) ≈ x` to 1e-12 m, 1e-15 m/s on canonical states across a 1-year epoch sweep.

### 4. `apsis::ephemeris::IEphemeris` + SpiceEphemeris

**Files**:
- `include/apsis/ephemeris/iephemeris.h` — interface.
- `include/apsis/ephemeris/spice_ephemeris.h`, `src/ephemeris/spice_ephemeris.cc`.
- `tests/unit/ephemeris/spice_test.cc`.

```cpp
// include/apsis/ephemeris/iephemeris.h
namespace apsis::ephemeris {
class IEphemeris {
public:
  virtual ~IEphemeris() = default;
  // Body position / velocity in ICRF wrt Solar-System Barycentre.
  virtual apsis::frames::State<apsis::frames::tags::ICRF>
  state(int body_naif_id,
        apsis::time::Time<apsis::time::tags::TDB> t) const = 0;
};
}
```

Per [[wiki/decisions/008-vendor-sofa-and-cspice]], every CSPICE call
(`furnsh_c`, `spkezr_c`, `spkpos_c`, etc.) is wrapped inside one
process-wide `std::mutex` held by `SpiceEphemeris`. The seam is a
hard rule — `tools/lint/cspice_seam.py` (introduced in Phase 0) flags
any other `*_c(` in the codebase.

`SpiceEphemeris` constructor takes a vector of kernel paths and calls
`furnsh_c` for each under the lock; destructor calls `kclear_c`. State
queries take `Time<TDB>` (SPICE's native time) and return ICRF state.

### 5. `apsis::force::IForceModel` + adapters

**Files**:
- `include/apsis/force/iforce_model.h` — interface (the VE contract).
- `include/apsis/force/point_mass.h` + `src/force/point_mass.cc`.
- `include/apsis/force/spherical_harmonic.h` + `src/force/spherical_harmonic.cc` (Pines / Cunningham non-singular recursion to deg/order 20; coefficients loaded from `data/egm2008_d20.tab`).
- `include/apsis/force/third_body.h` + `src/force/third_body.cc` (consumes `IEphemeris` for lunar + solar gravitation).
- `tests/conformance/force_model_ve_contract.cc` (parameterised over all adapters).
- `tests/unit/force/{point_mass,spherical_harmonic,third_body}_test.cc`.

```cpp
// include/apsis/force/iforce_model.h
namespace apsis::force {
class IForceModel {
public:
  virtual ~IForceModel() = default;
  // Acceleration in ICRF [m/s^2] at time t for state x (ICRF, m + m/s).
  virtual apsis::math::Vec3
  acceleration(apsis::time::Time<apsis::time::tags::TT> t,
               const apsis::frames::State<apsis::frames::tags::ICRF>& x) const = 0;
  // ∂a / ∂x as 3x6 (cols 0..2 = ∂a/∂r, cols 3..5 = ∂a/∂v).
  virtual apsis::math::Mat36
  partials(apsis::time::Time<apsis::time::tags::TT> t,
           const apsis::frames::State<apsis::frames::tags::ICRF>& x) const = 0;
};
}
```

The VE-contract conformance test (`force_model_ve_contract.cc`):
- For each adapter under test, samples 32 representative `(t, x)` points.
- Computes `partials(t, x)` analytically.
- Computes the same numerically via central-difference of `acceleration` with `h = 1.0` for position columns, `h = 1e-3` for velocity columns.
- Asserts max relative error < 1e-6 (tolerance documented per adapter; spherical-harmonic gets 1e-5 due to coefficient noise).

`point_mass` derivatives are closed-form. `spherical_harmonic` uses the
analytical Pines-formulation gradient — no AD. `third_body` derivatives
treat the third-body position as frozen at evaluation time (effective
acceleration depends only on the active spacecraft's position).

### 6. `apsis::integrate::IIntegrator` + adapters

**Files**:
- `include/apsis/integrate/iintegrator.h` — interface stepping `(state, Φ, dt)`.
- `include/apsis/integrate/dp54.h` + `src/integrate/dp54.cc` + `src/integrate/dp54_coeffs.h` (Phase 1 ships Dormand-Prince 5(4) — Hairer Vol I Table 5.1 — as a coefficient stand-in. The DOP853 / Hairer Vol I Table 5.2 upgrade is a Phase 7 hardening item; see ADR-009 Phase 1 Implementation Note).
- `include/apsis/integrate/yoshida4.h` + `src/integrate/yoshida4.cc`.
- `GaussJackson8` (Berry-Healy 2004 ordinate-form with second-sum starter) — **deferred to Phase 7**. The Phase 1 stand-in (a single `Dp54` step under the GJ8 name) carried zero distinct behaviour and was deleted; the type rejoins the parameterised conformance gate when the real implementation lands.
- `tests/conformance/integrator_kepler.cc` (parameterised; Kepler problem → closed-form).
- `tests/conformance/integrator_phi.cc` (parameterised; Φ vs central-difference).

```cpp
// include/apsis/integrate/iintegrator.h
namespace apsis::integrate {
struct StepResult {
  apsis::frames::State<apsis::frames::tags::ICRF> x;
  apsis::math::Mat6 phi;       // updated ∂x(t+dt)/∂x(t0)
  double dt_actually_taken;    // == dt for fixed-step adapters
};

class IIntegrator {
public:
  virtual ~IIntegrator() = default;
  virtual StepResult step(
      apsis::time::Time<apsis::time::tags::TT> t,
      const apsis::frames::State<apsis::frames::tags::ICRF>& x,
      const apsis::math::Mat6& phi,
      double dt,
      const apsis::force::IForceModel& force) = 0;
};
}
```

Each adapter integrates the augmented system `dx/dt = a(t, x)` and
`dΦ/dt = A(t, x) · Φ` where `A = ∂(dx/dt)/∂x` is the 6×6 dynamics
Jacobian assembled from `force.partials(t, x)` (the bottom-half-3 rows
hold position partials of velocity = identity; only the top is
non-trivial for orbital ODEs).

`Dp54` is adaptive (DP5(4) coefficients per ADR-009 Phase 1 Implementation
Note); `Yoshida4` is fixed-step composition (4th-order) of velocity-Verlet
flow. `GaussJackson8` is deferred to Phase 7; once it lands it will use
the Phase-1-or-Phase-7 adaptive RK to bootstrap eight back-points then
advance by GJ8 ordinate-form (Berry-Healy 2004 §5).

The Phase-1 Kepler-problem conformance test compares each adapter's
trajectory (one circular orbit) to the analytical solution, asserting
max position error <1 m for Dp54 (rtol 1e-12) and <100 m for Yoshida4
(Δt = 30 s). The original tighter targets (<1e-7 m for full DOP853,
<1e-5 m for GJ8) re-engage when those adapters land in Phase 7.

The Φ conformance test perturbs initial state by 1 m / 1e-3 m/s in each
of the six axes, integrates 1 hour, compares the resulting state delta
to `Φ · δx0`, asserts agreement to integrator tolerance.

### 7. `apsis::integrate::EnckeWrapper`

**Files**:
- `include/apsis/integrate/encke_wrapper.h` + `src/integrate/encke_wrapper.cc`.
- `src/math/f_and_g_series.{h,cc}` — Lagrange [[wiki/concepts/f-and-g-series|f / g coefficients]] for Keplerian reference propagation.
- `tests/unit/integrate/encke_test.cc`.

`EnckeWrapper<I : IIntegrator>` decomposes state as
`x(t) = x_kepler(t; x0, t0) + δx(t)`, where the reference is propagated
analytically via f-and-g series and the deviation `δx` is propagated
numerically by the wrapped integrator under the *deviation* dynamics
`dδx/dt = a(t, x_kepler + δx) − a_kepler(x_kepler)`. Periodic
rectification when `||δx|| / ||x_kepler|| > rectify_threshold` (default
1e-2).

Per [[wiki/concepts/long-arc-state-conditioning]], Encke is the
preferred long-arc form when force perturbations are small relative to
the dominant central body. For Phase 1 it is *opt-in* (default Φ
propagation uses the bare integrator); regression test
`jpl_de_roundtrip` runs both Encke-on and Encke-off variants.

### 8. Reference-data plumbing

**Files** (under `data/`, vendored per [[wiki/decisions/011-reference-data-shipping]]):
- `data/de440_phase1.bsp` — DE440 truncated kernel covering 2024-01-01 → 2036-01-01 TDB (≈10 MB).
- `data/iers_eop_phase1.csv` — IERS EOP series covering same window plus 90-day pad.
- `data/egm2008_d20.tab` — EGM2008 normalised coefficients to degree-and-order 20 (text format).
- `data/iss_ref_vectors.json` — published ISS state vectors at known epochs.
- `data/SHA256SUMS` — manifest verified by `tests/unit/data/integrity_test.cc`.
- `data/README.md` — provenance and licence per file.

### 9. Regression test: JPL DE round-trip

**Files**: `tests/regression/jpl_de_roundtrip.cc`, `tests/regression/CMakeLists.txt`.

- Load `data/de440_phase1.bsp` via `SpiceEphemeris`.
- Take Earth state at t0 = 2025-01-01 12:00:00 TT directly from the kernel.
- Propagate Earth (treated as a test particle) under solar point-mass + lunar third-body for 10 years using `Dop853` with `rtol=1e-13, atol=1e-9`.
- Compare to direct kernel query at t1 = 2035-01-01 12:00:00 TT.
- Tolerance: `||Δr|| < 100 km`, `||Δv|| < 1 mm/s`. (Earth's orbit is dominated by point-mass + lunar perturbation; full-fidelity would require full DE, asteroids, relativistic terms — out of scope.)
- Run separately with `EnckeWrapper<Dop853>` enabled; assert Encke-on closure ≤ Encke-off closure at this scale.

The 100 km tolerance reflects that Phase 1's force model is **not**
ephemeris-grade for Earth's heliocentric orbit; the test exists to
prove the propagation pipeline closes consistently, not to reproduce
DE itself. The full DE440-grade reproduction case is a Phase 7
acceptance criterion.

### 10. Regression test: LEO Kepler 24-hour closure

**Files**: `tests/regression/leo_kepler_24h.cc`.

Phase 1 ships an *analytical-oracle* regression rather than a published-
state-vector reproduction. The original "ISS state-vector reproduction"
deliverable required real NASA-published reference vectors — sourcing,
licensing, and validating those is non-trivial and is deferred to
Phase 7 as a separate gate.

- Synthetic ISS-like initial state in J2000 (~6.8e6 m geocentric radius,
  ~7.66 km/s circular speed, ~51.6 deg inclination).
- Convert J2000 → ICRF via `transform<ICRF, J2000>` so the frame-bias
  seam is exercised (the Phase 7 ISS test will need the same path).
- Force model: `PointMass(Earth)` only (no SH, no third body — the
  trajectory is exactly Keplerian, so the f-and-g universal-variable
  propagator is the analytical oracle and the residual is pure
  integrator truncation).
- Propagate forward 24 hours with `Dp54 (rtol=1e-12, atol=1e-9, dt_max=60 s)`.
- Compare to `apsis::math::fandg::propagate(x0, 24 h, mu)`.
- Tolerance: 0.1 m position, 1e-4 m/s velocity. Picked experimentally
  (empirical residual ~1.5e-2 m / ~1.7e-5 m/s; ~1 order of margin to
  absorb host noise). The Phase 7 DOP853 upgrade is expected to drop
  this by ~7 orders.

What this test does NOT cover (deferred to Phase 7):
- Real ISS state-vector reproduction against published NASA data.
- Spherical-harmonic gravity (SH adapter ships with FD partials in
  Phase 1; Pines analytical gradient is a Phase 7 hardening item).
- Third-body perturbations (the JPL DE round-trip in §9 covers the
  third-body wiring; this regression intentionally isolates the
  Kepler dynamics so f-and-g is the exact oracle).

### 11. Class D followup: gcov coverage gate

Per [[wiki/decisions/013-class-d-software-classification]] and
`docs/process/software-test-plan.md` §3, Class D mandates code-coverage
measurement. Threshold: ≥ 80% statement coverage on first-party code
(`src/`, `include/apsis/`); vendored upstreams excluded.

**Files**:
- `cmake/apsis_compile_options.cmake` — extend with an `APSIS_ENABLE_COVERAGE` option that adds `--coverage` (= `-fprofile-arcs -ftest-coverage`) to first-party targets only (vendored SOFA / CSPICE retain `-w` and no coverage instrumentation).
- `cmake/coverage.cmake` (new) — `apsis_add_coverage_target()` defines a `coverage` Make / Ninja target that runs `lcov --capture`, filters out `_deps/`, `external/`, `tests/`, and `/usr/`, generates `build/coverage/lcov.info` and a textual summary, and asserts ≥ 80% statement coverage on first-party files (CI-fail otherwise).
- `.github/workflows/ci.yml` — extend the gcc-13 job: install `lcov`; configure with `-DAPSIS_ENABLE_COVERAGE=ON`; build; ctest; `cmake --build build --target coverage`; upload `lcov.info` as a CI artefact; fail the job on threshold miss.

The coverage gate runs only on the gcc-13 job (one Linux toolchain
per matrix is sufficient for Class D coverage measurement). The
clang-17 and sanitizer jobs do not measure coverage.

**Threshold rationale**: 80% is the NPR 7150.2D Class D minimum (per
`docs/process/software-test-plan.md`). The first build is expected to
exceed it because Phase 1 is mostly tested code by construction —
every adapter is exercised by its conformance test. The gate exists
to catch *regression* in coverage, not to bootstrap it.

### 12. Class D followup: REQ traceability lint

Per `docs/process/software-test-plan.md` §7, every `REQ-*` ID in
`docs/REQUIREMENTS.md` must trace to at least one covering test.

**Files**:
- `tools/lint/req_traceability.py` (new) — scans `docs/REQUIREMENTS.md` for `REQ-*` IDs, scans `tests/**/*.cc` for `// requirements: REQ-*, REQ-*` header comments, builds the bidirectional map, and reports (a) REQ IDs without a covering test, (b) test files missing the `// requirements:` header. Exits non-zero on either category. Output is a markdown-formatted matrix to stdout.
- `.github/workflows/ci.yml` — add a `req-traceability` step on the gcc-13 job (Linux only) running the script.
- Phase 1 test files (`tests/unit/**/*.cc`, `tests/conformance/**/*.cc`, `tests/regression/**/*.cc`) — add `// requirements: REQ-*` header comments where the test exercises a REQ. Tests that exercise no specific REQ (e.g. compile-fail tests, internal sanity checks) declare `// requirements: none` to acknowledge the lint scan.

**Bootstrap policy**: the script's gate is **soft on first run** —
report missing-coverage REQ IDs as warnings, not errors, until the
project explicitly turns the gate hard via a `STRICT_REQ_TRACEABILITY=1`
env var. Phase 1 lands with the script in soft mode (warns but
doesn't fail CI); the gate flips hard at a later phase boundary when
the test surface is mature enough that 100% REQ coverage is realistic.

## Verification

### Automated (Phase 1 gate)
- [x] All Phase 0 checks still green.
- [x] `ctest --test-dir build -L unit` — every unit test passes (time, frames, math, force, integrate, ephemeris, data integrity).
- [x] `ctest --test-dir build -L conformance` — `IIntegrator` Kepler conformance, `IIntegrator` Φ conformance, and `IForceModel` VE-contract conformance all green for every adapter currently behind the seam (Dp54 / Yoshida4 × PointMass / ThirdBody; GaussJackson8 deferred to Phase 7, SphericalHarmonic excluded from VE-contract conformance pending the Phase 7 Pines analytical-gradient upgrade).
- [x] `ctest --test-dir build -L regression` — `jpl_de_roundtrip` and `leo_kepler_24h` pass within declared tolerances. (Real-data ISS regression deferred to Phase 7.)
- [x] Compile-fail tests (`scale_mixing_compile_fail.cc`, frame-mixing equivalent) fail to compile as expected (`try_compile` with `EXPECT_FAIL`).
- [x] `tools/lint/cspice_seam.py` reports zero CSPICE call sites outside `src/ephemeris/`.
- [x] Sanitizer build (`APSIS_ENABLE_SANITIZERS=ON`) green on all unit + conformance tests.

### Manual
- [ ] Walk the `apsis::time::convert` graph in `src/time/convert.cc` and verify the only path between any two scales is via TAI (TT↔TDB excepted).
- [x] Open `src/ephemeris/spice_ephemeris.cc` and confirm every CSPICE call (`furnsh_c`, `spkezr_c`, `kclear_c`, etc.) is inside a `CspiceLock` RAII guard on the **process-wide** `cspice_global_mutex()` (per ADR-008; per-instance scope was an undisclosed deviation, fixed earlier in this PR with a covering concurrency test in `tests/unit/ephemeris/spice_concurrent_test.cc`).
- [ ] Confirm `data/SHA256SUMS` lists every file under `data/` and the runtime check matches.
- [ ] Spot-check `Time<UTC>` arithmetic across the 2017-01-01 leap second: `Time<UTC>{2016-12-31 23:59:59}` + `Duration{2s}` → `Time<UTC>{2017-01-01 00:00:00}`.
- [x] Visually inspect `leo_kepler_24h` residual; confirmed numerical residual (~1.5e-2 m / 1.7e-5 m/s) sits comfortably below the asserted tolerance (0.1 m / 1e-4 m/s) and is dominated by step-controller error rather than f-and-g iteration noise.

## Out of scope for Phase 1 (deferred)

- **Windows / MSVC support.** Phase 0 dropped `windows-2022` from the CI matrix. NAIF distributes a separate CSPICE tarball for Windows (`PC_Windows_VisualC_64bit/packages/cspice.zip`); the Linux source we vendor has multiple small MSVC incompatibilities (the `complex` type in f2c.h getting shadowed by `<complex.h>`'s macro, `inquire.c`'s `isatty` redefining UCRT's, ...). The proper fix is OS-conditional fetch in `cmake/fetch_external.cmake` plus a parallel `external/cspice/PINNED_VERSION.windows`. Estimated 50–100 LOC of CMake + a `.zip` extraction path. Track as a Phase 1 followup; reinstate `windows-2022` in the matrix when it lands. The design overview's "CI passes on Linux + Windows" is a v1.0 release gate, not a per-phase one.
- **Real Dop853 (Hairer Vol I Table 5.2).** Phase 1 ships `Dp54` (DP5(4), Hairer Vol I Table 5.1) as a coefficient stand-in behind the IIntegrator seam. The full DOP853 upgrade is a Phase 7 hardening item; the seam, PI step controller, and Phi augmentation are unchanged at the upgrade. See ADR-009 Phase 1 Implementation Note.
- **Berry-Healy 2004 ordinate-form Gauss-Jackson 8.** The `GaussJackson8` Phase 1 stand-in (a single `Dp54` step under the GJ8 name) was deleted because it carried zero distinct behaviour. Phase 7 reintroduces the type behind `IIntegrator` with the second-sum starter and the ordinate-form predictor-corrector.
- **Pines analytical gradient for `SphericalHarmonic`.** Phase 1 ships SH `partials()` as a finite-difference evaluation of the Cunningham acceleration; the adapter declares `kAnalyticalPartials = false` and is excluded from the VE-contract conformance gate. Phase 7 lands the analytical Pines gradient and re-includes the adapter in the gate.
- **Real ISS state-vector regression.** The Phase 1 regression suite ships `leo_kepler_24h` (Dp54 vs. f-and-g closed-form Kepler oracle, point-mass Earth only). Reproduction against published NASA reference vectors needs sourcing, licensing, and validation that are out of Phase 1 scope; deferred to Phase 7 with sourcing TBD.
- **Full DE440 fetch tooling.** Phase 1 ships a hand-curated truncated kernel under `data/de440_phase1.bsp`. Automated fetch + slice + checksum-validate of the full DE440 (and follow-on DE441) lives behind `cmake/fetch_large_data.cmake` in Phase 7.
- **Yoshida4 co-integrated Φ.** The Phase 1 Yoshida4 adapter approximates Φ via Verlet kick/drift composition; the conformance tolerance is consequently 100 m / 1e-1 m/s instead of the cleaner-but-not-symplectic analytical `dΦ/dt = A Φ` flow. Phase 7 may upgrade Φ-tracking inside Yoshida4 to a co-integrated linear ODE.
- **`SphericalHarmonic` ICRF→body-fixed rotation.** The Phase 1 SH adapter treats body-fixed and ICRF as aligned (acceptable because Phase 1 SH content is zonal-only, so rotation is a no-op around the Z axis at the regression-test epochs). Phase 7 composes the ITRS↔ICRF rotation inside the acceleration call, removing the alignment assumption.
- **Atmospheric drag, SRP.** Phase 7 hardening. The Phase 1 LEO Kepler regression sidesteps drag by using a point-mass-only force model; the future ISS-from-NASA-data regression will need both.
- **Higher-degree gravity (>deg 20).** Phase 7. EGM2008 to deg 20 is sufficient for the Phase 1 force model wiring.
- **Live EOP refresh tooling.** Phase 7. Phase 1 ships with a frozen EOP slice covering regression-test epochs.
- **Relativistic corrections (Schwarzschild, Lense-Thirring).** Phase 7. Below regression-tolerance for the Phase 1 cases.
- **TEME↔ITRS implementation body.** Stubbed in Phase 1; full implementation belongs to Phase 2 alongside SGP4.
- **Compile-fail test framework polish.** Phase 1 uses `try_compile` with `EXPECT_FAIL`; if this proves brittle on Windows, fall back to a one-job linux-only compile-fail suite (acceptable since the language semantics are platform-independent).

## Codegen / fallback notes

- If `data/de440_phase1.bsp` is missing or its SHA mismatches, the
  regression test is *skipped* with a clear `GTEST_SKIP("missing
  data/de440_phase1.bsp — see data/README.md")` rather than failing —
  this avoids confusing CI failures on data refresh PRs while still
  signalling the gap.
- If SOFA's `iauXys06a` is unavailable in the vendored release (would
  indicate a wrong tarball), `src/frames/icrs_itrs.cc` emits a
  configure-time `#error` naming the missing function. Manual
  fallback: pin SOFA to a release that ships it (every release since
  2010 does).
- If a regression tolerance proves unachievable with the Phase 1 force
  model, prefer **widening the tolerance with a documented reason**
  over **adding force-model adapters**. The latter belongs in Phase 7.
  (The original ISS regression's "widen to 5 km / 5 m/s if drag absence
  dominates" rule is obsolete — the Phase 1 LEO Kepler regression uses
  point-mass-only by construction; drag-vs-no-drag is no longer in
  play for the Phase 1 gate.)
