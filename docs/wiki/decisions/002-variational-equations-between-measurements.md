---
type: decision
title: "Variational equations propagated between measurements, not as augmented state"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: []
components: []
requirements: [REQ-INT-014, REQ-PHY-016, REQ-GNC-004, REQ-CAT-009]
---

## Status

**Accepted** 2026-05-05. Surfaced during the architecture-review grilling on the *Variational Equations* deepening candidate (see [[synthesis/audit-summary-2026-05-05]] cluster F2.1 → deepening candidate #2). Cited from [[concepts/variational-equations]] as the load-bearing rationale for the framework's Φ-propagation shape.

## Context

The state-transition matrix `Φ(t, t₀)` is required by:

- **Orbit EKF** (REQ-GNC-004) for the predict step between measurement updates.
- **Pc covariance propagation** (REQ-CAT-009) for rolling joint covariance from epoch to TCA over screening windows up to 7 days (REQ-CAT-005).

Three placements were considered for *where Φ lives in the simulator's data flow*:

1. **Augmented natural state.** Add `vec(Φ)` to the integrator's state vector — natural state grows from `R⁶` to `R⁴²` (or larger with parameter augmentation). One integrator, one step controller.
2. **Parallel continuous integration.** Φ propagates continuously alongside the natural state; runs in lockstep, querying the natural-state integrator's dense output for `(r, v)` at each Φ step.
3. **Between-measurement propagation atop parallel mechanism.** Φ is integrated only when an EKF update or Pc covariance roll-forward needs it. Each invocation initialises `Φ(t_k, t_k) = I` and propagates to the next requested epoch via the parallel mechanism of (2).

## Decision

Apsis SHALL use **Option 3** — Φ is propagated **between measurements** atop the parallel-integrator mechanism described in (2). Φ is **never** part of the natural-state vector that the orbit propagator integrates.

Concretely:
- Natural-state integrator (REQ-INT-001/002/003) integrates `[r, v]` (or the floating-base configuration) without modification.
- The Variational Equations module ([[concepts/variational-equations]]) exposes `propagate(state_dense_output, [t₀, t₁], force_list) → Φ(t₁, t₀)`.
- Each invocation initialises `Φ(t₀, t₀) = I` internally; callers don't carry Φ between calls.
- Φ uses **RK4** as the default integrator for `dΦ/dt = A·Φ` (linear ODE; symplectic / GJ8 machinery for the natural state is unnecessary here).
- Φ-step size matches the natural-state integrator's dense-output node spacing.

## Rationale

- **No coupling between natural-state and Φ accuracy needs.** With augmented state (Option 1), the adaptive step controller responds to the larger of (natural-state error norm, Φ error norm). Φ tends to grow over time (especially in unstable orbital regimes), forcing step shrinkage that has nothing to do with the natural state's accuracy needs. Decoupling step controllers eliminates this failure mode.
- **No bloated state vector.** R⁴² or larger augmented states force every operation that touches "the state" — checkpoint / restore (REQ-MC-005), telemetry recording (REQ-OBS-002), conservation invariants (REQ-OBS-004) — to either special-case Φ or carry it everywhere it isn't needed. The natural-state vector stays semantically clean (orbit + attitude + joints, nothing else).
- **EKF-cadence is the actual demand.** The Pc pipeline rolls covariance over windows defined by EKF/observation epochs, not over arbitrary fractional-second intervals. Φ is wanted at observation boundaries, not continuously. Computing it on demand at the boundaries matches the access pattern and avoids continuous unused work.
- **Initialisation is automatic.** With between-measurement propagation, `Φ(t_k, t_k) = I` is set inside the module at every invocation. Callers don't need to remember initialisation, store Φ between calls, or worry about "what's Φ's value mid-orbit?" — those questions have no architectural answer.
- **REQ-INT-008 dense output makes the parallel mechanism free.** The natural-state integrator already exposes dense output for event detection. Querying it at Φ-step nodes is a method call, not new infrastructure.
- **Pc covariance interval matches the consumption pattern.** Pc roll-forward over a 7-day window is a single `propagate(state, [t_epoch, TCA], forces)` call. No additional ledger-keeping for Φ between calls.

## Alternatives considered

**Option 1 — Augmented natural state.** Rejected because (a) bloats the state vector and forces every state-touching subsystem to special-case Φ; (b) couples Φ accuracy needs to natural-state step controller, leading to spurious step shrinkage; (c) makes "what's Φ at arbitrary t?" semantically meaningful when it shouldn't be — Φ is implicitly relative to a reference epoch, and embedding it in the natural state hides which epoch.

**Option 2 — Continuous parallel integration.** Rejected because (a) does redundant work between consumption epochs (Pc rolls covariance once per CDM, not continuously); (b) requires the framework to maintain Φ state between calls, raising "what's Φ relative to?" questions that Option 3 answers automatically; (c) doesn't simplify any consumer's API relative to Option 3.

**Hybrid: augmented state for the orbit EKF (which wants Φ continuously), between-measurement for Pc.** Rejected because two parallel mechanisms multiply test surface and create a class of bugs where the two paths disagree. One mechanism, two consumers.

## Consequences

- The **Variational Equations module is a first-class subsystem** with its own integrator, conformance harness, and concept page — not an EKF-internal helper. This invites contributions from sensitivity-analysis use cases (post-v1) without requiring EKF surgery.
- Φ-propagation **performance characteristics decouple** from natural-state propagation. Long Pc roll-forward windows can use larger Φ steps if conditioning permits; tight EKF measurement cadences keep Φ steps fine. Each consumer tunes independently.
- The natural-state integrator interface (subsystems §3.2) **does not change**. Existing integrators (DOPRI 8(7), GJ8, Yoshida 8) remain unmodified.
- Force models implement the variational-equations contract once (per [[concepts/variational-equations]]); both consumers benefit. New force models added later automatically participate.
- **Conformance discipline** (REQ-PHY-020) becomes feasible because force-model partials have a single canonical evaluation path through the framework, not multiple consumer-specific call sites.

## Open items if accepted

- Choose between **shared Φ across observers within one trial** (one integrator pass per epoch range, both EKF and Pc consume) vs **independent Φ per consumer** (each consumer requests its own Φ; possibly redundant but simpler ownership). Likely the former for efficiency once observation rates exceed ~1 Hz; defer until first benchmark.
- Define **Φ caching policy** for repeated requests over overlapping intervals (Pc may re-roll covariance several times during conjunction refinement). LRU cache keyed on `(t₀, t₁, force_set_hash)` is the obvious shape; defer until performance shows it matters.
- Standardise **Φ condition-number diagnostic** thresholds (Apsis logs `cond(Φ)`; current threshold `1e10` is a guess). Tighten or relax based on early Monte Carlo experience.
