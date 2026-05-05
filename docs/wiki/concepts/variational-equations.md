---
type: concept
canonical_name: Variational Equations
aliases: [variational equations, state-transition matrix propagation, force-model linearization, Φ propagation]
created: 2026-05-05
requirements: [REQ-PHY-016, REQ-PHY-020, REQ-INT-014, REQ-GNC-004, REQ-CAT-009]
sources: []
decisions: [002-variational-equations-between-measurements]
---

# Variational Equations

The mathematical and architectural framework that lets the orbit EKF and the Pc covariance pipeline propagate a **state-transition matrix Φ** alongside the spacecraft trajectory. This page is the canonical contract: every force model contributes its partials; the framework assembles them into the system Jacobian `A(t)`; a dedicated integrator propagates `Φ` between measurement / observation epochs.

## The math (briefly)

For an orbit state `x = [r; v] ∈ R⁶` evolving under `ẋ = f(x, t)` where `f = [v; a(r, v, t; p)]` and `p` is a vector of estimable parameters (drag coefficient, SRP coefficient, empirical-acceleration constants, …), the **first-order variational equation** is

```
dΦ/dt = A(t) · Φ,    Φ(t₀) = I
```

with

```
A(t) = [   0      I    ]
       [ ∂a/∂r  ∂a/∂v ]
```

For state augmented with `n_p` estimable parameters, A becomes `(6 + n_p) × (6 + n_p)` and includes `∂a/∂p` blocks. The state-transition matrix `Φ(t, t₀)` maps perturbations at `t₀` to perturbations at `t`:

```
δx(t) = Φ(t, t₀) · δx(t₀)
```

This is the linearised orbit dynamics — exactly what the EKF state-transition step and covariance roll-forward require.

The full development is in Vallado §10, Battin §9, and Montenbruck & Gill §7 — Apsis follows their conventions.

## Two layers

The Apsis Variational Equations subsystem has two distinct seams:

### Layer 1 — Force-model contribution

Each `ForceModel` provides three Jacobians per call site:

```cpp
class ForceModel {
public:
    virtual Vector3d acceleration(const State& s, const Time& t) const = 0;

    // Variational-equations contract:
    virtual Matrix3d partial_dadr(const State& s, const Time& t) const = 0;
    virtual Matrix3d partial_dadv(const State& s, const Time& t) const = 0;
    virtual std::vector<ParameterPartial> partial_dadp(const State& s, const Time& t) const {
        return {};   // default: no estimable parameters
    }

    // Estimable parameters declared statically at scenario setup:
    virtual std::vector<std::string> estimable_parameters() const { return {}; }

    // Conformance verification (see below):
    virtual std::vector<TestPoint> conformance_grid() const { return DEFAULT_GRID; }
    virtual double conformance_tolerance() const { return 1e-8; }

    // ... acceleration, name, is_conservative, etc.
};

struct ParameterPartial {
    std::string name;       // matches estimable_parameters() order
    Vector3d daudp;         // ∂a/∂p_i
};
```

`partial_dadp` returns partials w.r.t. each statically-declared estimable parameter (in the order returned by `estimable_parameters()`). The EKF / covariance propagator binds to declared names at scenario setup, so per-call overhead is just the partials evaluation.

#### Optional finite-difference fallback

Force models without analytical partials (e.g. table-lookup atmospheres, prototype empirical forces, user-defined perturbations) opt in to a centralised FD helper:

```cpp
class MyEmpiricalForce : public ForceModel {
public:
    Matrix3d partial_dadr(const State& s, const Time& t) const override {
        return FiniteDifferenceJacobian::dadr(*this, s, t);
    }
    // ... etc.
};
```

The helper handles step-size selection (relative perturbation per coordinate based on state magnitudes), symmetric difference for second-order accuracy, and is the same code the conformance test uses. **This is opt-in, not a base-class default** — every force-model author makes a deliberate choice between analytical and FD. The tradeoff (speed and conditioning vs derivation effort) shows up at the force-model declaration site, not silently inside the EKF loop.

### Layer 2 — Framework assembly and Φ propagation

A dedicated `VariationalEquationsIntegrator` module owns the assembly and propagation:

```cpp
class VariationalEquationsIntegrator {
public:
    // Propagate Φ from t0 to t1 given the natural state's dense output
    // and the per-spacecraft force-model list.
    MatrixXd propagate(
        const StateHistory& state_dense_output,  // from REQ-INT-008
        Time t0, Time t1,
        const std::vector<const ForceModel*>& forces
    );  // returns Φ(t1, t0); initialised internally as Φ(t0, t0) = I.
};
```

Internally it:

1. Initialises `Φ = I_{(6+n_p) × (6+n_p)}` at `t₀`.
2. Steps from `t₀` to `t₁` using **RK4** (default — Φ's ODE is linear, so the symplectic / GJ8 machinery used for the natural state is unnecessary).
3. At each step, queries the **state integrator's dense output** for `(r, v)`, evaluates each force's partials at that state, sums into `A(t)`, advances Φ.
4. Step size matches the state integrator's dense-output node spacing — partials are evaluated at the same `(r, v)` the natural state actually passed through.
5. Returns `Φ(t₁, t₀)`.

The module is Φ-on-demand: it is invoked by the EKF when it needs a propagation step between measurements, and by the Pc covariance pipeline when it rolls covariance from epoch to TCA. **Φ is never carried as part of the natural state** (see [[decisions/002-variational-equations-between-measurements|ADR-002]] for the rationale).

## Conformance harness

A generic test harness verifies that every registered ForceModel's analytical partials agree with finite differences over the model's declared `conformance_grid()`:

```cpp
TEST(ForceModelConformance, AllRegistered) {
    for (auto& force : registered_force_models()) {
        for (auto& tp : force.conformance_grid()) {
            auto analytical_r = force.partial_dadr(tp.state, tp.time);
            auto numerical_r  = FiniteDifferenceJacobian::dadr(force, tp.state, tp.time);
            EXPECT_LT(relative_error(analytical_r, numerical_r), force.conformance_tolerance());
            // ... same for partial_dadv, partial_dadp ...
        }
    }
}
```

The harness is **a CI gate**, not advisory — analytical-partial bugs are subtle, the test runs in seconds for the full force registry, and the cost of a missed bug compounds across every EKF / Pc result.

This is the operationalisation of *"the interface is the test surface"* from the architecture-skill discipline ([[../meta/wiki-system|wiki layer]]; [LANGUAGE.md](../../meta/wiki-conventions.md)) — the conformance test crosses the same seam as the EKF and the Pc covariance pipeline. One test contract, all force models covered automatically, including user-defined ones (REQ-EXT-001, REQ-EFF-010).

## Consumers

| Consumer | What it requests | Where |
|---|---|---|
| Orbit EKF | `Φ(t_{k+1}, t_k)` between measurement updates; parameter partials for state augmentation (e.g. Cd) | REQ-GNC-004; subsystems §5.3 |
| Pc covariance propagation | `Φ(TCA, t_epoch)` over screening windows up to 7 days (REQ-CAT-005) | REQ-CAT-009; subsystems §6.4 |
| Sensitivity analysis (future) | Φ over arbitrary intervals; sensitivities `∂x_final / ∂p` for parameter-tuning Monte Carlo | (out of scope v1) |

## Numerical conditioning

- **Φ growth.** Φ is unbounded for unstable orbital regimes (close conjunctions, near-resonance). Long Φ propagations (>>1 orbital period) can saturate `f64` exponent. Apsis logs Φ condition number after each propagation; if `cond(Φ) > 1e10`, emit a diagnostic — usually a sign that the propagation interval is too long for the orbit's Lyapunov characteristics.
- **Partial-evaluation conditioning.** `∂a/∂r` for the central-body point-mass term contains `1/r³` and `1/r⁵` factors. At LEO altitudes these are well-conditioned (`r ≈ 7×10⁶ m`); for parabolic / hyperbolic close approaches near a small body, conditioning degrades. Force models in those regimes should declare a tighter `conformance_tolerance()` to catch implementation bugs early.
- **FD step size.** The FD helper picks `h = max(|x_i| · √ε_machine, h_min)` per coordinate, where `h_min ≈ 1 m` for position and `1 mm/s` for velocity. Force models with unusual scales (e.g. relativistic corrections at micro-acceleration levels) override `conformance_tolerance()` to avoid spurious conformance failures.

## See also

- [[decisions/002-variational-equations-between-measurements]] — why Φ is propagated between measurements rather than as augmented state.
- [[concepts/kalman-filter]] — the orbit EKF is a primary consumer.
- [[concepts/floating-base-dynamics]] — the orbit-state subset of the floating-base configuration; Apsis's variational equations are over orbit state only at v1, not the joint configuration.
- [[sources/foster-estes-1992-jsc-25898-pc]] — Pc method whose covariance roll-forward consumes Φ.
- [[sources/iers-conventions-2010]] — IERS-conformant force models all expose the variational-equations contract.
- Vallado §10, Battin §9, Montenbruck & Gill §7 — textbook treatments (not in corpus; standard astrodynamics references).
