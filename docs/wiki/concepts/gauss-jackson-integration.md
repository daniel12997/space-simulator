---
type: concept
canonical_name: "Gauss-Jackson Integration"
aliases: [Gauss-Jackson, GJ8, second-sum method, GJ]
created: 2026-05-04
updated: 2026-05-04
---

# Gauss-Jackson Integration

A fixed-step **predictor-corrector multi-step integrator** for second-order ODEs of the form `r̈ = a(r, t)`, derived as the summed (double-integration) form of the Störmer-Cowell method, which is itself the second-order analogue of the Adams family. The "second sum" — the operator ∇⁻² applied to the difference table of accelerations — is what makes Gauss-Jackson distinct from its predecessors and why it is sometimes called the **second-sum method**.

In Apsis the canonical order is **8** (GJ8), per REQ-INT-002 and per the U.S. space surveillance community's standardisation since the 1960s (architecture §3 Foundation > Dynamics core > Integrators).

## Algorithm structure

A multi-step integrator advances from `t_n` to `t_{n+1}` using the function values at the current and several previous mesh points (the **backpoints**). For order N, the backpoint set has N+1 entries. The eighth-order GJ algorithm advances using the current point + 8 previous accelerations.

Each step is a **predictor-corrector pair**:

1. **Predictor** — computes `r_{n+1}` and `ṙ_{n+1}` from the existing 9-point backpoint history using the predictor coefficients (Tables 5, 6 of [[sources/berry-healy-2004-gauss-jackson]]).
2. **Evaluate** — calls the force model to compute `r̈_{n+1}` at the predicted position.
3. **Corrector** — refines `r_{n+1}` and `ṙ_{n+1}` using the now-known `r̈_{n+1}` plus the previous backpoint accelerations.

The corrector may be applied once (PEC), once-with-final-evaluate (PECE), or iterated to a tolerance (P(EC)^n). Iteration trades compute for accuracy. **Predictor-only mode (PE)** halves the per-step compute cost and is viable for circular orbits at the price of some stability for eccentric orbits ([[sources/berry-healy-2004-gauss-jackson]] §"Effects of Step Size and Order on Accuracy and Stability", Tables 10, 11).

## Startup

A multi-step method needs N+1 backpoints to begin, but the initial value problem provides only one point — the epoch. GJ8 startup builds the surrounding 8 points symmetrically (j = -4 to +4 about epoch) by:

1. Evaluating the [[concepts/f-and-g-series|two-body f and g series]] at the 8 surrounding epochs to seed initial position/velocity guesses.
2. Computing accelerations at all 9 points from the full force model.
3. Applying mid-corrector and corrector formulas iteratively (the SECECE…CE pattern) until the eight non-epoch accelerations converge.

Mid-corrector coefficients are derived recursively from the corrector coefficients ([[sources/berry-healy-2004-gauss-jackson]] Eqs 54, 59, 63). The full eighth-order coefficient array is (N+1)(N+2) = 90 numbers — small, pre-tabulated once, used at every startup.

Alternative startup: a single-step integrator (Runge-Kutta) for the first 8 points. Less elegant; required if a two-body solution isn't a good initial guess (e.g., near singular cases). The f-and-g approach dovetails with REQ-INT-004 (analytical Keplerian propagation) — Apsis already provides the primitive.

## Order vs step size — the counterintuitive bit

The naive expectation is "higher order is always better." [[sources/berry-healy-2004-gauss-jackson]] §"Effects of Step Size and Order on Accuracy and Stability" shows otherwise (Tables 8, 9):

- **At small step (30s on LEO)** higher orders are dramatically more accurate (GJ12 reaches 10⁻¹⁵ position error ratio vs 10⁻¹¹ for GJ6).
- **At large step (240s on LEO)** orders 12 and 14 are *unstable* — the eccentricity grows hyperbolically — while GJ8 remains stable with ~10⁻⁴ error ratio.
- The summary states it bluntly: "step size has more of an effect on accuracy than the order of the method does, and that higher orders are subject to instability at large step sizes" (p. 356).

**Implication for Apsis**: GJ8 is the right canonical order because it tolerates step sizes up to ~120s on a LEO satellite while remaining stable. Choosing GJ12 or GJ14 in pursuit of higher accuracy is counterproductive once step size grows beyond ~60s.

## Continuity requirement

GJ assumes `r̈(t)` is continuous and smooth across the entire backpoint window. Discontinuities — solar radiation pressure switching at an eclipse boundary, attitude-mode-driven thrust toggles, discrete maneuver impulses — break the Taylor-series assumption that underpins the difference operators ([[sources/berry-healy-2004-gauss-jackson]] §"Multi-Step Integrators").

Two practical recoveries:

- **Restart at the discontinuity.** Re-run the SECECE…CE startup with the post-event state as the new epoch. Fast in Apsis because the f-and-g startup is cheap. **This is the natural fit for Apsis's event-driven architecture (REQ-INT-009/010)** — eclipse-entry/exit events become GJ-restart triggers.
- **Encke-type correction across the boundary** (Woodburn 2001, AAS 01-223 — not yet ingested). Avoids the restart cost at the price of more complex bookkeeping around the event.

## Compensated summation

The ∇⁻² second-sum operator accumulates a running sum of accelerations over potentially many thousands of steps. Naive summation drops about 1 ULP per step into round-off; over a 1-year propagation at 60s steps that's ~5×10⁵ ULPs of accumulated noise on the position state. Kahan-Neumaier compensated summation (REQ-INT-006) recovers most of this — essentially free per step, transformative on long arcs.

## Implementation references

- [[sources/berry-healy-2004-gauss-jackson]] — derivation, eighth-order ordinate-form coefficient tables, startup procedure, accuracy/stability analysis, predictor-only variant.
- Vallado "Fundamentals of Astrodynamics and Applications" Ch. 8 (cited in [[sources/berry-healy-2004-gauss-jackson]] reference [14]) — textbook treatment with worked examples. INDEX-only paywalled.
- Coefficient-generating code at `http://hdl.handle.net/1903/2202` (UMD repository) — generates GJ coefficients to any order; useful as a verification oracle for Apsis's hard-coded GJ8 tables.

## See also

- [[decisions/009-hand-rolled-integrator-family]] — Apsis's GJ8 sits alongside DOP853 and Yoshida-4 behind one `IIntegrator` seam; this ADR is the canonical implementation decision for that family.
