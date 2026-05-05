# Apsis — System Architecture Overview

> **Status:** Draft v0.4 (v0.4: Long-arc state conditioning deepening per [[wiki/concepts/long-arc-state-conditioning]] and [[wiki/decisions/003-tagged-time-scale-types]]; v0.3: Variational Equations deepening per [[wiki/concepts/variational-equations]] and [[wiki/decisions/002-variational-equations-between-measurements]]; v0.2: revised against wiki audit 2026-05-05; see [[wiki/synthesis/audit-summary-2026-05-05]])
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

**Conditioning over precision.** The simulator runs entirely in `f64`. Numerical robustness comes from the three pillars of [[wiki/concepts/long-arc-state-conditioning|long-arc state conditioning]] — two-component time, Encke-style perturbation propagation, and compensated summation — plus frame partitioning at precision-rich boundaries and structure-preserving integrators. Not from wider word sizes. This is the same discipline that lets professional astrodynamics codes (MONTE, GMAT, Orekit) hold satellites stable over decades in `f64`.

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

**Time system.** TAI/TT/UTC/UT1/TDB with leap-second tables and IERS Bulletin A polar motion / DUT1 data, plus optional TCG/TCB for relativistic-strict use cases. Two-component time representation (`epoch_jd + offset_seconds`, both `f64`) for nanosecond precision over multi-decade arcs. The `Time` type is **tagged with its scale at the type level** — scale-mixing is a compile-time error per [[wiki/decisions/003-tagged-time-scale-types|ADR-003]]. SOFA-backed.

**Ephemerides.** SPICE kernels (SPK for positions, PCK for body orientations, FK for frame definitions, LSK for leap seconds, CK for spacecraft attitude history). `SpkEzr`-style queries return position and velocity of any body in any frame at any TDB epoch.

**Frames.** Right-handed throughout. Inertial root: **ICRF** (current realization ICRF3); the J2000 mean-equator-and-equinox frame is treated as a distinct frame related to ICRF by the constant ~17 mas frame bias. Body-centered inertial (CCI): one per major body. Body-centered fixed (CCF): rotating with the body. Per-spacecraft frames (vehicle, NTW, RSW, LVLH) for GNC. IAU 2006/2000A precession-nutation for ICRF↔ITRF via the **CIO-based pipeline** (see ADR-001) implemented through SOFA `iauC2t06a` and supporting routines.

**Math.** Eigen for linear algebra. Custom symplectic integrators where Eigen-based ODE libraries don't suffice. Compensated summation utilities.

### Dynamics core

**Force model.** Pluggable `ForceModel` interface. Each force is a class (`PointMassGravity`, `SphericalHarmonicGravity`, `ThirdBody`, `AtmosphericDrag`, `SolarRadiationPressure`, `EarthRadiationPressure`, `RelativisticCorrection`, `EmpiricalAcceleration`). Each implements the **Variational Equations contract** ([[wiki/concepts/variational-equations]]): `acceleration(state, t)` plus `partial_dadr` / `partial_dadv` / `partial_dadp` (the last for statically-declared estimable parameters such as Cd, Cr). Analytical partials are required by default; force models without them opt in to a centralised finite-difference helper. A generic conformance harness (CI gate) verifies analytical partials against finite differences for every registered force model. Force lists are configured per-entity and can be enabled/disabled at runtime.

**Integrators.** Multiple, selected per regime:
- Dormand-Prince 8(7) adaptive RK — general purpose
- Gauss-Jackson 8th-order — fixed-step, Earth satellite workhorse
- Yoshida 8th-order symplectic — long-arc gravity-only
- Bulirsch-Stoer — high-precision when needed
- Keplerian analytical — cruise phases
- SGP4 — catalog objects
- **Variational Equations integrator** — propagates the state-transition matrix Φ between measurement / observation epochs, atop the dense output of whichever natural-state integrator is active. Default RK4 for the linear ODE `dΦ/dt = A·Φ`. Φ is propagated on demand, not as augmented natural state ([[wiki/decisions/002-variational-equations-between-measurements|ADR-002]])

**State conversions.** Cartesian (canonical), classical orbital elements, modified equinoctial elements, Brouwer-Lyddane mean elements, TLE-compatible. Quaternions (canonical) ↔ DCM ↔ Euler ↔ MRPs for attitude.

**Encke-style propagation.** Optional **wrapper** around any base integrator (DOPRI 8(7), GJ8, Yoshida 8, etc.) that integrates deviations from a Keplerian reference orbit. The reference is propagated analytically; only the perturbation correction `δr̈` is integrated numerically by the wrapped base integrator. Auto-rectification when deviation exceeds a threshold (default 1% of reference radius). The wrapper exposes absolute-coordinate dense output, so downstream consumers (Variational Equations integrator, etc.) need not be Encke-aware. See [[wiki/concepts/long-arc-state-conditioning]] for the rationale; one of three pillars of long-arc precision.

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
- `Debris` — J2-only, lightweight, used for synthetic debris populations and testing (distinct from `CatalogObject` which is SGP4-propagated tracked debris from the SSN catalog; statistical sub-tracking-threshold MMOD flux per ORDEM is OOS for v1)

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

The URDF describes geometry and inertia; a sidecar configuration file (YAML) describes effectors and sensors attached to named links. **Flexible-body modal coordinates** for solar arrays, antennas, and other appendages are supported per the hybrid-coordinate method (Likins 1970 JPL TR 32-1329) — added as URDF-extension elements that the Apsis loader recognizes (vanilla URDF has no native modal-coordinate support). **Slosh modes** are added as additional pendulum joints in the URDF tree, anchored at the tank link, parameterized per Abramson 1966 NASA SP-106 and Dodge 2000.

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
| Time conversions, IAU models | SOFA (C) v18+ | Official IAU library; permissive license |
| Ephemerides, frames, SPK/PCK kernels | SPICE / CSPICE | JPL NAIF, free; CSPICE is thread-unsafe — serialize or per-thread instance |
| Multi-body dynamics | Pinocchio (C++) | Featherstone algorithms (ABA / RNEA / CRBA), **analytical derivatives** (Carpentier & Mansard 2018 — 30-60% faster than autodiff+codegen, numerically exact), autodiff support |
| URDF parsing | urdfdom (C++) | ROS standard; complemented by Xacro for templating |
| Linear algebra | Eigen (C++) | Header-only, ubiquitous |
| ODE integrators (standard) | Boost.Numeric.Odeint | RK45, RK78 |
| ECS framework | EnTT or flecs | Header-only / batteries-included |
| Atmospheric density (default) | NRLMSISE-00 reference | Free FORTRAN, C ports |
| Atmospheric density (alternative) | JB2008 reference | Free C/Fortran from Space Environment Technologies (`sol.spacenvironment.net`); requires F10/S10/M10/Y10/Dst space-weather indices |
| Geomagnetic field | IGRF-14 reference (or current) | Free FORTRAN, C ports; coefficient text file from IAGA / NOAA NCEI / BGS |
| Geopotential coefficients | EGM2008 (Earth), GRGM900C/GRGM1200A (Moon) | Free; vendored EGM2008, additional models from ICGEM (`icgem.gfz.de`) |
| TLE / SGP4 | Vallado SGP4 reference (WGS-72 constants) | Canonical implementation; WGS-72 per STR#3 to match operational AFSPC predictions |
| CCSDS CDM ingest | Custom parser (KVN + XML) | CCSDS 508.0-B-1 Blue Book is small; no dominant third-party C++ library |
| Python bindings | pybind11 | C++/Python glue |
| Serialization | Cap'n Proto or Protobuf | For checkpoints, telemetry |
| Result storage | Parquet (Apache Arrow) | Columnar, pandas-friendly |

## 6. What gets built

The novel engineering — and the project's value-add — is in:

- The **orchestration layer** (event-driven time stepping with multi-rate GNC)
- The **floating-base coupling** between Pinocchio and the orbital propagator
- The **GNC plant/controller architecture** (message bus, sample-and-hold, failure injection)
- The **Long-arc state conditioning subsystem** ([[wiki/concepts/long-arc-state-conditioning]]) — three pillars: two-component time (tagged at type level per [[wiki/decisions/003-tagged-time-scale-types|ADR-003]]), Encke wrapper over base integrators, compensated (Kahan-Neumaier) summation primitives. Together they let `f64` hold satellites stable over decades.
- The **Variational Equations subsystem** ([[wiki/concepts/variational-equations]]) — per-force partials contract, framework assembly of A, Φ propagation between measurements; bridges force models to orbit estimation and Pc covariance propagation
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
| 3. Multi-body / URDF | 6-8 weeks | Pinocchio integration, floating-base coupling, effectors, flexible-body modal coordinates (per Likins 1970 hybrid-coordinate method) and slosh DOFs (per Abramson/Dodge), validate angular momentum conservation. |
| 4. GNC layer | 4-6 weeks | Sensor models, MEKF, basic controllers, message bus, multi-rate scheduling. |
| 5. Monte Carlo | 3-4 weeks | Snapshot/restore, RNG management, parallel trials, result aggregation. |
| 6. Scenario layer | 3-4 weeks | pybind11 bindings, Python DSL, examples. |
| 7+. Hardening | ongoing | Higher-fidelity force models, advanced GNC, validation campaigns, documentation. |

End of Phase 6 (~6-8 months sustained effort) produces a usable simulator. Phases 1-2 alone (~10 weeks) are sufficient for trajectory studies and conjunction analysis without GNC.
