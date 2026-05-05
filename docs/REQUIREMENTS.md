# Apsis — Feature-Level Requirements

> **Status:** Draft v0.1
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
| REQ-TIME-001 | The system SHALL maintain time in TAI, TT, UTC, UT1, and TDB scales with documented conversions between all pairs. | M |
| REQ-TIME-002 | The system SHALL store time as a two-component representation (epoch + offset) preserving nanosecond precision over a 100-year span. | M |
| REQ-TIME-003 | The system SHALL load and apply leap-second tables, with support for table updates without recompilation. | M |
| REQ-TIME-004 | The system SHALL load and apply IERS Bulletin A polar motion and DUT1 data over the mission interval. | M |
| REQ-TIME-005 | The system SHALL provide right-handed inertial frames for ICRF/J2000, GCRF, MCI, and per-body CCI for all major solar-system bodies. | M |
| REQ-TIME-006 | The system SHALL provide right-handed body-fixed frames (ITRF for Earth, equivalents for other bodies) using IAU 2006/2000A precession-nutation. | M |
| REQ-TIME-007 | The system SHALL provide per-spacecraft frames including body, LVLH, RSW, and NTW. | M |
| REQ-TIME-008 | The system SHALL transform position, velocity, and attitude between any pair of supported frames at any supported epoch. | M |
| REQ-TIME-009 | The system SHALL maintain millimeter-level position precision in CCI frames out to 50 AU. | M |
| REQ-TIME-010 | The system SHALL transition between CCI and CCF frames at user-configured altitude or SOI boundaries without observable state discontinuity. | M |
| REQ-TIME-011 | The system MAY provide local floating-origin frames for interstellar-scale precision. | C |

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
| REQ-PHY-012 | The system SHOULD implement Schwarzschild relativistic correction for the central body. | S |
| REQ-PHY-013 | The system MAY implement Lense-Thirring and de Sitter relativistic corrections. | C |
| REQ-PHY-014 | The system MAY implement solid Earth tides, ocean tides, and pole tides per IERS Conventions 2010. | C |
| REQ-PHY-015 | The system SHALL implement empirical accelerations (constant or piecewise-constant) in user-selectable frames. | M |
| REQ-PHY-016 | Each force model SHALL provide partial derivatives `∂a/∂r` and `∂a/∂v` for use in variational equations. | S |
| REQ-PHY-017 | The system SHALL provide per-force runtime enable/disable flags. | M |
| REQ-PHY-018 | The system SHALL provide per-force timing instrumentation. | M |

## 3. Integration and propagation

| ID | Requirement | Priority |
|---|---|---|
| REQ-INT-001 | The system SHALL provide adaptive Runge-Kutta (Dormand-Prince 8(7) or equivalent) as the default integrator. | M |
| REQ-INT-002 | The system SHALL provide Gauss-Jackson 8th-order fixed-step integration. | M |
| REQ-INT-003 | The system SHALL provide a symplectic integrator (Yoshida 8 or equivalent) for long-arc gravity-only propagation. | S |
| REQ-INT-004 | The system SHALL provide analytical Keplerian propagation. | M |
| REQ-INT-005 | The system SHALL provide SGP4 propagation for TLE-defined objects. | M |
| REQ-INT-006 | All integrators SHALL use compensated (Kahan-Neumaier) summation for state accumulation. | M |
| REQ-INT-007 | The system SHALL provide Encke-style perturbation propagation as an alternative formulation, with automatic reference rectification. | S |
| REQ-INT-008 | The system SHALL provide dense output (continuous interpolation between integration nodes) for all numerical integrators. | M |
| REQ-INT-009 | The system SHALL detect and localize events to a user-specified tolerance using root-finding on the dense output. | M |
| REQ-INT-010 | The system SHALL support the following event types as built-ins: periapsis, apoapsis, eclipse entry/exit, SOI crossing, altitude triggers, ground station AOS/LOS, and time triggers. | M |
| REQ-INT-011 | The system SHALL allow user-defined event functions and callbacks. | M |
| REQ-INT-012 | The system SHALL preserve total angular momentum to within integrator tolerance over a 1-year propagation when no external torques are applied. | M |
| REQ-INT-013 | The system SHALL preserve total energy to within integrator tolerance over a 100-year propagation under symplectic integration with conservative forces only. | S |

## 4. Spacecraft modeling

| ID | Requirement | Priority |
|---|---|---|
| REQ-SC-001 | The system SHALL load spacecraft configurations from URDF files. | M |
| REQ-SC-002 | The system SHALL support a sidecar configuration file (YAML) attached to a URDF that defines effectors and sensors mounted on named links. | M |
| REQ-SC-003 | The system SHALL validate URDF files at load time and report inconsistencies (missing inertias, unreferenced links, etc.). | M |
| REQ-SC-004 | The system SHALL support time-varying mass properties for fueled craft, with automatic updates as propellant is consumed. | M |
| REQ-SC-005 | The system MAY support flexible-body modal coordinates for solar arrays, antennas, and similar appendages. | C |
| REQ-SC-006 | The system MAY support fuel slosh modeling (pendulum or spring-mass surrogate models). | C |
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
| REQ-GNC-003 | The system SHALL provide a Multiplicative Extended Kalman Filter (MEKF) for attitude estimation. | M |
| REQ-GNC-004 | The system SHALL provide an EKF for orbit estimation with at minimum position, velocity, and Cd as estimated parameters. | M |
| REQ-GNC-005 | The system SHOULD provide a UKF as an alternative orbit estimator. | S |
| REQ-GNC-006 | The system SHALL provide PD and LQR attitude controllers. | M |
| REQ-GNC-007 | The system SHALL provide a stationkeeping controller with deadband-on-elements logic. | M |
| REQ-GNC-008 | The system SHOULD provide an MPC attitude/orbit controller. | S |
| REQ-GNC-009 | The system SHALL provide a finite-state-machine mode logic layer with user-configurable modes and transitions. | M |
| REQ-GNC-010 | The system SHALL allow user-defined controllers and estimators via interfaces. | M |
| REQ-GNC-011 | The system SHALL support modeling of computational delay (controller output applied N ticks after measurement). | S |
| REQ-GNC-012 | The system SHALL support effector failure injection at the message bus layer. | S |
| REQ-GNC-013 | The system SHALL support hardware-in-the-loop mode with the controller running on external flight hardware over a real-time interface. | C |

## 9. Environment models

| ID | Requirement | Priority |
|---|---|---|
| REQ-ENV-001 | The system SHALL provide ephemerides for Sun, Moon, and major planets via SPICE SPK kernels (DE440 or later). | M |
| REQ-ENV-002 | The system SHALL provide planetary orientation via SPICE PCK kernels. | M |
| REQ-ENV-003 | The system SHALL implement IGRF-13 (or current generation) geomagnetic field. | M |
| REQ-ENV-004 | The system SHOULD support World Magnetic Model (WMM) as an alternative. | C |
| REQ-ENV-005 | The system SHALL provide solar and geomagnetic indices (F10.7, Ap, Kp) for atmospheric and space-weather modeling. | M |
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
| REQ-CAT-006 | The system SHALL apply orbit-based pre-filters (apogee/perigee, inclination) before geometric screening. | M |
| REQ-CAT-007 | The system SHALL apply spatial hashing or equivalent broad-phase filter before refined TCA computation. | M |
| REQ-CAT-008 | The system SHALL compute time of closest approach (TCA) and miss distance for candidate pairs to better than 1 second / 10 m precision. | M |
| REQ-CAT-009 | The system SHALL compute probability of collision (Pc) using Foster's, Akella-Alfriend's, or Chan's method when covariance is available. | M |
| REQ-CAT-010 | The system SHALL emit conjunction events with full TCA/miss/Pc data when Pc exceeds a configurable threshold. | M |
| REQ-CAT-011 | The system SHOULD provide an automated avoidance maneuver planner that produces minimum-Δv along-track burns to reduce Pc below threshold. | S |
| REQ-CAT-012 | The system MAY perform full N×N conjunction screening across the catalog (not just against active spacecraft). | C |

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
| REQ-SCN-007 | The system SHALL serialize scenarios to disk and reload them with bit-identical behavior. | S |
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
