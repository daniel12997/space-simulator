# Apsis — Feature-Level Requirements

> **Status:** Draft v0.6 (v0.6: Conjunction Screening Pipeline deepening per [[wiki/concepts/conjunction-screening]] and [[wiki/decisions/005-broad-phase-strategy-pluggable]]; v0.5: Attitude Estimator Family deepening per [[wiki/concepts/attitude-estimation-policy]], [[wiki/concepts/usque]], and [[wiki/decisions/004-hybrid-attitude-estimation-mode-logic]]; v0.4: Long-arc state conditioning deepening per [[wiki/concepts/long-arc-state-conditioning]] and [[wiki/decisions/003-tagged-time-scale-types]]; v0.3: Variational Equations deepening per [[wiki/concepts/variational-equations]] and [[wiki/decisions/002-variational-equations-between-measurements]]; v0.2: revised against wiki audit 2026-05-05; see [[wiki/synthesis/audit-summary-2026-05-05]])
> **Purpose:** Feature-level requirements for the Apsis spaceflight simulator. These describe *what* the system shall do, not *how* it implements them. Lower-level technical requirements (algorithms, data structures, library choices) are captured in the architecture and subsystem documents.

## Conventions

- **SHALL** — mandatory requirement
- **SHOULD** — strong recommendation, may be deferred with justification
- **MAY** — optional capability

**Priority levels (MoSCoW):**
- **M** (Must) — required for v1.0 release
- **S** (Should) — required for v1.0 if effort permits; otherwise v1.1
- **C** (Could) — nice-to-have; v2+
- **W** (Won't) — explicitly out of scope for v1

**Requirement IDs** follow the pattern `REQ-<CATEGORY>-<NUMBER>`. Categories:

| Code | Category |
|---|---|
| TIME | Time and frame management |
| PHY | Physics and force models |
| INT | Integration and propagation |
| SC | Spacecraft modeling |
| MBD | Multi-body dynamics |
| EFF | Effectors |
| SEN | Sensors |
| GNC | Guidance, navigation, control |
| ENV | Environment models |
| CAT | Catalog and conjunction |
| MC | Monte Carlo |
| SCN | Scenario and orchestration |
| PERF | Performance and scaling |
| OBS | Observability and validation |
| ARCH | Architecture and non-functional |
| EXT | Extensibility |

---

## 1. Time and frame management

| ID | Requirement | Priority |
|---|---|---|
| REQ-TIME-001 | The system SHALL maintain time in TAI, TT, UTC, UT1, and TDB scales with documented conversions between all pairs. The system MAY additionally support TCG (Geocentric Coordinate Time) and TCB (Barycentric Coordinate Time) per IAU 2000 resolutions for relativistic-strict use cases. | M (TCG/TCB MAY) |
| REQ-TIME-002 | The system SHALL store time as a two-component representation `(epoch_jd, offset_seconds)` preserving nanosecond precision for `|offset| ≤ 10⁷ s` (~4 months). When `|offset|` exceeds the rectification threshold, the time SHALL auto-rectify (`epoch_jd ← epoch_jd + offset_seconds/86400`; `offset_seconds ← 0`) preserving the represented instant exactly. See [[wiki/concepts/long-arc-state-conditioning]] for the full rationale and how this composes with REQ-INT-006 (compensated summation) and REQ-INT-007 (Encke). | M |
| REQ-TIME-003 | The system SHALL load and apply leap-second tables, with support for table updates without recompilation. | M |
| REQ-TIME-004 | The system SHALL load and apply IERS Bulletin A polar motion and DUT1 data over the mission interval. | M |
| REQ-TIME-005 | The system SHALL provide right-handed inertial frames including ICRF (current realization ICRF3), the J2000 mean-equator-and-equinox frame (related to ICRF by the constant ~17 mas frame bias), GCRF, MCI, and per-body CCI for all major solar-system bodies. ICRF and J2000 SHALL be treated as distinct frames. | M |
| REQ-TIME-006 | The system SHALL provide right-handed body-fixed frames (ITRF for Earth, equivalents for other bodies) using IAU 2006/2000A precession-nutation via the CIO-based pipeline (see ADR-001) implemented through the IAU SOFA library. | M |
| REQ-TIME-007 | The system SHALL provide per-spacecraft frames including body, LVLH, RSW, and NTW. | M |
| REQ-TIME-008 | The system SHALL transform position, velocity, and attitude between any pair of supported frames at any supported epoch. | M |
| REQ-TIME-009 | The system SHALL maintain millimeter-level position precision in CCI frames out to 50 AU. This SHALL be achieved through the combined application of the three pillars of [[wiki/concepts/long-arc-state-conditioning|long-arc state conditioning]]: two-component time representation (REQ-TIME-002), compensated summation in integrator state accumulation (REQ-INT-006), and Encke-style perturbation propagation (REQ-INT-007). | M |
| REQ-TIME-010 | The system SHALL transition between CCI and CCF frames at user-configured altitude or SOI boundaries without observable state discontinuity. | M |
| REQ-TIME-011 | The system MAY provide local floating-origin frames for interstellar-scale precision. | C |
| REQ-TIME-013 | The `Time` type SHALL be tagged with its time scale (TAI / TT / UTC / UT1 / TDB; optionally TCG / TCB) at the type level. Conversions between scales SHALL be explicit and route through the IAU SOFA library. Mixing scales without explicit conversion SHALL be a compile-time error. See [[wiki/decisions/003-tagged-time-scale-types|ADR-003]] for the rationale (vs untagged) and serialization / Python-binding implications. | M |
| REQ-FRM-013 | The `State` type SHALL be tagged with its frame (ICRF / GCRS / ITRS / J2000 / TEME / BodyFixed) at the type level. Transforms between frames SHALL be explicit and route through SOFA / EOP-aware machinery. Mixing frames without explicit transform SHALL be a compile-time error. See [[wiki/decisions/010-phantom-typed-time-and-state|ADR-010]] for the implementation pattern. Parallels REQ-TIME-013 for the frame pillar. | M |

## 2. Physics and force models

| ID | Requirement | Priority |
|---|---|---|
| REQ-PHY-001 | The system SHALL provide a pluggable `ForceModel` interface allowing user-defined forces to be added without modifying the integrator. | M |
| REQ-PHY-002 | The system SHALL implement point-mass gravity for any configured central body. | M |
| REQ-PHY-003 | The system SHALL implement spherical-harmonic gravity to user-selectable degree and order, supporting EGM2008 (Earth) and equivalents for the Moon and Mars. | M |
| REQ-PHY-004 | The spherical-harmonic gravity implementation SHALL use a singularity-free formulation (Pines or equivalent) and pre-normalized coefficients. | M |
| REQ-PHY-005 | The system SHALL implement third-body gravity for Sun, Moon, and major planets using a numerically stable formulation that avoids cancellation at small spacecraft–central-body distances. | M |
| REQ-PHY-006 | The system SHALL implement atmospheric drag using NRLMSISE-00 as the default density model. | M |
| REQ-PHY-007 | The atmospheric drag implementation SHALL compute relative velocity in the rotating atmosphere frame. | M |
| REQ-PHY-008 | The system SHOULD support JB2008 as an alternative atmospheric density model. | S |
| REQ-PHY-009 | The system SHALL implement solar radiation pressure with cannonball geometry and conical shadow (umbra + penumbra). | M |
| REQ-PHY-010 | The system SHOULD support N-plate SRP modeling for attitude-dependent radiation pressure. | S |
| REQ-PHY-011 | The system SHOULD support Earth radiation pressure (albedo + IR) using Knocke's model. | S |
| REQ-PHY-012 | The system SHALL implement Schwarzschild relativistic correction for the central body per IERS Conventions 2010 Ch. 10. | M |
| REQ-PHY-013 | The system MAY implement Lense-Thirring and de Sitter relativistic corrections. | C |
| REQ-PHY-014 | The system SHALL implement solid Earth tides, ocean tides, and pole tides per IERS Conventions 2010 Ch. 7-8. Default-on for conjunction-assessment scenarios; default-off for low-precision use. | S |
| REQ-PHY-015 | The system SHALL implement empirical accelerations (constant or piecewise-constant) in user-selectable frames. | M |
| REQ-PHY-016 | Each force model SHALL implement the **Variational Equations contract** ([[wiki/concepts/variational-equations]]): `partial_dadr(state, t) → Matrix3d`, `partial_dadv(state, t) → Matrix3d`, and `partial_dadp(state, t) → vector<ParameterPartial>` returning partials with respect to its statically-declared estimable parameters. The contract is consumed by orbit estimation (REQ-GNC-004) and Pc covariance propagation (REQ-CAT-009). | M |
| REQ-PHY-017 | The system SHALL provide per-force runtime enable/disable flags. | M |
| REQ-PHY-018 | The system SHALL provide per-force timing instrumentation. | M |
| REQ-PHY-019 | The system SHALL use the **zero-tide** permanent-tide convention per IERS Conventions 2010 default. Mismatch with input gravity-coefficient files SHALL be detected and either converted automatically or flagged as an error. | M |
| REQ-PHY-020 | Each ForceModel implementation SHALL pass a finite-difference conformance test (analytical partials within `conformance_tolerance()` of finite-difference oracle over the model's declared `conformance_grid()`) per [[wiki/concepts/variational-equations]]. The conformance harness SHALL be a CI gate covering all registered force models, including user-defined ones. | M |

## 3. Integration and propagation

| ID | Requirement | Priority |
|---|---|---|
| REQ-INT-001 | The system SHALL provide adaptive Runge-Kutta (Dormand-Prince 8(7) or equivalent) as the default integrator. | M |
| REQ-INT-002 | The system SHALL provide Gauss-Jackson 8th-order fixed-step integration. | M |
| REQ-INT-003 | The system SHALL provide a symplectic integrator (Yoshida 8 or equivalent) for long-arc gravity-only propagation. | S |
| REQ-INT-004 | The system SHALL provide analytical Keplerian propagation. | M |
| REQ-INT-005 | The system SHALL provide SGP4 propagation for TLE-defined objects. The implementation SHALL use **WGS-72** fundamental constants per Spacetrack Report No. 3 (operational AFSPC convention) and SHALL convert SGP4's TEME output to the requested target frame per the Vallado et al. 2006 §VI recipe. | M |
| REQ-INT-006 | All integrators SHALL use compensated (Kahan-Neumaier) summation for state accumulation. The system SHALL provide both per-scalar (`CompensatedSum<T>`) and vectorised (`CompensatedAccumulator<V>`) primitives; the latter is used by the Variational Equations integrator (REQ-INT-014) for Φ matrix accumulation by default. See [[wiki/concepts/long-arc-state-conditioning]] §"Compensated summation". | M |
| REQ-INT-007 | The system SHALL provide Encke-style perturbation propagation as a **wrapper** over any base integrator (DOPRI 8(7), GJ8, Yoshida 8, etc.), not as a standalone integrator class. The wrapper SHALL maintain an analytical Keplerian reference orbit, integrate the deviation `δr` via Battin's form, and auto-rectify when `|δr| / |r_reference| > 0.01` (default 1% threshold; configurable). Engagement SHALL be per-spacecraft via scenario configuration (default off). The wrapper's dense output SHALL expose absolute coordinates so downstream consumers (e.g. the Variational Equations integrator) need not be Encke-aware. See [[wiki/concepts/long-arc-state-conditioning]] §"Encke perturbation propagation". | S |
| REQ-INT-008 | The system SHALL provide dense output (continuous interpolation between integration nodes) for all numerical integrators. | M |
| REQ-INT-009 | The system SHALL detect and localize events to a user-specified tolerance using root-finding on the dense output. | M |
| REQ-INT-010 | The system SHALL support the following event types as built-ins: periapsis, apoapsis, eclipse entry/exit, SOI crossing, altitude triggers, ground station AOS/LOS, and time triggers. | M |
| REQ-INT-011 | The system SHALL allow user-defined event functions and callbacks. | M |
| REQ-INT-012 | The system SHALL preserve total angular momentum to within integrator tolerance over a 1-year propagation when no external torques are applied. | M |
| REQ-INT-013 | The system SHALL preserve total energy to within integrator tolerance over a 100-year propagation under symplectic integration with conservative forces only. | S |
| REQ-INT-014 | The system SHALL provide a **Variational Equations integrator** ([[wiki/concepts/variational-equations]]) that propagates the state-transition matrix Φ between measurement / observation epochs, consuming per-force partials (REQ-PHY-016) and the natural-state integrator's dense output (REQ-INT-008). Φ propagation SHALL initialise `Φ(t₀, t₀) = I` per invocation, use a method appropriate for the linear ODE `dΦ/dt = A(t)·Φ` (default RK4), and return `Φ(t₁, t₀)` to the caller. Φ SHALL NOT be carried in the natural-state vector ([[wiki/decisions/002-variational-equations-between-measurements|ADR-002]]). | M |

## 4. Spacecraft modeling

| ID | Requirement | Priority |
|---|---|---|
| REQ-SC-001 | The system SHALL load spacecraft configurations from URDF files. | M |
| REQ-SC-002 | The system SHALL support a sidecar configuration file (YAML) attached to a URDF that defines effectors and sensors mounted on named links. | M |
| REQ-SC-003 | The system SHALL validate URDF files at load time and report inconsistencies (missing inertias, unreferenced links, etc.). | M |
| REQ-SC-004 | The system SHALL support time-varying mass properties for fueled craft, with automatic updates as propellant is consumed. | M |
| REQ-SC-005 | The system SHOULD support flexible-body modal coordinates for solar arrays, antennas, and similar appendages, using the hybrid-coordinate method per Likins (1970, JPL TR 32-1329). Required for any spacecraft whose first elastic modes overlap ACS bandwidth (typically 0.1-2 Hz). | S |
| REQ-SC-006 | The system SHOULD support fuel slosh modeling (pendulum or spring-mass surrogate models per Abramson 1966 NASA SP-106 / Dodge 2000 SwRI update). Required for any fueled craft whose slosh modes overlap ACS bandwidth. | S |
| REQ-SC-007 | The system SHALL allow multiple distinct spacecraft to be active simultaneously in a scenario. | M |

## 5. Multi-body dynamics

| ID | Requirement | Priority |
|---|---|---|
| REQ-MBD-001 | The system SHALL implement floating-base multi-body dynamics where the URDF root link's pose is the spacecraft's orbital state and attitude. | M |
| REQ-MBD-002 | The system SHALL solve forward dynamics (compute joint accelerations from applied forces and torques) at each integration step. | M |
| REQ-MBD-003 | The system SHALL solve inverse dynamics (compute joint torques required for prescribed motion) for use in controller design. | S |
| REQ-MBD-004 | The system SHALL correctly couple internal joint motion to spacecraft attitude via angular momentum conservation. | M |
| REQ-MBD-005 | The system SHALL support revolute, prismatic, fixed, and continuous (unlimited rotation) joint types. | M |
| REQ-MBD-006 | The system SHALL apply joint limits, friction, and damping as configured in URDF. | M |
| REQ-MBD-007 | The system SHALL provide Jacobian and mass matrix queries for use in controller design (LQR, MPC). | S |

## 6. Effectors

| ID | Requirement | Priority |
|---|---|---|
| REQ-EFF-001 | The system SHALL provide a thruster effector with configurable magnitude, direction, Isp, and throttle. | M |
| REQ-EFF-002 | The thruster effector SHALL update spacecraft mass via the rocket equation. | M |
| REQ-EFF-003 | The thruster effector SHALL support optional first-order valve dynamics (rise/fall time). | S |
| REQ-EFF-004 | The system SHALL provide a reaction wheel effector with wheel speed state, configurable max torque, max momentum, and friction model. | M |
| REQ-EFF-005 | The reaction wheel effector SHALL correctly couple wheel momentum to spacecraft attitude. | M |
| REQ-EFF-006 | The system SHALL provide a control moment gyro (CMG) effector with gimbal angle and rate states. | S |
| REQ-EFF-007 | The CMG effector SHALL exhibit singularity geometry (gimbal lock) without artificial smoothing. | S |
| REQ-EFF-008 | The system SHALL provide a magnetorquer effector that produces `m × B` torque using the configured geomagnetic field. | M |
| REQ-EFF-009 | The system SHALL provide a solar array drive effector (joint position controller with rate/accel limits). | M |
| REQ-EFF-010 | The system SHALL allow user-defined effectors via the `Effector` interface. | M |
| REQ-EFF-011 | The system MAY support Variable-Speed CMG (VSCMG) per Schaub, Vadali & Junkins (1998), which subsumes RW + SGCMG + VSCMG configurations under one steering law and provides singularity-robust torque tracking. If implemented, REQ-EFF-004 (RW) and REQ-EFF-006 (CMG) MAY be implemented as configurations of the VSCMG framework. | C |

## 7. Sensors

| ID | Requirement | Priority |
|---|---|---|
| REQ-SEN-001 | The system SHALL provide a star tracker sensor producing quaternion measurements with configurable noise and Sun/Earth/Moon keepout. | M |
| REQ-SEN-002 | The system SHALL provide sun sensor (coarse and fine) producing unit-vector measurements with eclipse-aware validity. | M |
| REQ-SEN-003 | The system SHALL provide a magnetometer producing body-frame field measurements with hard-iron and soft-iron biases. | M |
| REQ-SEN-004 | The system SHALL provide a GPS receiver sensor producing ITRF position and velocity, with sky-visibility based validity. | M |
| REQ-SEN-005 | The system SHALL provide gyro/IMU sensors with bias drift (Markov model), Angle Random Walk, and Rate Random Walk per axis. | M |
| REQ-SEN-006 | The system SHALL provide accelerometer sensors measuring non-gravitational acceleration in body frame. | M |
| REQ-SEN-007 | Each sensor SHALL produce timestamped `Measurement` messages on the GNC bus at its declared rate. | M |
| REQ-SEN-008 | Each sensor SHALL accept an explicit RNG argument; no global random state. | M |
| REQ-SEN-009 | The system SHALL allow user-defined sensors via the `Sensor` interface. | M |
| REQ-SEN-010 | The system SHALL support sensor failure injection (dropout, bias step, scale factor error, stuck-at value). | S |

## 8. Guidance, navigation, control

| ID | Requirement | Priority |
|---|---|---|
| REQ-GNC-001 | The system SHALL separate the continuous-time plant from the discrete-time GNC stack, communicating via a typed message bus. | M |
| REQ-GNC-002 | GNC components SHALL declare their sample rates; the system SHALL schedule them via multi-rate timing with explicit zero-order hold on outputs. | M |
| REQ-GNC-003 | The system SHALL provide a Multiplicative Extended Kalman Filter (MEKF) for attitude estimation, using MRP (Modified Rodrigues Parameters) as the 3-vector covariance state per Markley (2003) and the second-order MEKF extension as default. MEKF is the canonical nominal-mode algorithm wrapped in the `AttitudeEstimator` manager (REQ-GNC-016) under the hybrid policy of [[wiki/concepts/attitude-estimation-policy]]. | M |
| REQ-GNC-004 | The system SHALL provide an EKF for orbit estimation with at minimum position, velocity, and Cd as estimated parameters. State-transition matrix Φ for the predict step SHALL be obtained from the Variational Equations integrator (REQ-INT-014); parameter augmentation (e.g. Cd) consumes `partial_dadp` from REQ-PHY-016. | M |
| REQ-GNC-005 | The system SHOULD provide a UKF as an alternative orbit estimator (per Wan & van der Merwe 2000 scaled-UT formulation with augmented state). | S |
| REQ-GNC-006 | The system SHALL provide attitude controllers including: a PD-on-quaternion-error controller (small-angle / pointing-stability regime), a Lyapunov-stable nonlinear controller per Schaub, Vadali & Junkins (1998) (large-slew regime), and an LQR controller. | M |
| REQ-GNC-007 | The system SHALL provide a stationkeeping controller with deadband-on-elements logic. | M |
| REQ-GNC-008 | The system SHOULD provide an MPC attitude controller (slew planning under torque constraints). | S |
| REQ-GNC-009 | The system SHALL provide a finite-state-machine mode logic layer with user-configurable modes and transitions. | M |
| REQ-GNC-010 | The system SHALL allow user-defined controllers and estimators via interfaces. | M |
| REQ-GNC-011 | The system SHALL support modeling of computational delay (controller output applied N ticks after measurement). | S |
| REQ-GNC-012 | The system SHALL support effector failure injection at the message bus layer. | S |
| REQ-GNC-013 | The system SHALL support hardware-in-the-loop mode with the controller running on external flight hardware over a real-time interface. | C |
| REQ-GNC-014 | The system SHALL provide a USQUE (UKF-based attitude estimator per Crassidis & Markley 2003) as the acquisition-mode and inconsistency-recovery counterpart to MEKF. USQUE SHALL use the `(a=1, f=1)` MRP-flavoured GRP parameterisation (algebraically identical to MEKF's MRP error representation, enabling direct covariance hand-off). The two algorithms SHALL operate under the **hybrid mode logic** of [[wiki/decisions/004-hybrid-attitude-estimation-mode-logic|ADR-004]]: boot in USQUE; switch to MEKF when `||MRP|| < 10°` AND `trace(P_attitude) < threshold_acquired`; continuously monitor MEKF via NIS chi-squared test (default `p=0.99`, `N=5` consecutive samples); revert to USQUE on sustained inconsistency. See [[wiki/concepts/attitude-estimation-policy]] for the full policy. | M |
| REQ-GNC-015 | The system SHOULD provide a constrained-trajectory MPC for rendezvous and proximity operations per Di Cairano, Park & Kolmanovsky (2012), supporting line-of-sight cone constraints, terminal velocity matching, and obstacle/debris avoidance. | S |
| REQ-GNC-016 | The system SHALL provide an `AttitudeEstimator` operational interface that wraps MEKF (REQ-GNC-003) and USQUE (REQ-GNC-014), owns the `AttitudeEstimationPolicy` trigger logic from [[wiki/decisions/004-hybrid-attitude-estimation-mode-logic|ADR-004]], and exposes `pin_mode()` for override by the broader GNC mode FSM (REQ-GNC-009). The `AttitudeEstimationPolicy` SHALL be a pure-logic class testable in isolation against synthetic covariance and innovation histories. The MEKF and USQUE algorithm classes SHALL each independently satisfy the `Estimator` interface and SHALL remain usable standalone (for benchmarking, alternative wrappers, or custom policies). | M |

## 9. Environment models

| ID | Requirement | Priority |
|---|---|---|
| REQ-ENV-001 | The system SHALL provide ephemerides for Sun, Moon, and major planets via SPICE SPK kernels (DE440 or later). | M |
| REQ-ENV-002 | The system SHALL provide planetary orientation via SPICE PCK kernels. | M |
| REQ-ENV-003 | The system SHALL implement IGRF-14 (or current generation) geomagnetic field. | M |
| REQ-ENV-004 | The system SHOULD support World Magnetic Model (WMM) as an alternative. | C |
| REQ-ENV-005 | The system SHALL provide solar and geomagnetic indices for atmospheric and space-weather modeling. For NRLMSISE-00 (REQ-PHY-006): F10.7 (daily and 81-day average), Ap (daily and 3-hour history). For JB2008 (REQ-PHY-008): F10, S10, M10, Y10 solar UV indices plus Dst geomagnetic index. Kp also provided for general use. | M |
| REQ-ENV-006 | The system SHALL support time-varying space weather (historical from CelesTrak, predicted for future epochs). | M |
| REQ-ENV-007 | The system SHALL provide eclipse geometry (umbra, penumbra) for any spacecraft with respect to any occulting body. | M |
| REQ-ENV-008 | The system SHALL provide a GPS constellation visibility model (true GPS satellite ephemerides for sensor validity). | S |

## 10. Catalog and conjunction analysis

| ID | Requirement | Priority |
|---|---|---|
| REQ-CAT-001 | The system SHALL ingest two-line element sets (TLEs) from Space-Track and CelesTrak formats. | M |
| REQ-CAT-002 | The system SHALL maintain a catalog of at least 100,000 objects propagated by SGP4. | M |
| REQ-CAT-003 | The system SHALL refresh catalog TLEs from external sources on demand. | S |
| REQ-CAT-004 | The system SHALL ingest CARA covariance data (CDMs) when available and associate covariances with catalog objects. | S |
| REQ-CAT-005 | The system SHALL perform conjunction screening between active spacecraft and the full catalog over a configurable forward interval (default 7 days). | M |
| REQ-CAT-006 | The system SHALL apply two pre-filter layers before geometric screening: (a) orbit-element pre-filters (apogee/perigee, inclination), and (b) analytical orbit-distance bounds (Hoots-Crawford / Healy formulation). Together these layers SHALL eliminate the bulk of the N×M pair count via cheap arithmetic on orbital elements before any per-epoch geometric evaluation. See [[wiki/concepts/conjunction-screening]] §3.1-3.2. | M |
| REQ-CAT-007 | The system SHALL provide a geometric broad-phase pair filter as a swappable strategy interface (`BroadPhase`), with default implementation **spatial hashing** and required alternative **sort-and-sweep**. Strategy selection SHALL be per-scenario configurable. See [[wiki/decisions/005-broad-phase-strategy-pluggable|ADR-005]] for rationale and the future trajectory-tube extension point. | M |
| REQ-CAT-008 | The system SHALL find **all local minima** of `\|r_A - r_B\|` within the screening window for each candidate pair (a pair with similar-period orbits can have multiple close approaches in a 7-day window — each is a separate conjunction event). TCA precision SHALL be better than 10 m miss-distance (which implies sub-millisecond TCA timing precision at typical relative velocities). Localisation via Brent's method on `d/dt \|r_A - r_B\|² = 0`. | M |
| REQ-CAT-009 | The system SHALL compute probability of collision (Pc) via a method registry. Each registered method declares its own validity predicate; the pipeline picks the **first valid method in registry order** for the current pair. Default registry: `[foster_2d, monte_carlo]` — Foster's 2D analytic method (Foster & Estes 1992; Chan series evaluator per Bombardelli 2015) as primary while `ε = t_c/T_orbit < 1e-3`; deterministic seeded Monte Carlo (REQ-MC-003) as fallback when Foster's short-term-encounter assumption fails. The chosen method SHALL be recorded in the conjunction event's `pc_method` field. Joint covariance roll-forward from epoch to TCA SHALL use the Variational Equations integrator (REQ-INT-014). See [[wiki/concepts/conjunction-screening]] §4.4. | M |
| REQ-CAT-010 | The system SHALL emit `ConjunctionEvent` records with the following payload when Pc exceeds the configured threshold (default 1e-5): TCA in TT, miss distance, relative velocity at TCA, Pc, `pc_method` diagnostic tag, combined HBR, per-object state at TCA (position + velocity), per-object covariance at TCA (6×6 RTN), per-object identifiers, and screening diagnostics (broad-phase strategy used, sample-epoch count, MC seed if applicable, originator-Pc from CDM if any). See [[wiki/concepts/conjunction-screening]] §4.6. | M |
| REQ-CAT-011 | The system SHOULD provide an automated avoidance maneuver planner that produces optimal impulsive Δv per Bombardelli & Hernando-Ayuso (2015) — eigenvector of `MᵀQM` for either max-miss-distance or min-Pc objective. Along-track burns are a constrained-mode option, not the default. | S |
| REQ-CAT-012 | The system MAY perform full N×N conjunction screening across the catalog (not just against active spacecraft). | C |
| REQ-CAT-013 | The system SHALL maintain a per-spacecraft Hard-Body Radius (HBR) configuration. CCSDS 508 CDM ingestion (REQ-CAT-004) does not carry HBR; HBR SHALL be supplied separately and combined as `HBR = HBR_primary + HBR_secondary` for Pc computation. | M |
| REQ-CAT-014 | The system SHALL ingest CCSDS 508.0-B-1 Conjunction Data Messages (CDMs) in both KVN and XML serializations per the Blue Book specification. Originator-provided Pc fields SHALL be retained for cross-reference but not used as authoritative; Apsis-computed Pc is authoritative. | M |
| REQ-CAT-015 | The system SHOULD support Patera's 2D analytic Pc method as a registered alternative covering the moderate-ε regime where Foster's short-term assumption fails but Monte Carlo cost is undesirable. When registered, Patera takes precedence over Monte Carlo for `ε < 1e-1`; Monte Carlo remains the catch-all for `ε > 1e-1`. | S |
| REQ-CAT-016 | The system SHALL provide a `ConjunctionScreeningPipeline` operational interface covering catalog-driven conjunction assessment per [[wiki/concepts/conjunction-screening]], composing inner-stage modules (catalog propagator, orbit-element pre-filter, analytical-distance pre-filter, broad-phase strategy, TCA refiner, covariance propagator, Pc calculator, HBR resolver, event emitter). Each inner stage SHALL be independently testable against synthetic inputs. | M |
| REQ-CAT-017 | Broad-phase pair filtering SHALL be implemented as a strategy interface (`BroadPhase`) per [[wiki/decisions/005-broad-phase-strategy-pluggable|ADR-005]]. v1 SHALL ship two implementations: spatial-hash (default) and sort-and-sweep. Future implementations (e.g. trajectory-tube intersection) SHALL be addable without modifying the pipeline interface or any consumer. Strategy selection is per-scenario configurable. | M |

## 11. Monte Carlo

| ID | Requirement | Priority |
|---|---|---|
| REQ-MC-001 | The system SHALL support running N independent trials of a scenario with seeded perturbations. | M |
| REQ-MC-002 | Each trial SHALL be deterministic given a seed: same seed and same machine produce bit-identical results. | M |
| REQ-MC-003 | The system SHALL spawn per-component RNGs from a per-trial master RNG; no global random state. | M |
| REQ-MC-004 | The system SHALL support distributional perturbation specifications (Gaussian, uniform, truncated Gaussian, user-defined) on initial conditions, force model parameters, and sensor parameters. | M |
| REQ-MC-005 | The system SHALL support snapshot/restore of the world state for fast trial reset. | M |
| REQ-MC-006 | The system SHALL execute trials in parallel across worker processes (local) and across nodes (cluster, optional). | M |
| REQ-MC-007 | The system SHALL aggregate per-trial results into a campaign result store (Parquet or HDF5). | M |
| REQ-MC-008 | The system SHALL support user-defined per-trial metrics computed from trial state. | M |
| REQ-MC-009 | The system SHALL provide standard analysis utilities (dispersion ellipses, percentile plots, failure-rate computation) over campaign results. | S |
| REQ-MC-010 | The system SHALL be capable of completing 10,000 trials of a 24-hour LEO closed-loop GNC scenario in under 4 hours on a 32-core workstation. | S |

## 12. Scenario and orchestration

| ID | Requirement | Priority |
|---|---|---|
| REQ-SCN-001 | The system SHALL provide a Python API (via pybind11) as the primary scenario authoring interface. | M |
| REQ-SCN-002 | A scenario SHALL specify epoch, initial state, spacecraft configuration, force model, GNC stack, mission timeline, and termination conditions. | M |
| REQ-SCN-003 | The system SHALL support scheduled events at absolute or relative times (e.g., "at T+3600s, fire main engine for 120s"). | M |
| REQ-SCN-004 | The system SHALL support conditional events triggered by state functions (e.g., "when altitude < 100 km, trigger entry interface"). | M |
| REQ-SCN-005 | The system SHALL support mission phases (cruise, pre-burn, burn, post-burn, science) with per-phase configuration. | M |
| REQ-SCN-006 | The system SHALL run scenarios in three modes: as-fast-as-possible (batch), real-time-paced (visualization), and hardware-locked (HIL). | M |
| REQ-SCN-007 | The system SHALL serialize scenarios to disk and reload them with bit-identical behavior. | M |
| REQ-SCN-008 | The system SHALL provide example scenarios for: LEO orbit propagation, GEO stationkeeping, lunar transfer, Mars transfer, attitude slew, conjunction avoidance. | M |

## 13. Performance and scaling

| ID | Requirement | Priority |
|---|---|---|
| REQ-PERF-001 | The system SHALL propagate a single spacecraft with a high-fidelity force model (geopotential 70×70, third-body, drag, SRP) at 1000× real time during cruise on a single core. | M |
| REQ-PERF-002 | The system SHALL propagate a closed-loop GNC scenario at 50 Hz controller rate at 50× real time during cruise on a single core. | S |
| REQ-PERF-003 | The system SHALL propagate a 50,000-object SGP4 catalog one epoch in under 1 second on a single core. | M |
| REQ-PERF-004 | The system SHALL execute conjunction screening (broad-phase pre-filter + spatial hash + TCA refinement) over a 7-day forward window for a 50,000-object catalog and one active spacecraft in under 60 seconds on a single core. | S |
| REQ-PERF-005 | The system SHALL scale Monte Carlo throughput linearly with core count up to 32 cores on a single machine. | M |
| REQ-PERF-006 | Memory usage SHALL not exceed 4 GB for a 100,000-object catalog with full SGP4 state. | M |

## 14. Observability and validation

| ID | Requirement | Priority |
|---|---|---|
| REQ-OBS-001 | The system SHALL provide structured logging at configurable verbosity levels. | M |
| REQ-OBS-002 | The system SHALL provide a subscriber-based telemetry recorder that writes only requested variables. | M |
| REQ-OBS-003 | Recorded telemetry SHALL be written in a columnar format (Parquet) with schema metadata. | S |
| REQ-OBS-004 | The system SHALL provide built-in conservation checks (total energy, total angular momentum, total mass) as test invariants. | M |
| REQ-OBS-005 | The system SHALL include a regression test suite covering: two-body orbits, J2-perturbed orbits, Hohmann transfers, lunar trajectories, attitude maneuvers, conjunction screening. | M |
| REQ-OBS-006 | The system SHALL include validation cases reproducing published reference trajectories (ISS state vectors from public sources, MRO cruise trajectory, etc.) within published tolerances. | M |
| REQ-OBS-007 | The system SHALL provide profiling instrumentation per force model, per integrator step, and per GNC component. | S |

## 15. Architecture and non-functional

| ID | Requirement | Priority |
|---|---|---|
| REQ-ARCH-001 | The simulator core SHALL be implemented in C++17 or later. | M |
| REQ-ARCH-002 | The simulator SHALL provide a Python binding (pybind11) for scenario authoring and analysis. | M |
| REQ-ARCH-003 | The simulator SHALL build and run on Linux (primary) and Windows (secondary). macOS support is desired but not required. | M |
| REQ-ARCH-004 | The simulator SHALL use CMake as its build system. | M |
| REQ-ARCH-005 | The simulator SHALL produce no compiler warnings under `-Wall -Wextra -Wpedantic` (clang/gcc) at the highest practical setting. | S |
| REQ-ARCH-006 | The simulator SHALL include unit tests with code coverage targeting ≥80% on the dynamics core. | M |
| REQ-ARCH-007 | All numerical state SHALL be `f64` (double-precision). Use of `f32` is permitted only for rendering output. | M |
| REQ-ARCH-008 | The simulator SHALL not use global mutable state. All RNGs, time, and force-model state SHALL be passed explicitly. | M |
| REQ-ARCH-009 | The simulator SHALL not introduce nondeterminism within a single trial (no thread-pool internal parallelism, no hash-map iteration order dependencies in dynamics). | M |
| REQ-ARCH-010 | Documentation SHALL include: API reference (Doxygen), architecture overview, subsystem design, user guide, validation report. | M |

## 16. Extensibility

| ID | Requirement | Priority |
|---|---|---|
| REQ-EXT-001 | New force models SHALL be addable without modifying core simulator code (only new files implementing the `ForceModel` interface). | M |
| REQ-EXT-002 | New integrators SHALL be addable without modifying core simulator code. | M |
| REQ-EXT-003 | New effectors and sensors SHALL be addable without modifying core simulator code. | M |
| REQ-EXT-004 | New estimators and controllers SHALL be addable without modifying core simulator code. | M |
| REQ-EXT-005 | The Python binding SHALL allow user-defined force models, effectors, sensors, estimators, and controllers to be implemented in Python with acceptable performance for prototyping. | S |
| REQ-EXT-006 | The system SHALL provide a plugin discovery mechanism (registry of available components readable by the scenario layer). | S |

## 17. Out of scope (v1)

| ID | Item |
|---|---|
| OOS-001 | Real-time 3D visualization (deferred to a separate companion tool; recorded telemetry is sufficient for v1). |
| OOS-002 | Mission design optimization (Lambert solving, pork-chop generation, low-thrust optimal control). Use GMAT or PyKEP as offline companion tools. |
| OOS-003 | Spacecraft thermal modeling. |
| OOS-004 | Spacecraft power systems modeling beyond simple solar array geometry. |
| OOS-005 | Spacecraft communication link budgets (data rate, BER) beyond geometric visibility. |
| OOS-006 | Plasma and charging environment modeling. |
| OOS-007 | Re-entry aerothermodynamics (full hypersonic CFD-based modeling). Drag and simple heating only. |
| OOS-008 | Multiplayer / networked simulation. |
| OOS-009 | Game-style user interface; the v1 product is library + Python API only. |
| OOS-010 | Contact dynamics for capture, berthing, docking, or surface landing. The Apsis MBD module (REQ-MBD-*) covers free-flying floating-base dynamics; contact-constrained inverse-dynamics control (per Mistry et al. 2010 orthogonal-decomposition method) is deferred. |
| OOS-011 | FDIR (Fault Detection, Isolation, Recovery) response logic. Failure *injection* is in scope (REQ-SEN-010, REQ-GNC-012); the FDIR controller implementing automated response is deferred. |
| OOS-012 | Sub-tracking-threshold orbital debris flux modeling (NASA ORDEM 3.1). Apsis covers SSN-tracked deterministic debris (REQ-CAT-*); statistical micrometeoroid+orbital-debris (MMOD) flux for shielding sizing is deferred to a separate companion analysis. |
| OOS-013 | Multi-panel high-fidelity drag and SRP modeling (Sentman / Doornbos accommodation coefficients, panel-by-panel summation with attitude). v1 ships cannonball drag and N-plate SRP only. |

## 18. Acceptance criteria summary

The v1.0 release is gated on:

1. All **M (Must)** requirements implemented and tested.
2. Validation report demonstrates reproduction of at least three published reference cases within tolerance.
3. Performance requirements REQ-PERF-001, REQ-PERF-003, REQ-PERF-005, REQ-PERF-006 met on reference hardware.
4. Example scenarios (REQ-SCN-008) all run end-to-end and produce sensible results.
5. Monte Carlo capability runs a 1,000-trial reference campaign and produces correctly-shaped dispersion statistics.
6. Conjunction screening reproduces a known historical close-approach event from public TLE data within published miss-distance tolerance.
7. Documentation suite (REQ-ARCH-010) is complete.
8. CI passes on Linux and Windows.

---

## Appendix A — Traceability

Requirements trace to architecture sections as follows:

| Category | Architecture coverage |
|---|---|
| TIME | `01-architecture.md` §3 Foundation; `02-subsystems.md` §1 |
| PHY | `02-subsystems.md` §2 |
| INT | `02-subsystems.md` §3 |
| SC, MBD | `02-subsystems.md` §4 |
| EFF, SEN | `02-subsystems.md` §4.5, §4.6 |
| GNC | `02-subsystems.md` §5 |
| ENV | `02-subsystems.md` §1, §2 |
| CAT | `02-subsystems.md` §6 |
| MC | `02-subsystems.md` §7 |
| SCN | `01-architecture.md` §3 Scenario layer |
| PERF | `02-subsystems.md` §8 |

## Appendix B — Open questions

- **License of dependencies.** SPICE is public domain; SOFA has an attribution license; Pinocchio is BSD-2; EnTT/flecs are MIT. Most permissive. EGM2008 coefficients are unrestricted. Confirm full dependency license matrix before release.
- **TLE redistribution.** Space-Track has terms of service that limit redistribution. CelesTrak is more permissive. Establish which path the simulator's example data takes.
- **Cluster MC.** Whether cluster-scale Monte Carlo is in v1 or deferred. Local-machine parallelism is sufficient for most use cases; cluster is a v1.x extension.
- **HIL real-time interface.** Spec needed for the HIL transport (UDP? Shared memory? RTI DDS?). Likely v2 territory.
- **Scenario DSL maturity.** Whether the scenario layer is "Python API" (pure pybind11 with helper classes) or a higher-level YAML/JSON DSL with Python escape hatches. Recommend starting with Python API and growing a YAML layer on top.
