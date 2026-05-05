# Apsis — Subsystem Design

> Companion to `01-architecture.md`. Detailed designs for each major subsystem.
>
> **Status:** Draft v0.3 (v0.3: Variational Equations deepening per [[wiki/concepts/variational-equations]] and [[wiki/decisions/002-variational-equations-between-measurements]]; v0.2: revised against wiki audit 2026-05-05; see [[wiki/synthesis/audit-summary-2026-05-05]])

---

## 1. Time and frames

### 1.1 Time scales

The simulator maintains explicit conversions between five primary time scales, plus two optional scales for relativistic-strict use:

- **TAI** — International Atomic Time, monotonic, no leap seconds
- **TT** — Terrestrial Time, TT = TAI + 32.184 s (offset only)
- **UTC** — coordinated universal, has leap seconds; for human-readable I/O
- **UT1** — Earth rotation time; UTC + DUT1 from IERS Bulletin A
- **TDB** — Barycentric Dynamical Time; for ephemeris evaluation, differs from TT by periodic terms ≤2 ms
- **TCG** (optional) — Geocentric Coordinate Time; relativistic GCRS coordinate time
- **TCB** (optional) — Barycentric Coordinate Time; relativistic BCRS coordinate time, JPL DE ephemeris time argument

Internal computation uses TT for spacecraft propagation and TDB for ephemeris queries; UTC and UT1 are used only at boundaries. Apsis defaults to TT-rate-scaled GCRS coordinates (the engineering norm); TCG/TCB are available for strict relativistic work but not used in default-precision force models.

### 1.2 Time representation

Time is stored as `(epoch_jd: f64, offset_seconds: f64)`. The epoch is a reference Julian Date (typically J2000 = 2451545.0 TT). The offset is seconds since the epoch. This preserves nanosecond precision for offsets up to ±10^7 seconds (roughly four months) before requiring epoch rectification.

Single-`f64` Julian dates lose ~12 µs of precision near J2000 and worse over decades. Two-component time is non-negotiable for multi-decade missions.

### 1.3 Frame hierarchy

All frames are right-handed.

**Inertial root: ICRF** (current realization ICRF3 per Charlot et al. 2020). All other inertial frames are constant rotations of this. The **J2000 mean-equator-and-equinox** frame is treated as a distinct frame, related to ICRF by the constant ~17 mas frame bias per Kaplan 2005 (USNO Circular 179) §3. Apsis applies the frame bias explicitly when converting between ICRF and J2000.

**Body-Centered Inertial (CCI) per body.** Origin at body center, axes aligned with ICRF (or with body's equatorial pole and vernal point, depending on body). For Earth: GCRF. For Mars: MCI. Inertial — orbital state propagation lives here.

**Body-Centered Fixed (CCF) per body.** Origin at body center, rotating with body. Z to north pole, X through prime meridian. For Earth: ITRF (with full IAU 2006/2000A precession-nutation, polar motion, ERA). Non-inertial — used for surface coordinates, atmospheric velocity, magnetic field evaluation.

**Heliocentric Ecliptic (HCI / ECL).** Sun-centered, ecliptic plane reference. For interplanetary cruise.

**Per-spacecraft frames.** Vehicle body frame defined per spacecraft (typically aligned to instrument boresight or bus mechanical axes; mission-specific). Orbital frames LVLH (Local Vertical Local Horizontal: +Z to nadir, +Y opposite orbit normal, +X completing right-hand triad), RSW (Radial / Along-track / Cross-track), and NTW (in-track / Normal / cross-track Width) for GNC and analysis. Body-to-orbital-frame DCMs are quaternion-derived per the chosen attitude representation.

### 1.4 Frame transitions

Transitions happen at precision-rich boundaries:

- **CCI ↔ CCF** at ~atmosphere edge (typically 100 km for Earth). f64 supports ~nm absolute precision at LEO radii (ULP at 7×10⁶ m ≈ 1.5 nm); the practical mm-level uncertainty floor at LEO comes from physical-model fidelity (tides, ephemeris, force-model truncation), not f64 arithmetic.
- **CCI ↔ HCI** at sphere of influence boundaries.

The state vector is transformed and the integrator continues in the new frame. CCI ↔ CCF transitions are mathematically continuous in position (f64 LSB only). CCI ↔ HCI transitions at SOI are continuous in position but the primary attractor switches discontinuously; adaptive integrators may need to re-bracket step size at the boundary as the dominant gravitational source changes.

## 2. Force models

### 2.1 Interface

```cpp
class ForceModel {
public:
    virtual Vector3d acceleration(const State& s, const Time& t) const = 0;

    // Variational Equations contract — see [[wiki/concepts/variational-equations]].
    virtual Matrix3d partial_dadr(const State& s, const Time& t) const = 0;
    virtual Matrix3d partial_dadv(const State& s, const Time& t) const = 0;
    virtual std::vector<ParameterPartial> partial_dadp(const State& s, const Time& t) const {
        return {};   // default: no estimable parameters
    }
    virtual std::vector<std::string> estimable_parameters() const { return {}; }

    // Conformance verification (REQ-PHY-020):
    virtual std::vector<TestPoint> conformance_grid() const { return DEFAULT_GRID; }
    virtual double conformance_tolerance() const { return 1e-8; }

    virtual std::string name() const = 0;
    virtual bool is_conservative() const = 0;  // for symplectic integrator selection
};

struct ParameterPartial {
    std::string name;       // matches estimable_parameters() order
    Vector3d daudp;         // ∂a/∂p_i
};
```

A spacecraft's force list is a `std::vector<std::unique_ptr<ForceModel>>`. The propagator sums contributions. Per-force enable flags and timing instrumentation are first-class.

**Analytical partials are required by default.** Force models without analytical partials (table-lookup atmospheres, prototype empirical forces, user-defined perturbations) opt in to a centralised finite-difference helper:

```cpp
class MyEmpiricalForce : public ForceModel {
public:
    Matrix3d partial_dadr(const State& s, const Time& t) const override {
        return FiniteDifferenceJacobian::dadr(*this, s, t);
    }
    // ... etc.
};
```

The opt-in is deliberate (not a base-class default) so the analytical-vs-FD choice surfaces at the force-model declaration site, not silently inside the EKF loop. The same `FiniteDifferenceJacobian` helper is used by the conformance harness to verify analytical implementations. See [[wiki/concepts/variational-equations]] for the full contract and [[wiki/decisions/002-variational-equations-between-measurements|ADR-002]] for the framework-side propagation choice.

### 2.2 Gravity

**Point-mass gravity.** Trivial. `a = -μ r̂ / r²`.

**Spherical harmonic gravity.** EGM2008 coefficients to user-selected degree/order. Pines formulation (singularity-free at poles). Pre-normalized coefficients. Recursion is O(N²) in degree N. Typical configurations:
- LEO high-fidelity: 70×70
- GEO: 12×12 (tesseral terms matter)
- Lunar: GRGM1200A to 165×165 for low orbits
- Game-grade: 8×8

**Third-body gravity (Battin formulation).** Avoids cancellation when the spacecraft is much closer to the central body than to the third body:

```
a₃ = -(μ₃ / |s - r|³) [r + f(q) · s]
```

where `s` is the third-body position, `r` is the spacecraft position, and `f(q)` is a stable polynomial in `q = r·(r-2s)/|s|²`.

Sun, Moon, and major planets included by default for Earth-orbiting craft. SPICE provides positions.

**Solid Earth tides, ocean tides, pole tides.** Per IERS Conventions 2010 Ch. 7-8 (REQ-PHY-014, S). Centimeter-level effects on satellite position. **Default-on for conjunction-assessment scenarios** (where 10 cm matters relative to the 10 m miss-distance specification); default-off for low-precision use cases. Apsis uses the **zero-tide** permanent-tide convention per IERS Conventions Ch. 6 default (REQ-PHY-019); mismatched gravity-coefficient files are detected and either converted or flagged.

### 2.3 Atmospheric drag

```
a_drag = -½ ρ Cd (A/m) v_rel |v_rel|
```

`v_rel` is computed in the body-fixed frame (subtract `ω_body × r` from inertial velocity). `ρ` comes from a configured atmosphere model:

- Exponential — toy/sanity-check
- US Standard Atmosphere 1976 — static, no solar dependence
- NRLMSISE-00 — operational standard, requires F10.7 and Ap indices
- JB2008 — alternative, slightly better at high altitudes

Solar/geomagnetic indices loaded from CelesTrak space-weather files; supports both historical data and prediction.

`Cd` is configured per-spacecraft (default 2.2). v1 ships **cannonball drag only** (single Cd, single area); high-fidelity multi-panel drag with attitude-dependent accommodation coefficients (Sentman / Doornbos formulations) is OOS for v1 (OOS-013).

### 2.4 Solar radiation pressure

```
a_SRP = -ν · Cr · (A/m) · (Φ/c) · (AU/r_sun)² · ŝ
```

`ν` is the shadow factor:
- Cylindrical shadow — fastest, ~1% area error, no penumbra
- Conical shadow — proper umbra/penumbra with Sun and Earth as spheres (default)
- Multi-body shadowing — for cis-lunar missions, account for both Earth and Moon

Cannonball model by default (single Cr, single area). N-plate model available for high-fidelity work where attitude affects SRP appreciably.

### 2.5 Earth radiation pressure

Albedo (reflected sunlight) and Earth IR. Knocke's model. ~10-20% of direct SRP for low-altitude satellites. Off by default; on for precision orbit determination work.

### 2.6 Relativistic corrections

**Schwarzschild correction** for the central body (REQ-PHY-012, M) per IERS Conventions 2010 Ch. 10. **Default-on** for the gravitational acceleration term — small but secular, matters for multi-decade arcs and high-precision orbit determination. Lense-Thirring and de Sitter terms (REQ-PHY-013, C) are available but **default-off**; relevant only for very-precise GNSS, GRACE, Lageos-class scenarios. Shapiro delay (light-time relativistic correction) is **default-off** and only enabled for deep-space tracking observables.

### 2.7 Empirical accelerations

Catch-all for unmodeled effects. Typically a constant acceleration in RSW or NTW frame, estimated from tracking data. Useful for orbit determination and for "fudge factor" studies.

### 2.8 Thrust

Modeled separately from natural forces — driven by effector commands rather than the world.

- **Impulsive maneuvers** — instantaneous Δv at scheduled times
- **Finite burns** — `a_thrust = (T/m) û_thrust` integrated over the burn, with `dm/dt = -T/(g₀·Isp)`
- **Continuous low-thrust** — same as finite burn but with duration measured in days/weeks

## 3. Integrators

### 3.1 Selection guidance

| Integrator | Use case |
|---|---|
| Keplerian analytical | Cruise phases, no perturbations active |
| SGP4 | Catalog objects from TLEs |
| Dormand-Prince 8(7) | General purpose, adaptive, default |
| Gauss-Jackson 8 | Earth satellites, fixed-step, very stable |
| Yoshida 8 (symplectic) | Long-arc gravity-only, energy-conserving |
| Bulirsch-Stoer | High-precision validation runs |

### 3.2 Integrator interface

```cpp
class Integrator {
public:
    virtual StepResult step(State& s, double dt_max, const ForceModel& f, EventDetector& ev) = 0;
    virtual void rectify_reference() = 0;  // for Encke-style only
    virtual State interpolate(double t_query) const = 0;  // dense output
};
```

`dt_max` is the largest step the integrator may take; events shrink it. Adaptive integrators may take smaller steps internally for accuracy. `interpolate` provides continuous output between integration nodes — needed for event localization and for ground-truth generation at arbitrary times.

### 3.3 Compensated summation

All integrators use Kahan-Neumaier summation for state accumulation:

```cpp
template<typename T>
class CompensatedSum {
    T sum;
    T compensation;
public:
    void add(T value);  // Neumaier-corrected
    T value() const;
};
```

Recovers ~1 ULP per step, essentially free, transforms long-arc behavior.

### 3.4 Encke-style propagation

Optional formulation: integrate the deviation from a reference Keplerian orbit rather than the absolute state.

```
r_actual(t) = r_kepler(t) + δr(t)
δr̈ = a_perturb(r_actual, t) - μ/|r_kepler|³ · (r_actual - r_kepler)  [Battin's form]
```

The reference is propagated analytically. Only `δr` is numerically integrated. State magnitudes are kilometers, not 10^7 meters. Cancellation is structurally avoided.

When `|δr|` exceeds a threshold (typically 1% of orbit radius), the reference is **rectified** — set to the current actual state, `δr` reset to zero. Rectification is free; it just resets the analytical reference.

### 3.5 Event detection

The integrator integrates `g(s, t) = 0` event functions alongside the dynamics. When a sign change is detected between integration nodes, the event is localized via Brent's method or similar root-finder using the dense output. Events:

- Periapsis / apoapsis passage (dot product `r · v = 0`)
- Eclipse entry / exit (geometric occlusion)
- SOI crossing (`|r - r_body| - r_SOI = 0`)
- Altitude triggers (atmospheric entry, etc.)
- Ground station AOS / LOS (elevation angle = 0)
- Scheduled time triggers (maneuvers, mode transitions)

Detected events trigger callbacks that may modify state, switch frames, or terminate the integration step.

### 3.6 Variational Equations integrator

A dedicated module ([[wiki/concepts/variational-equations]]) propagates the **state-transition matrix Φ** between measurement / observation epochs, consuming per-force partials (REQ-PHY-016) and the natural-state integrator's dense output (REQ-INT-008). Φ is **not** carried in the natural-state vector — see [[wiki/decisions/002-variational-equations-between-measurements|ADR-002]] for the rationale.

```cpp
class VariationalEquationsIntegrator {
public:
    // Propagate Φ from t0 to t1 given the natural state's dense output
    // and the per-spacecraft force-model list.
    MatrixXd propagate(
        const StateHistory& state_dense_output,
        Time t0, Time t1,
        const std::vector<const ForceModel*>& forces
    );  // returns Φ(t1, t0); initialised internally as Φ(t0, t0) = I.
};
```

Internally:

1. Initialises `Φ = I_{(6+n_p) × (6+n_p)}` at `t₀` (where `n_p` is the count of estimable parameters declared across the force list).
2. Steps from `t₀` to `t₁` using **RK4** by default — Φ's ODE `dΦ/dt = A(t)·Φ` is linear, so the symplectic / Gauss-Jackson machinery used for the natural state is unnecessary.
3. At each step, queries the natural-state integrator's dense output for `(r, v)`, evaluates each force's `partial_dadr` / `partial_dadv` / `partial_dadp` at that state, sums into `A(t) = [[0, I, 0]; [Σ ∂a/∂r, Σ ∂a/∂v, Σ ∂a/∂p]; [0, 0, 0]]`, advances Φ.
4. Step size matches the natural-state integrator's dense-output node spacing — partials are evaluated at the same `(r, v)` the natural state actually passed through.
5. Returns `Φ(t₁, t₀)`.

Consumers:
- **Orbit EKF** (§5.3) — requests `Φ(t_{k+1}, t_k)` between measurement updates.
- **Pc covariance propagation** (§6.4) — requests `Φ(TCA, t_epoch)` over screening windows up to 7 days.

Conformance with the per-force partials interface (REQ-PHY-020) is verified by a generic CI-gated test harness that auto-discovers registered force models and compares analytical partials against finite differences over each model's declared `conformance_grid()`.

## 4. Spacecraft and multi-body

### 4.1 URDF model

Each active spacecraft has a URDF file describing:

- **Links** — rigid bodies with mass, COM, inertia tensor, visual/collision geometry
- **Joints** — kinematic relationships between links (revolute, prismatic, fixed, continuous)
- **Joint limits, dynamics** — position/velocity/effort limits, friction, damping

URDF describes geometry and inertia. It does **not** describe spacecraft-specific elements (thrusters, reaction wheels as control elements, sensors). Those go in a sidecar configuration:

```yaml
spacecraft:
  urdf: ./models/sat_alpha.urdf
  effectors:
    main_thruster:
      type: thruster
      link: thruster_nozzle_link
      direction: [1, 0, 0]   # body frame
      max_thrust: 22.0       # N
      isp: 220.0             # s
    rw_x:
      type: reaction_wheel
      link: rw_x_link
      axis: [1, 0, 0]
      max_torque: 0.1
      max_momentum: 0.5
      inertia: 1.5e-3
  sensors:
    star_tracker_a:
      type: star_tracker
      link: star_tracker_a_link
      sigma_arcsec: 5.0
      rate_hz: 10
      sun_keepout_deg: 30
    gyro:
      type: imu
      link: imu_link
      bias_initial: [0, 0, 0]
      arw: 0.001  # deg/sqrt(hr)
      rrw: 1e-5
      rate_hz: 100
```

### 4.2 Floating base

The URDF root link is the spacecraft's "floating base" — six free DOF in 3D space. Its pose is the spacecraft's orbital state (from the ECS) plus its attitude (quaternion).

Pinocchio's `Model` is built with a 6-DOF free-flyer joint as the root. The orbital propagator updates the floating-base pose; Pinocchio handles internal joint dynamics; aggregate forces and torques are applied to the appropriate links.

### 4.3 Aggregate dynamics

At each integration step:

1. **Gather external forces.** ForceModel evaluates gravity, drag, SRP, etc. Gravity is computed at COM; drag and SRP may be computed per-panel (high fidelity) or at COM (cannonball).
2. **Compute internal forces.** Pinocchio computes joint torques from internal controllers (wheel speed control, gimbal positioning).
3. **Apply effector forces.** Thrusters apply force at their link with their direction. Reaction wheels apply torque on their spin axis. Magnetorquers apply `m × B` torque.
4. **Forward dynamics.** Pinocchio computes joint accelerations (including floating-base acceleration) given the applied forces.
5. **Aggregate to floating base.** Translational acceleration → orbital propagator. Rotational acceleration → attitude propagator.
6. **Integrate joint dynamics.** Internal joint positions/rates are integrated alongside.

Angular momentum conservation is the key validation invariant. With no external torques, total angular momentum (orbital + attitude + wheel + joint) must be constant to integrator precision.

### 4.4 Mass properties

Spacecraft mass is time-varying when fuel is consumed. Each `FuelTank` effector tracks remaining propellant; the URDF inertia for the tank link is updated based on fill fraction. Pinocchio's `Model` is rebuilt when mass properties change. The default rebuild threshold is 1% mass change (configurable per scenario); for missions with large fuel-fraction-changes during finite burns the threshold should be tightened (e.g., 0.1%) to avoid drifting CG/inertia errors.

### 4.5 Effectors

Common interface:

```cpp
class Effector {
public:
    virtual void apply(SpacecraftDynamics& sc, const Command& cmd, const Time& t) = 0;
    virtual void update_state(double dt) = 0;  // for effectors with internal state
    virtual std::string link_name() const = 0;
};
```

**Thruster.** Magnitude and direction in body frame. Throttle command 0-1. Optional valve dynamics (first-order lag). Updates spacecraft mass via `dm = -F·dt/(g₀·Isp)`.

**Reaction wheel.** Wheel speed state. Commanded torque (with saturation). Friction model (viscous + Coulomb). Modeled in URDF as a one-DOF revolute joint between the wheel link and the bus link, with the wheel inertia loaded into Pinocchio about the spin axis. Momentum coupling to spacecraft attitude falls out automatically from Pinocchio's floating-base dynamics.

**Control moment gyro / VSCMG.** Apsis implements the unified **Variable-Speed CMG (VSCMG)** framework per Schaub, Vadali & Junkins (1998), which subsumes single-gimbal CMG (SGCMG, fixed wheel rate), reaction wheel (gimbal frozen), and VSCMG (both DOFs commandable) as configurations of the same equations. Per-device state: gimbal angle γ, gimbal rate γ̇, wheel spin rate Ω. For redundant clusters (N > 3), the steering law uses a **weighted minimum-norm pseudo-inverse** that smoothly transitions from CMG-mode away from singularities to RW-mode near them, providing exact torque tracking even at classical-CMG singular configurations. Singularity geometry remains observable (manipulability `√det(A_γ A_γᵀ)`) but does not cause torque-tracking failure.

**Magnetorquer.** Commanded magnetic dipole. Torque is `m × B`, where `B` is from IGRF at current position in body-fixed frame transformed to body frame.

**Solar array drive.** Joint position controller for the array's pitch/roll axis. Commanded angle; first-order lag to actual angle.

### 4.6 Sensors

Common interface:

```cpp
class Sensor {
public:
    virtual Measurement sample(const SpacecraftDynamics& sc, const Time& t, RNG& rng) = 0;
    virtual double rate_hz() const = 0;
};
```

Sensors observe the *truth* state (spacecraft, environment) and produce noisy `Measurement` messages on the bus.

**Star tracker.** Quaternion measurement, σ ~ arcseconds. Sun/Earth/Moon keepout — measurement marked invalid when bright body is in FOV. Default keepout half-angles: 30° Sun, 20° Earth, 20° Moon (mission-overridable via YAML config).

**Sun sensor.** Unit vector to Sun in body frame. Coarse (degrees) or fine (arcminutes). Invalid in eclipse.

**Magnetometer.** B field in body frame. Hard-iron and soft-iron biases per axis. Cross-talk with magnetorquers (must zero magnetorquer command before sampling, or model the interference).

**GPS receiver.** Position + velocity in ITRF. σ ~ meters. Only valid in LEO with sky visibility (model GPS satellite constellation visibility).

**Gyro / IMU.** Angular rate with bias drift (random walk model: bias is integrated white noise). Angle Random Walk (ARW) and Rate Random Walk (RRW) per axis. Optional g-sensitivity, scale factor errors.

**Accelerometer.** Non-gravitational acceleration in body frame. σ ~ μg. Measures thrust, drag, SRP — useful for thrust calibration and unmodeled-acceleration estimation.

## 5. GNC stack

### 5.1 Plant / controller separation

The continuous-time plant (dynamics + sensors) and discrete-time GNC stack communicate exclusively via a typed message bus. No GNC code reads dynamics state directly; no dynamics code reads GNC state directly.

### 5.2 Multi-rate scheduling

GNC components declare their rates. The orchestrator builds a schedule of GNC ticks. Between ticks, the integrator runs freely with effector commands held at their last commanded values (zero-order hold).

Typical rates:
- Sensors: 1-100 Hz (per sensor)
- Estimator: matches highest sensor rate, typically 10-100 Hz
- Attitude controller: 10-50 Hz
- Orbit controller: 0.01-1 Hz (or event-driven, for maneuvers)
- Effector commands: held until next controller tick

### 5.3 Estimators

**MEKF (Multiplicative Extended Kalman Filter)** for attitude. Global state is a 4-component unit quaternion. Covariance state is the 3-vector **MRP (Modified Rodrigues Parameters)** error representation per Markley (2003), which is non-singular for attitude errors up to 360°. The default Apsis MEKF is the **second-order extension** per Markley 2003 (preserves quaternion normalization to second order in the error, more robust than first-order MEKF for moderate errors). State also includes gyro bias (Farrenkopf model per Lefferts et al. 1982) and optionally other parameters. Updates use star tracker, sun sensor, magnetometer measurements.

For **acquisition mode** (large initial attitude errors where MEKF can fail to converge — Crassidis & Markley 2003 demonstrated this on a single-magnetometer TRMM simulation), Apsis provides a **USQUE (UnScented QUaternion Estimator)** fallback per Crassidis & Markley 2003 — a UKF-attitude variant that uses generalized-Rodrigues-parameter sigma points and converges from large initial errors at ~2× MEKF cost. Mode logic switches from USQUE to MEKF once attitude error is bounded.

**EKF / UKF** for orbit. State is position + velocity + (optional) drag coefficient, SRP coefficient, empirical accelerations. EKF predict step obtains Φ from the **Variational Equations integrator** (§3.6, [[wiki/concepts/variational-equations]]); parameter augmentation (e.g. Cd) consumes `partial_dadp` from the per-force-model interface. UKF (per Wan & van der Merwe 2000 scaled-UT, augmented-state formulation) is the alternative for highly nonlinear regimes or where Jacobian computation is awkward. Updates from GPS, ground tracking, optical navigation.

Both are user-swappable. The estimator interface is:

```cpp
class Estimator {
public:
    virtual void predict(const Time& t) = 0;
    virtual void update(const Measurement& m) = 0;
    virtual EstimatedState current_estimate() const = 0;
};
```

### 5.4 Controllers

**Attitude controllers.**
- **PD on quaternion error** — small-angle / pointing-stability regime (most science modes). Simple, well-understood, but degrades for large rotations.
- **Lyapunov-stable nonlinear** (Schaub, Vadali & Junkins 1998) — globally asymptotically stable on MRP- or quaternion-based attitude error. Default for large-slew maneuvers and acquisition-mode pointing.
- **LQR** around a reference trajectory.
- **MPC** for slew planning under torque/momentum constraints (REQ-GNC-008, S).
- Sliding-mode and adaptive variants — extension points for robustness studies; not in v1 default set.

**Orbit controllers.**
- **Stationkeeping** — deadband around target orbit elements (REQ-GNC-007).
- **Constrained-trajectory RPO MPC** (Di Cairano, Park & Kolmanovsky 2012, REQ-GNC-015 S) — supports LOS cone constraints, terminal velocity matching, debris/obstacle avoidance for proximity operations and rendezvous. Distinct from slew MPC.
- **Formation flying** — relative motion control.
- **Low-thrust trajectory tracking** — Lyapunov-based or NMPC.

**Collision-avoidance maneuver (CAM) planner.** When a CDM-flagged conjunction has Pc above threshold, the CAM planner produces an optimal impulsive Δv per Bombardelli & Hernando-Ayuso (2015): the analytic eigenvector of `MᵀQM` (max-miss-distance objective) or `Mᵀ Q* M` (min-Pc objective). The maneuver direction is generally **not** along-track; restricting to along-track is a constrained-mode option. See §6.5.

**Mode logic.** A finite state machine selects which controller is active. Modes typically include: detumble, sun-pointing, science, slew, comms, safe. Mode transitions triggered by events or commands.

### 5.5 Failure injection

Built-in failure modes for testing:
- Sensor: dropout, bias step, scale factor error, noise level change, stuck-at value
- Effector: stuck open/closed, partial loss of thrust, pointing error, complete failure
- Computation: missed deadline (controller skips a tick), command latency change

Injected via the scenario file or interactively.

## 6. Catalog and conjunction analysis

### 6.1 Catalog ingestion

TLEs from Space-Track or CelesTrak, refreshed daily. Each TLE becomes a `CatalogObject` entity with `OrbitalState`, `TLEData`, `SGP4State`, and (if uploaded by JSpOC) `Covariance`.

**TLE format** per CelesTrak / Spacetrack Report No. 3: classic 80-column 2-line format with implicit-decimal-point fields, modulo-10 checksum. Apsis logs but does not fail on bad-checksum lines (operational catalogs occasionally contain them; behavior is configurable).

**SGP4 propagation** uses **WGS-72** fundamental constants per STR#3 to match operational AFSPC TLE-producing predictions (REQ-INT-005). Output frame is **TEME** (True Equator Mean Equinox); conversion to GCRS / ITRS uses the Vallado et al. 2006 §VI recipe.

### 6.1.1 CCSDS CDM ingest

Conjunction Data Messages per CCSDS 508.0-B-1 Blue Book are ingested in both KVN (key-value notation, default) and XML serializations. Each CDM provides:
- TCA, miss distance, originator-computed Pc (retained for cross-reference, not authoritative).
- Per-object state at TCA, 6×6 covariance in RTN frame, drag and SRP coefficients, maneuverability flag.

CDMs do **not** carry **Hard-Body Radius (HBR)**. Apsis maintains a per-spacecraft HBR config (REQ-CAT-013) and combines as `HBR = HBR_primary + HBR_secondary` for Pc computation. Apsis-computed Pc supersedes the originator-provided value; both are emitted in the conjunction event for traceability.

### 6.2 SGP4 propagation

Vallado's reference SGP4 implementation, integrated as a system over `CatalogObject` entities. Hot loop, SoA layout, optional SIMD batching. Per-object cost ~10 µs; 50,000-object catalog propagates in ~500 ms per epoch.

### 6.3 Spatial indexing

A spatial hash maintained per epoch. Each object's position is hashed into a 3D voxel (typical voxel size 100 km). Pair-checking iterates only over same-voxel and neighbor-voxel pairs.

For longer-horizon screening, **orbit-based pre-filters** apply first:
- Apogee/perigee filter — if A's apogee < B's perigee or vice versa, skip
- Inclination filter — for shared central body, large inclination differences imply small mutual altitude window
- These filters typically eliminate 99%+ of pairs before any geometric check

### 6.4 Conjunction refinement

For candidate pairs (those passing pre-filters and spatial hashing):

1. **TCA bracketing.** Find the time interval containing the closest approach using coarse-grained relative position evaluation.
2. **TCA localization.** Brent's method on `d/dt |r_A - r_B|² = 0` over the bracketed interval.
3. **Miss distance and relative velocity.** Evaluated at TCA.
4. **Joint covariance roll-forward.** Per-object 6×6 covariance is propagated from CDM epoch to TCA via the **Variational Equations integrator** (§3.6, [[wiki/concepts/variational-equations]]) — `P(TCA) = Φ · P(t_epoch) · Φᵀ` for each object, then summed into the joint covariance at TCA.
5. **Probability of collision (Pc).** Computed from miss distance, relative velocity, and combined covariance using Foster's, Akella-Alfriend's, or Chan's method.

### 6.5 Avoidance maneuver planning

When `Pc` exceeds a threshold (default 10⁻⁵), the system flags the conjunction. For active spacecraft, Apsis's automated CAM planner (REQ-CAT-011) computes the **optimal impulsive Δv** per Bombardelli & Hernando-Ayuso (2015):

```
maximize  Δvᵀ A Δv     subject to  |Δv| ≤ Δv_max
```

with `A = Mᵀ Q M` (max-miss-distance) or `A = Mᵀ Q* M` (min-Pc), where `M` is the linear b-plane-displacement-per-Δv matrix and `Q*` is the inverse b-plane covariance. The optimal direction is the eigenvector of `A` corresponding to the largest eigenvalue — generally **not** along-track. Along-track-restricted CAM is available as a constrained-mode option (when operational policy mandates a fixed maneuver direction).

CAM is typically planned 1-3 orbits before TCA. The simulator re-evaluates the conjunction after maneuver insertion. For finite-thrust CAMs (where the impulsive assumption breaks down) or multi-burn sequences, Apsis falls back to numerical optimization (constrained NMPC, REQ-GNC-008).

## 7. Monte Carlo

### 7.1 Trial structure

A trial is a single self-contained simulation:
- Initial state (perturbed from nominal)
- Configured spacecraft (URDF + GNC)
- Force model
- Mission timeline
- Termination condition (time, event, divergence)
- Recording configuration

### 7.2 Determinism

- Each trial has a master RNG seeded from `(campaign_seed, trial_index)`
- All stochastic sources draw from per-component RNGs spawned from the master
- No global RNG state. No hash-map iteration order dependencies. No within-trial parallelism.
- Same seed → same machine → bit-identical results

### 7.3 Snapshot/restore

The ECS registry is snapshotted after world setup (ephemerides loaded, spacecraft instantiated, force models configured). Each trial restores from snapshot, applies trial-specific perturbations, runs, and discards.

Snapshot/restore is fast — typically microseconds for a fully-loaded world — because the ECS data is contiguous and the snapshot is a memory copy.

### 7.4 Parallel execution

Trials run in worker processes (not threads — process isolation guarantees no shared state). A coordinator process distributes trial seeds to workers, collects results, writes to storage.

Local: Python `multiprocessing` with pre-loaded workers. Cluster: MPI or a job-queue system (Celery, Ray).

### 7.5 Result aggregation

Each trial produces a result record with:
- Trial index and seed
- Final state
- User-specified metrics (closest approach, max attitude error, fuel used, etc.)
- Time series of recorded variables (downsampled)
- Termination cause

Records are written to Parquet files (one per worker, merged at end of campaign). Analysis is done in Python with pandas / numpy / matplotlib.

## 8. Time acceleration

### 8.1 Acceleration regimes

| Regime | Mechanism | Typical max factor |
|---|---|---|
| Cruise (no perturbations, no GNC) | Analytical Keplerian | 10⁶+ |
| Cruise (full force model, no GNC) | Adaptive integrator with large steps | 10³-10⁴ |
| Active GNC, low rates | Event-driven scheduling | 10²-10³ |
| Active GNC, high rates (50-100 Hz) | Limited by GNC compute cost | 10-100 |
| Multi-body with Pinocchio at full rate | Limited by MBD cost | 1-100 |
| Catalog-only (50k objects, SGP4) | SoA SGP4 loop | 10⁴+ effective |
| Hardware-in-the-loop | Locked to wall clock | 1× exactly |

### 8.2 Event-driven stepping

The integrator advances to the next scheduled event:
- Next GNC tick (highest-rate scheduled component)
- Next sensor sample
- Next scheduled maneuver
- Next geometric event (eclipse, periapsis, SOI crossing)
- Next user-defined trigger

Between events, the integrator runs as freely as adaptive stepping permits. During cruise with no GNC and no near-term events, this can mean a single integration step of hours.

### 8.3 Fidelity downshift during warp

Optional: when warp factor exceeds a threshold and dynamics permit, the simulator downshifts to lower-fidelity modes:
- High-fidelity → analytical Keplerian (for true coast phases)
- Multi-body → rigid-body lumped (during attitude-stable cruise)
- High-order geopotential → low-order

Re-engages full fidelity when warp ends or dynamics demand it (atmospheric entry, maneuver imminent, etc.).

This is the same pattern KSP uses for its time warp, but explicit and configurable rather than hard-coded.

### 8.4 Real-time pacing

A separate `Pacer` component, optional. When attached, it throttles the simulator to wall-clock real time × user multiplier. When not attached, the simulator runs as fast as the CPU allows.

For HIL mode, the pacer locks to 1× and synchronizes with the external hardware's clock.
