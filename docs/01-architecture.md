# Apsis — System Architecture Overview

> **Status:** Draft v0.1
> **Scope:** High-fidelity spaceflight simulator for satellite engineering, GNC development, Monte Carlo verification, and full-catalog conjunction analysis.

---

## 1. Goals

Apsis is a flight-dynamics-grade simulator built around four primary use cases:

1. **Single-spacecraft mission simulation** at engineering fidelity — multi-body dynamics, full force model, closed-loop GNC, interplanetary capability.
2. **Control system development** — swappable controllers/estimators, proper continuous-discrete plant/controller separation, rapid iteration.
3. **Monte Carlo verification** — thousands of trials with deterministic seeding, parallel execution, statistical aggregation.
4. **Catalog-scale propagation** — ~50,000 tracked objects with conjunction screening alongside the user's active spacecraft.

The design explicitly rejects two common shortcuts. It does not use a generic game-engine physics package as the dynamics core (precision and structure-preservation matter). It does not impose a single architectural pattern across all layers (ECS for the world, classical OO for per-spacecraft dynamics, message-passing for GNC).

## 2. Design principles

**Conditioning over precision.** The simulator runs entirely in `f64`. Numerical robustness comes from frame partitioning, Encke-style perturbation formulations, compensated summation, and structure-preserving integrators — not from wider word sizes. This is the same discipline that lets professional astrodynamics codes (MONTE, GMAT, Orekit) hold satellites stable over decades in `f64`.

**Hierarchy of inertial frames.** All dynamics live in inertial frames where Newton's laws apply directly. Body-fixed frames are used only at boundaries (surface coordinates, magnetic fields, atmosphere-relative velocity). Frame transitions happen at precision-rich boundaries (atmosphere edges, SOI crossings) rather than mid-orbit.

**Analytical where possible, numerical where necessary.** Keplerian conic propagation, SGP4, and analytical perturbation theories are used during cruise phases. Numerical integration is reserved for segments with non-conservative forces, active control, or close-body interactions. This is what makes time acceleration tractable.

**Plant and controller are separable.** The continuous-time dynamics and the discrete-time GNC stack communicate via a typed message bus. Controllers run at their declared sample rates with explicit zero-order hold. Failure injection and timing perturbation are first-class.

**Tiered fidelity per entity.** Active spacecraft get full multi-body dynamics. Catalog objects get SGP4. Debris gets J2-only. Each tier is appropriate to its purpose and computational budget.

**Data-oriented where it pays off.** ECS for the world (many similar entities, bulk operations, snapshot/restore). Classical OO for tightly-coupled internal dynamics (multi-body trees). Message-passing for GNC plumbing. The pattern follows the data shape, not a single dogma.

**Reuse battle-tested libraries.** SPICE for ephemerides and frames. SOFA for time and IAU precession-nutation. Pinocchio for multi-body dynamics. NRLMSISE-00 for atmosphere. EGM2008 coefficients for geopotential. IGRF for magnetic field. Build the things that don't exist; don't rebuild the things that do.

## 3. Layered architecture

```
┌──────────────────────────────────────────────────────────────────┐
│  Scenario Layer (Python via pybind11)                            │
│  Mission timeline, initial conditions, MC campaigns              │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│  Orchestration                                                   │
│  Time stepping (event-driven), MC harness, recording             │
└────────────────────────────┬─────────────────────────────────────┘
                             │
              ┌──────────────┼──────────────┐
              ▼              ▼              ▼
┌──────────────────┐ ┌──────────────┐ ┌────────────────┐
│  ECS World       │ │  GNC Stack   │ │  Spacecraft    │
│  - Bodies        │ │  - Estimator │ │  Internals     │
│  - Catalog       │◄┤  - Control   │►│  (Pinocchio    │
│  - Spacecraft    │ │  - Sensors   │ │   MBD, URDF,   │
│    handles       │ │  - Effectors │ │   effectors)   │
└────────┬─────────┘ └──────┬───────┘ └───────┬────────┘
         │                  │ messages        │
         └────────┬─────────┴─────────────────┘
                  ▼
┌──────────────────────────────────────────────────────────────────┐
│  Dynamics Core                                                   │
│  Integrators │ Force models │ Frames │ State conversions         │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│  Foundation                                                      │
│  Time (SOFA) │ Ephemerides (SPICE) │ Math (Eigen) │ Logging      │
└──────────────────────────────────────────────────────────────────┘
```

### Foundation

**Time system.** TAI/TT/UTC/UT1/TDB with leap-second tables and IERS Bulletin A polar motion / DUT1 data. Two-component time representation (`epoch_jd + offset_seconds`, both `f64`) for nanosecond precision over multi-decade arcs. SOFA-backed.

**Ephemerides.** SPICE kernels (SPK for positions, PCK for body orientations, FK for frame definitions, LSK for leap seconds, CK for spacecraft attitude history). `SpkEzr`-style queries return position and velocity of any body in any frame at any TDB epoch.

**Frames.** Right-handed throughout. Inertial root: ICRF/J2000. Body-centered inertial (CCI): one per major body. Body-centered fixed (CCF): rotating with the body. Per-spacecraft frames (vehicle, NTW, RSW, LVLH) for GNC. IAU 2006/2000A precession-nutation for ICRF↔ITRF.

**Math.** Eigen for linear algebra. Custom symplectic integrators where Eigen-based ODE libraries don't suffice. Compensated summation utilities.

### Dynamics core

**Force model.** Pluggable `ForceModel` interface. Each force is a class (`PointMassGravity`, `SphericalHarmonicGravity`, `ThirdBody`, `AtmosphericDrag`, `SolarRadiationPressure`, `EarthRadiationPressure`, `RelativisticCorrection`, `EmpiricalAcceleration`). Each provides `acceleration(state, t)` and `partial_dadr()` for variational equations. Force lists are configured per-entity and can be enabled/disabled at runtime.

**Integrators.** Multiple, selected per regime:
- Dormand-Prince 8(7) adaptive RK — general purpose
- Gauss-Jackson 8th-order — fixed-step, Earth satellite workhorse
- Yoshida 8th-order symplectic — long-arc gravity-only
- Bulirsch-Stoer — high-precision when needed
- Keplerian analytical — cruise phases
- SGP4 — catalog objects

**State conversions.** Cartesian (canonical), classical orbital elements, modified equinoctial elements, Brouwer-Lyddane mean elements, TLE-compatible. Quaternions (canonical) ↔ DCM ↔ Euler ↔ MRPs for attitude.

**Encke-style propagation.** Optional formulation that integrates deviations from a reference Keplerian orbit. The reference is propagated analytically; only the perturbation correction is integrated numerically. This drastically reduces cancellation error on long arcs.

### ECS world

The world holds **everything that exists** in the simulation. Built on EnTT or flecs.

**Components (universal):**
- `OrbitalState { Vector3d r, v; FrameId frame; }`
- `Mass { double m; }` — supports time-varying for fueled craft
- `Covariance { Matrix6d P; }` — for catalog objects with uncertainty

**Components (markers / fidelity tier):**
- `CelestialBody` — Sun, planets, moons; ephemeris-driven
- `ActiveSpacecraft` — full propagator, multi-body, GNC; references SpacecraftDynamics
- `CatalogObject` — SGP4-propagated, low-fidelity
- `Debris` — J2-only, lightweight

**Components (capabilities):**
- `DragProperties { Cd, area_to_mass }`
- `SRPProperties { Cr, area_to_mass }`
- `Atmosphere { model_id }` — for bodies with atmospheres
- `MagneticField { model_id }`
- `SOI { radius }`
- `GroundStation { lat, lon, alt, antenna_pattern }`

**Systems:**
- `EphemerisSystem` — bulk-update celestial body positions from SPICE
- `SGP4PropagationSystem` — bulk-propagate catalog objects (SoA hot loop)
- `HighFidelityPropagationSystem` — drives the integrator for active spacecraft
- `SOITrackerSystem` — determine which body's SOI each spacecraft is in
- `SpatialIndexSystem` — maintain conjunction-screening data structure
- `ConjunctionScreeningSystem` — detect candidate close-approach pairs
- `EclipseSystem`, `GroundContactSystem`, etc.

### Spacecraft internals (per active spacecraft)

A `SpacecraftDynamics` object referenced from the ECS by handle. Owns:

- **Pinocchio model** built from URDF — the kinematic tree, link inertias, joints
- **Pinocchio data** — working storage for forward/inverse dynamics
- **Effectors** — thrusters, reaction wheels, CMGs, magnetorquers, gimbal drives
- **Sensors** — star tracker, sun sensor, magnetometer, GPS, gyros, accelerometers
- **Internal state** — joint positions, joint rates, wheel speeds, fuel mass, etc.
- **Floating-base coupling** — the URDF root joint is the orbital state from the ECS

The URDF describes geometry and inertia; a sidecar configuration file (YAML) describes effectors and sensors attached to named links. URDF flexibility (modal coordinates) is out of scope for v1; rigid-body only.

### GNC stack

Each active spacecraft has an attached GNC stack consisting of:

- **Sensors** — produce `Measurement` messages at their declared rates with realistic noise/bias models
- **Estimator** — consumes measurements, produces state estimates (typically MEKF for attitude, EKF/UKF for orbit)
- **Controller** — consumes estimates, produces effector commands at its declared rate
- **Effectors** — consume commands, produce forces/torques on links

Components communicate via a typed message bus. Multi-rate scheduling: sensors at e.g. 100 Hz, estimator at 100 Hz, attitude controller at 50 Hz, orbit controller at 0.1 Hz. The orchestration layer aligns integration with the highest-rate GNC tick.

### Orchestration

**Event-driven time stepping.** The integrator advances to "next event" — controller tick, sensor sample, scheduled maneuver, eclipse boundary, SOI crossing, ground station AOS/LOS, mission phase transition. Between events, the integrator runs as freely as adaptive stepping permits. This is the lever that makes time acceleration tractable.

**Monte Carlo harness.** Snapshot-restore on the ECS registry. Per-trial RNG seeded from the campaign master seed. Trial-level perturbation injection (initial conditions, parameters, sensor bias). Parallel execution across worker processes. Result aggregation to Parquet/HDF5.

**Recording.** Subscriber-based telemetry recorder. Subscribes to events and named state variables, writes only what is requested. Avoids the trap of dumping full trajectories by default.

### Scenario layer

Python via pybind11. A scenario file describes:
- Initial conditions (epoch, state, attitude)
- Spacecraft configuration (URDF path, sensor/effector config)
- Force model selection
- GNC stack selection
- Mission timeline (events, maneuvers, mode transitions)
- Termination conditions
- Recording configuration
- For MC campaigns: sample distributions, trial count, parallelism

## 4. What this architecture is NOT

**Not a game engine.** No render loop drives the sim. No frame budget. The sim runs to event boundaries; rendering, if attached, samples the sim state asynchronously.

**Not a real-time-only simulator.** Real-time pacing is a configurable layer on top of the core simulator, not baked into it. Hardware-in-the-loop mode exists but is one of several execution modes.

**Not single-threaded by default but not aggressively parallel within a trial.** Per-trial execution is single-threaded for determinism. Parallelism is across MC trials and across catalog objects.

**Not a mission design tool.** Apsis executes missions. Mission design (Lambert, pork-chop, low-thrust optimization) is a separate offline pipeline whose outputs feed Apsis as scenario inputs. GMAT and PyKEP are appropriate companions.

## 5. External dependencies (build-vs-reuse)

| Capability | Library | Notes |
|---|---|---|
| Time conversions, IAU models | SOFA (C) | Official IAU library |
| Ephemerides, frames, SPK/PCK kernels | SPICE / CSPICE | JPL NAIF, free |
| Multi-body dynamics | Pinocchio (C++) | Featherstone algorithms, autodiff support |
| URDF parsing | urdfdom (C++) | ROS standard |
| Linear algebra | Eigen (C++) | Header-only, ubiquitous |
| ODE integrators (standard) | Boost.Numeric.Odeint | RK45, RK78 |
| ECS framework | EnTT or flecs | Header-only / batteries-included |
| Atmospheric density | NRLMSISE-00 reference | Free FORTRAN, C ports |
| Geomagnetic field | IGRF-13 reference | Free FORTRAN, C ports |
| Geopotential coefficients | EGM2008 | Free, ICGEM hosts |
| TLE / SGP4 | Vallado SGP4 reference | Canonical implementation |
| Python bindings | pybind11 | C++/Python glue |
| Serialization | Cap'n Proto or Protobuf | For checkpoints, telemetry |
| Result storage | Parquet (Apache Arrow) | Columnar, pandas-friendly |

## 6. What gets built

The novel engineering — and the project's value-add — is in:

- The **orchestration layer** (event-driven time stepping with multi-rate GNC)
- The **floating-base coupling** between Pinocchio and the orbital propagator
- The **GNC plant/controller architecture** (message bus, sample-and-hold, failure injection)
- **Encke-style perturbation propagation** built on top of analytical references
- The **Monte Carlo harness** (snapshot/restore, deterministic seeding, parallel execution, aggregation)
- The **catalog/conjunction pipeline** (SoA SGP4, spatial indexing, refinement)
- The **scenario DSL** in Python
- **Tiered-fidelity entity management** — active spacecraft, catalog, debris in the same world

Everything else is glue around well-tested libraries.

## 7. Phased build plan

| Phase | Duration | Deliverable |
|---|---|---|
| 1. Propagator core | 6-8 weeks | Single spacecraft, force models, integrators, SPICE integration. Validate against published ephemerides. |
| 2. ECS world + catalog | 3-4 weeks | EnTT/flecs integration, SGP4 catalog, spatial index, conjunction screening. |
| 3. Multi-body / URDF | 6-8 weeks | Pinocchio integration, floating-base coupling, effectors, validate angular momentum conservation. |
| 4. GNC layer | 4-6 weeks | Sensor models, MEKF, basic controllers, message bus, multi-rate scheduling. |
| 5. Monte Carlo | 3-4 weeks | Snapshot/restore, RNG management, parallel trials, result aggregation. |
| 6. Scenario layer | 3-4 weeks | pybind11 bindings, Python DSL, examples. |
| 7+. Hardening | ongoing | Higher-fidelity force models, advanced GNC, validation campaigns, documentation. |

End of Phase 6 (~6-8 months sustained effort) produces a usable simulator. Phases 1-2 alone (~10 weeks) are sufficient for trajectory studies and conjunction analysis without GNC.
