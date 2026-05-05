---
type: decision
title: "Hand-rolled DOP853, Yoshida-4, and Gauss-Jackson 8 integrator family behind one IIntegrator seam"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: [berry-healy-2004-gauss-jackson]
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Phase 1 architectural decision.

## Context

The propagator core needs an `IIntegrator` seam that supports the
state-plus-Φ step contract from `docs/structure.md` Phase 1:

```
IIntegrator::step(state, Φ, dt) -> (state', Φ')
```

Three regimes must be covered, per the design overview's emphasis on
[[concepts/long-arc-state-conditioning|long-arc conditioning]]:

- **Adaptive RK** for general non-stiff orbital ODEs and
  [[concepts/variational-equations|variational-equations]] propagation
  between measurements ([[decisions/002-variational-equations-between-measurements]]).
- **Symplectic** for gravity-only long arcs where energy preservation
  matters more than per-step accuracy.
- **Multi-step** ([[concepts/gauss-jackson-integration|Gauss-Jackson 8]])
  for second-order orbital ODEs in the operationally common case where
  step size is fixed and the force evaluation is the cost driver.

The design overview commits to "structure-preserving integrators" as
part of the long-arc precision strategy; the seam must accommodate all
three families uniformly.

## Decision

Implement three integrators **hand-rolled in `src/integrate/`**, all
behind a common `IIntegrator` interface and covered by a single
parameterised conformance test:

- **`Dop853`** — Dormand-Prince 8(5,3) adaptive Runge-Kutta. Coefficients
  from Hairer-Nørsett-Wanner Vol. I Table 5.2. Adaptive stepsize via the
  embedded 5th-order solution. ~600 LOC.
- **`Yoshida4`** — fourth-order symplectic composition of the velocity-Verlet
  flow (Yoshida 1990). Used for gravity-only long-arc cases where energy
  drift matters. ~150 LOC.
- **`GaussJackson8`** — eighth-order multi-step second-sum integrator per
  [[sources/berry-healy-2004-gauss-jackson]]. Uses Dop853 for the
  starter. ~400 LOC including starter wiring.

All three step `(state, Φ, dt) -> (state', Φ')` by augmenting the state
vector with the columns of Φ and integrating in lockstep with the
underlying physical state. The Φ derivative is `∂a/∂x · Φ`, requiring
the `IForceModel::partials_dadx` from the VE contract.

## Rationale

- The augmented `(state, Φ)` step contract is the natural form for the
  VE roll-forward mandated by ADR-002. Boost.Odeint exposes its
  observer-pattern step interface around a single state vector, which
  forces us to either (a) flatten Φ into the state every step (loses
  type clarity, awkward sizes), or (b) write a custom Odeint stepper
  that replicates DOP853 anyway. The latter is no smaller than a
  hand-rolled DOP853.
- DOP853 coefficients are textbook and stable for decades. Implementing
  them is a transcription task with property-based tests; not a
  research project.
- GJ8 is not provided by Boost.Odeint or SUNDIALS in a form we can
  directly reuse. Berry-Healy 2004 gives a complete algorithmic
  description suitable for direct implementation.
- A hand-rolled family keeps the codebase under our control — debugging
  numerical behaviour for a 50-year arc means owning the integrator
  internals, not chasing them across a third-party API.
- Compile times stay low (no Boost transitive include explosion).

## Alternatives considered

- **Boost.Odeint adapter family.** Rejected per the contract-fit
  argument above. Also drags Boost into the dependency tree at Phase 1,
  which is otherwise avoidable.
- **SUNDIALS CVODE.** Excellent for stiff problems; overkill for
  non-stiff orbital ODEs; doesn't help with VE contract. Heavy build
  weight.
- **Pin one integrator (e.g. just DOP853).** Rejected because the design
  overview commits to symplectic for gravity-only long arcs and to
  GJ8-class multi-step for the operational fixed-step regime; one
  integrator can't cover all three regimes well.
- **Use an autograd / templated integrator that derives ∂a/∂x via
  forward-mode AD on the force model.** Considered; would let us drop
  the explicit `partials_dadx` requirement on `IForceModel`. Rejected
  because (a) AD across SPICE-backed third-body terms is intractable
  without rewriting the SPICE seam, (b) analytical partials are well-known
  for our force models and faster than AD.

## Consequences

- `IForceModel` must expose `partials_dadx` as a hard contract member,
  not optional. Adapters that lack analytical partials (none currently
  planned) would have to provide a finite-difference fallback — flagged
  but not implemented in Phase 1.
- The conformance test parameterised over `{Dop853, Yoshida4,
  GaussJackson8}` runs on a Kepler problem and checks: (a) solution
  matches f-and-g-series Keplerian closed-form to integrator tolerance;
  (b) Φ propagated by the integrator agrees with `∂x(t)/∂x(0)` derived
  via central-difference perturbation of initial conditions to the same
  tolerance. This is the *only* mechanism by which a new integrator
  adapter is admitted to the seam.
- Coefficient tables are committed as `constexpr std::array` in headers
  and hashed in CI to prevent silent corruption.
- Encke-wrapper composition ([[concepts/long-arc-state-conditioning]]) is
  a separate module that takes any `IIntegrator` and reframes it as a
  deviation propagator.

## Phase 1 Implementation Note (2026-05-05)

Phase 1 lands the IIntegrator seam, the Φ augmentation, the Encke
wrapper, and the conformance tests as committed above, but ships a
narrower adapter set than the steady-state vision:

- The adaptive RK adapter that ships in Phase 1 is **`Dp54`** —
  Dormand-Prince 5(4), Hairer Vol I Table 5.1. The full DOP853
  (Hairer Vol I Table 5.2) is deferred to Phase 7. The seam, the
  PI step controller, the Φ augmentation, and the conformance gate
  are unchanged at the upgrade; only the coefficient table and the
  per-step error weights move. The Phase 1 conformance tolerances
  are widened accordingly (Kepler closure < 1 m / 1 period at rtol
  1e-12 instead of the < 1e-7 m the plan originally targeted; the
  upgrade rebuilds those by ~7 orders of magnitude).
  Naming the class `Dp54` (rather than re-using `Dop853`) is
  intentional: the previous `Dop853` alias over a DP5(4) coefficient
  table was a load-bearing lie — a future maintainer would have read
  "Dop853" and assumed Hairer Table 5.2 fidelity.

- The Berry-Healy 2004 ordinate-form Gauss-Jackson 8 implementation
  is **deferred to Phase 7**. The `GaussJackson8` adapter and its
  conformance test were removed in Phase 1 because the seam-only
  stand-in (a single Dp54 step under the GJ8 name) carried zero
  distinct behaviour and the conformance test under both names was
  the same integrator twice. Phase 7 reintroduces the type behind
  `IIntegrator` with the second-sum starter and the ordinate-form
  predictor-corrector, at which point GJ8 rejoins the parameterised
  conformance gate.

- Analytical partials for `SphericalHarmonic` (the Pines gradient)
  are deferred to Phase 7. The Phase 1 SH adapter ships with a
  central-difference `partials()` and is **excluded** from the VE-
  contract conformance test parameterisation (the test runs on
  `{PointMass, ThirdBody}` only). `PointMass` and `ThirdBody`
  partials are analytical in Phase 1 as ADR-009 requires. Each
  adapter carries a `static constexpr bool kAnalyticalPartials`;
  the conformance test loops only over those that claim `true`,
  and a `static_assert` on `SphericalHarmonic::kAnalyticalPartials
  == false` in the conformance source guards the disclosure.

- The Phase 1 `ThirdBody` adapter's `acceleration()` was changed in
  service of fix-set landing this note: it now returns the
  conventional Vallado §8.7.2 / Montenbruck-Gill §3.3.2 form
  `mu_3 * ((r_3 - r) / |r_3 - r|^3 - r_3 / |r_3|^3)` rather than the
  Battin / f(q) stable substitution that was previously wired (the
  prior form had a sign in the f-term that disagreed with the
  conventional expression by about 2x in the LEO-vs-Sun regime; both
  forms passed the order-of-magnitude unit test, but only the
  conventional form lines up with the published analytical Jacobian).
  REQ-PHY-005's "must be numerically stable at small spacecraft-
  central-body distances" is unaffected at LEO (the cancellation
  costs ~5 of ~16 sig figs, well above the 1e-6 conformance tolerance);
  the Battin substitution returns as a Phase 7 hardening item if
  conjunction-screening close approaches require it.

The IIntegrator and IForceModel seams as documented above are
unchanged at the Phase 7 upgrades; these notes describe the fidelity
of what is wired behind the seams, not changes to the seams
themselves.
