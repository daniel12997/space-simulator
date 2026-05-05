# Structure Outline

Vertical-slice phasing of the Apsis build, derived from `00-design-overview.md`.
Each phase delivers an end-to-end capability with a verification hook. Earlier
phases stand on their own if later phases are deferred.

## Approach

Build outward from a single-spacecraft propagator, adding tiers of fidelity and
breadth one slice at a time. Each slice crosses every layer it needs (math →
adapter → seam → orchestration → test) and lands a regression case before the
next slice opens. C++17 core; Python (pybind11) only enters at Phase 6.

---

## Phase 1: Propagator core (single spacecraft, force-model + VE)

End-to-end propagation of one spacecraft state forward in time, with a
[[wiki/concepts/variational-equations|variational-equations]] roll-forward
contract on every force model. Closes
[[wiki/decisions/001-use-ceo-based-icrs-to-itrs|ADR-001]] /
[[wiki/decisions/002-variational-equations-between-measurements|ADR-002]] /
[[wiki/decisions/003-tagged-time-scale-types|ADR-003]]. Lands the JPL DE
round-trip and ISS-vector regression cases.

**Files**: `include/apsis/time/`, `include/apsis/frames/`, `include/apsis/force/`,
`include/apsis/integrate/`, `src/.../sofa_adapter.cc`, `src/.../spice_adapter.cc`,
`tests/regression/jpl_de_roundtrip.cc`, `tests/regression/iss_vector.cc`

**Key seams / types**:
- `Time<Scale>` — `Scale ∈ {TAI,TT,UTC,UT1,TDB}` ([[wiki/concepts/time-scales]]); two-component `(epoch_jd, offset_s)` for [[wiki/concepts/long-arc-state-conditioning|long-arc conditioning]]; cross-scale arithmetic is a compile error.
- `convert<To,From>(Time<From>) -> Time<To>` — single SOFA-mediated path.
- `State<Frame>` — position, velocity, frame tag.
- `transform<To,From>(State<From>, Time<TT>) -> State<To>` — ICRS↔ITRS via the CIO pipeline ([[wiki/concepts/celestial-ephemeris-origin|CIO]], [[wiki/concepts/earth-rotation-angle|ERA]], [[wiki/concepts/precession-nutation|precession-nutation]]); [[wiki/concepts/frame-bias|frame bias]] handled at the ICRF/J2000 seam.
- `IForceModel`: `acc(t, x) -> Vec3` and `partials(t, x) -> Mat<3,6>` (the VE contract).
- `IIntegrator::step(state, Φ, dt) -> (state', Φ')` — adaptive RK, symplectic, and [[wiki/concepts/gauss-jackson-integration|Gauss-Jackson 8]] adapters; one parameterised conformance test on the seam.
- `EnckeWrapper<Integrator>` — propagates deviation from a Keplerian reference (uses [[wiki/concepts/f-and-g-series|f and g series]] for the reference orbit).
- `SpiceEphemeris` adapter behind an `IEphemeris` seam (CSPICE access serialised at the seam).

**Verify**: unit tests for SOFA round-trip, frame round-trip, and conservation
on Kepler problem; regression `jpl_de_roundtrip` passes within mm over 10 yr;
`iss_vector` reproduces published state to spec tolerance; CTest green.

---

## Phase 2: ECS world + catalog + conjunction screening

Many entities, [[wiki/concepts/sgp4|SGP4]] catalog, and a
[[wiki/concepts/conjunction-screening|conjunction-screening]] pipeline with a
strategy-pluggable broad-phase. Closes
[[wiki/decisions/005-broad-phase-strategy-pluggable|ADR-005]]. End-to-end
deliverable: reproduce a known historical close approach from public TLE data.

**Files**: `include/apsis/ecs/`, `include/apsis/catalog/`, `include/apsis/sgp4/`,
`include/apsis/conjunction/`, `tests/regression/historical_conjunction.cc`

**Key seams / types**:
- `World` — ECS façade (entt or flecs adapter behind a thin seam).
- `Sgp4Propagator::propagate(tle, Time<UTC>) -> State<TEME>`; WGS-72 constants per Spacetrack Report #3.
- `CatalogService` — per-process lifetime, owns ~50k TLE set; queried by transient pipelines (per-process service vs per-screen module).
- `IBroadPhase::pairs(epoch, window) -> PairList` — adapters: `SpatialHash` (default), `SortAndSweep` (required), `TrajectoryTube` (later).
- `ConjunctionScreen::run(primaries, catalog, window) -> ConjunctionList` — narrow-phase using Φ-based covariance roll-forward from Phase 1; Foster Pc on the encounter geometry.
- One parameterised `IBroadPhase` conformance test covers all adapters.

**Verify**: 50k synthetic catalog screens in CI under a generous budget (real
1 s target deferred to Phase 7); historical-event regression reproduces miss
distance within published tolerance; both broad-phase adapters pass conformance.

---

## Phase 3: Multi-body / URDF (active-spacecraft tier)

Articulated spacecraft with floating base, effectors, flexible-body and slosh
DoFs — the "active" tier of tiered fidelity. Catalog stays SGP4.

**Files**: `include/apsis/mbd/`, `src/.../pinocchio_adapter.cc`, `urdf/` examples,
`tests/regression/angular_momentum.cc`

**Key seams / types**:
- `MultiBodyModel` — [[wiki/concepts/pinocchio-library|Pinocchio]] behind an `IMultiBody` seam (URDF in, model out); [[wiki/concepts/articulated-body-algorithm|ABA]] / [[wiki/concepts/composite-rigid-body-algorithm|CRBA]] under the seam.
- `MultiBodyState` — generalised q, v plus [[wiki/concepts/floating-base-dynamics|floating-base]] `State<Frame>` coupling.
- `IEffector::wrench(state, t) -> Wrench` — thrusters, RWs, control moments.
- `FlexibleMode`, `SloshMass` — added DoFs with their own integrator hooks.
- Coupling rule: floating base translates orbital `State` ↔ Pinocchio root frame at one named seam.
- VE contract from Phase 1 extends through MBD via [[wiki/concepts/analytical-rbd-derivatives|analytical RBD derivatives]] (Pinocchio's analytical gradients).

**Verify**: angular-momentum conservation invariant on a torque-free articulated
body to integrator tolerance over a long arc; URDF round-trip example loads
and propagates; energy invariant on a passive flexible mode.

---

## Phase 4: GNC layer (sensors → estimators → controllers)

Closed-loop attitude on the active-tier spacecraft. Closes
[[wiki/decisions/004-hybrid-attitude-estimation-mode-logic|ADR-004]] (hybrid
[[wiki/concepts/mekf|MEKF]] / [[wiki/concepts/usque|USQUE]] with mode policy).

**Files**: `include/apsis/gnc/sensors/`, `include/apsis/gnc/estimate/`,
`include/apsis/gnc/control/`, `include/apsis/gnc/bus/`,
`tests/regression/attitude_slew.cc`

**Key seams / types**:
- `ISensor::measure(truth, Time<TAI>) -> Measurement` — IMU ([[wiki/concepts/farrenkopf-gyro-model|Farrenkopf]] noise), star tracker, sun sensor adapters.
- `IAttitudeEstimator` adapters: `Mekf`, `Usque`. Shared MRP-flavoured [[wiki/concepts/generalized-rodrigues-parameters|GRP]] parameterisation for direct covariance hand-off; common [[wiki/concepts/quaternion-attitude-representation|quaternion]] reference.
- `AttitudeEstimatorManager` — pure-logic [[wiki/concepts/attitude-estimation-policy|estimator policy]] (boot-USQUE → NIS-monitored MEKF → fallback). Mission FSM may pin a mode.
- `IController::command(estimate, reference, t) -> ControlInput`.
- `MessageBus` — typed pub/sub, per-trial deterministic ordering.

**Verify**: closed-loop attitude slew converges within spec; NIS monitor flips
mode under injected sensor degradation; manager-policy unit tests cover the
mode table.

---

## Phase 5: Monte Carlo (snapshot, RNG, parallel trials, columnar results)

Thousands of deterministic trials on the same scenario, aggregated columnar.
Closes the determinism non-negotiables.

**Files**: `include/apsis/mc/`, `src/.../snapshot.cc`, `src/.../rng.cc`,
`src/.../trial_driver.cc`, `tests/regression/mc_1k_dispersion.cc`

**Key seams / types**:
- `Snapshot::capture(World) -> Bytes` / `Snapshot::restore(Bytes) -> World` (entt/flecs snapshot under the seam).
- `RngStream` — split per trial and per consumer; no global state; reproducible from `(campaign_seed, trial_id, consumer_id)`.
- `TrialDriver::run(scenario, n_trials) -> ResultStore` — across-trial parallelism only; trial body is single-threaded.
- `ResultStore` — Arrow-backed columnar store; one row per trial × sample.

**Verify**: same `(campaign_seed, trial_id)` produces bit-identical trajectories
across runs and across thread counts; 1 k-trial reference campaign produces
correctly-shaped dispersion statistics on a known case.

---

## Phase 6: Scenario layer (Python DSL + bindings + examples)

User-facing surface. pybind11 bindings, scenario DSL, the six example scenarios
from the v1.0 acceptance list.

**Files**: `python/apsis/`, `bindings/`, `examples/leo_propagation.py`,
`examples/geo_stationkeeping.py`, `examples/lunar_transfer.py`,
`examples/mars_transfer.py`, `examples/attitude_slew.py`,
`examples/conjunction_avoidance.py`

**Key seams / types**:
- `apsis.Scenario` — declarative scenario builder; binds to `World` + drivers.
- pybind11 bindings: `Time`, `State`, `World`, `TrialDriver`, force-model and integrator factories.
- DSL keeps frame and time-scale tags visible at the Python boundary (no silent flattening to floats).

**Verify**: all six example scenarios run end-to-end in CI; pybind layer
round-trips type-tagged Time/State without losing tags; one MC example calls
`TrialDriver` from Python and produces the same results as the C++ harness.

---

## Phase 7: Hardening (validation, performance, fidelity tail) — cross-cutting

Not a vertical slice — this is the cross-cutting completion pass that takes the
build from "runs" to "v1.0". Includes the validation report (3+ reference
cases), performance work to hit the 1000× real-time / 50k catalog / 60 s
screening targets, higher-fidelity force-model adapters, and Linux + Windows
CI matrix.

**Verify**: validation report committed; perf benchmarks meet targets on
reference hardware; CI matrix green; all `M` requirements in
`REQUIREMENTS.md` mapped to a passing test.

---

## Testing Checkpoints

After each phase, the following should be true and re-runnable:

- **P1**: single-spacecraft propagation regressions (JPL DE, ISS) green; SOFA / frame round-trip unit tests green; `IForceModel` VE contract test green for every force-model adapter.
- **P2**: 50k-object screening completes in CI (loose budget); historical-conjunction regression green; both broad-phase adapters pass conformance.
- **P3**: angular-momentum and energy invariants green on articulated bodies; URDF example loads + propagates.
- **P4**: closed-loop attitude slew regression green; estimator-manager policy unit tests green.
- **P5**: bit-identical trial reproducibility test green across thread counts; 1 k-trial dispersion regression green.
- **P6**: all six example scenarios run end-to-end in CI; Python ↔ C++ round-trip tests preserve type tags.
- **P7**: validation report regenerated; perf benchmarks meet targets; CI green on Linux + Windows.

If a phase fails, prior phases remain independently usable: P1 alone is a
trajectory-study tool; P1+P2 is a conjunction-screening tool without GNC; P1–P4
is a deterministic single-trial GNC sim; P1–P5 adds MC verification.
