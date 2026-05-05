---
type: source
title: "Implementation of Gauss-Jackson Integration for Orbit Propagation"
raw_path: docs/raw/papers/berry-healy-2004-gauss-jackson.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Berry, Matthew M.; Healy, Liam M.]
publication_date: 2004-09
venue: "Journal of the Astronautical Sciences 52(3):331-357"
based_on: "AAS 01-426, AAS/AIAA Astrodynamics Specialist Conference, Quebec City, Jul-Aug 2001"
---

# Berry & Healy (2004) — Implementation of Gauss-Jackson Integration for Orbit Propagation

The reference paper for actually implementing eighth-order Gauss-Jackson, as used by the U.S. space surveillance community since the 1960s (the SpecialK / ASW / AOES family of catalog propagators). Derives Adams, summed-Adams, Störmer-Cowell, and Gauss-Jackson from a single backward-difference operator algebra (§"Multi-Step Integrators" through §"Gauss-Jackson"), tabulates the eighth-order ordinate-form coefficients as exact rationals, lays out the startup procedure and predictor-corrector iteration patterns, and reports stability/accuracy benchmarks across step size and order on circular (ISS) and eccentric (CRRES) test orbits.

## What it provides for an implementation

**The eighth-order coefficients in ordinate form.** Table 5 (eighth-order summed-Adams coefficients b_jk) and Table 6 (eighth-order Gauss-Jackson coefficients a_jk), as exact rationals with denominators up to ~10^8. These are the canonical numerical reference; any GJ8 implementation evaluates `r_n = h² (S_n + Σ_{k=-4..4} a_jk r̈_{n+k-4})` and `ṙ_n = h (s_n + Σ a_jk r̈_…)` with these coefficients. Difference-form derivations (Eqs 50-53) are equivalent and easier to extend across orders, but for fixed-order GJ8 the ordinate form is simpler to program.

**Startup procedure (§"Procedure" → "Startup").** Build 9 initial points symmetric about epoch (j = -4 to +4) from a 5th-order Taylor expansion of the two-body solution (the [[concepts/f-and-g-series|f and g series]]; alternatively a Runge-Kutta single-step integrator). Iteratively refine via mid-corrector formulas (j = -4 to j = 3) and corrector (j = 4) until the eight non-epoch accelerations converge. The pattern is denoted SECECE…CE: S = startup estimate, E = evaluate, C = correct.

**Predictor-corrector iteration patterns.** PEC, PECE, P(EC)^n. The corrector may be applied once (PEC), with one final evaluate (PECE), or iterated until convergence on a tolerance. **Predictor-only mode (Herrick)** is also viable — halves the compute cost since one acceleration evaluation per step instead of two. For circular LEO (e ≈ 0) predictor-only is essentially as accurate as predictor-corrector at the same step size; for eccentric orbits the corrector helps stability (§"Effects of Step Size and Order on Accuracy and Stability", Tables 10, 11).

**Mid-corrector formulas.** Coefficients for j = -N/2 .. N/2 - 1 are computed by recursive backward-differences of the corrector coefficients (Eq. 54, 59, 63). Pre-tabulating the entire (N+1)(N+2) coefficient array — for N=8, that's 90 entries — eliminates per-startup recomputation.

## "Step size matters more than the order of the method"

The paper's §"Effects of Step Size and Order on Accuracy and Stability" benchmark on ISS (e=0.001, T=92 min) and CRRES (e=0.716, T=607 min) with a 24×24 WGS-84 + Jacchia-70 + lunar/solar force model is the load-bearing empirical finding (p. 356, Summary, direct quote): *"step size has more of an effect on accuracy than the order of the method does, and that higher orders are subject to instability at large step sizes."*

From Tables 8, 9 (ISS, CRRES; predictor-corrector):

| Step (s) | GJ6 ISS error ratio | GJ8 | GJ10 | GJ12 | GJ14 |
|---|---|---|---|---|---|
| 30  | 1.0e-11 | 1.5e-12 | 3.8e-13 | 1.5e-15 | — |
| 60  | 2.4e-9  | 1.5e-9  | 1.1e-9  | 9.7e-10 | 9.0e-10 |
| 120 | 9.7e-8  | 1.1e-7  | 1.1e-7  | 8.8e-8  | 1.1e-7 |
| 240 | 1.1e-4  | 1.3e-4  | 1.2e-4  | **\*** unstable | **\*** unstable |

Asterisks denote integration instability (eccentricity grows hyperbolic). The implication is counterintuitive: at large step sizes a *lower-order* method is preferable for stability, and at small step sizes higher orders dramatically improve accuracy. **GJ8 is the practical workhorse precisely because it tolerates step sizes up to ~120s on a typical LEO without going unstable, while still giving 10⁻⁷-10⁻⁸ position error ratio.** This is why the U.S. space surveillance community standardized on eighth-order specifically rather than chasing higher orders. See [[concepts/gauss-jackson-integration]] for the design implication.

## Discontinuities require restart

GJ requires `f(t, y)` to be continuous and smooth across the backpoints in the difference table (§"Multi-Step Integrators", paragraph 4). Solar radiation pressure across an eclipse boundary, attitude-mode-driven thrust toggles, and similar discontinuities break the Taylor-series assumption. Two recovery options:

- **Restart the multi-step.** Re-run the SECECE…CE startup at the new initial point. Fast if the startup is implemented well (Apsis's [[concepts/f-and-g-series|f and g series]] startup is cheap).
- **Encke-type correction.** Cited as Woodburn 2001 (AAS 01-223) for eclipse boundaries specifically — "Mitigation of the Effects of Eclipse Boundary Crossings on the Numerical Integration of Orbit Trajectories Using an Encke Type Correction Algorithm". Not yet ingested.

Apsis's event-driven scheduling (REQ-INT-009/010 and architecture §3 Orchestration) makes this natural: eclipse-entry/exit events trigger GJ restart at the boundary, and the integrator continues with a fresh backpoint set on the post-event side.

## Heritage

Originated in the U.S. space surveillance community in the 1960s as part of the AOES (Advanced Orbit/Estimation Subsystem). Two production lineages descended from it:

- **SpecialK** — used by Naval Network and Space Operations Command for special-perturbations catalog maintenance.
- **ASW** (Astrodynamics Support Workstation) — used at Cheyenne Mountain for the special-perturbations catalog and for conjunction assessment supporting NASA.

The paper analyzes SpecialK specifically (§Introduction) but explicitly notes its analysis applies to all GJ implementations including ASW. Method named "Gauss-Jackson" because of its appearance in a 1924 paper by Jackson (Monthly Notices RAS 84:602-606), per Herrick 1972 ([[sources/berry-healy-2004-gauss-jackson]] reference [10]).

## Reproducibility

Code that computes Gauss-Jackson coefficients to any order is at `http://hdl.handle.net/1903/2202` (UMD institutional repository — the same archive that hosted this paper as a free PDF). Apsis's GJ8 implementation can use Tables 5-6 directly or compute from this code as an oracle for verification.

## Apsis relevance

- **REQ-INT-002**: "The system SHALL provide Gauss-Jackson 8th-order fixed-step integration." This paper IS the implementation reference.
- **REQ-INT-006**: compensated summation. The second-sum operator ∇⁻² accumulates over many steps and benefits substantially from Kahan-Neumaier compensation — the long-arc behavior of GJ depends on this.
- **REQ-INT-009/010**: event detection at boundaries. The eclipse-restart pattern requires GJ to register as an event consumer.
- **REQ-INT-004**: analytical Keplerian propagation. Two-body f-and-g series is reused for GJ startup.
- **subsystems §3.1, §3.2**: lists GJ8 as the Earth-satellite workhorse; this paper supplies the algorithm.
- **architecture §3 Foundation > Dynamics core > Integrators**: GJ8 listed; no further implementation guidance — this paper fills that gap.

## Surfaced for human review (no silent spec edits)

Two implementation-detail observations that don't yet warrant ADRs but should be noted when a `gauss-jackson-integrator` component page is later authored:

1. **PEC vs PECE iteration mode** is a configuration choice that significantly affects compute cost. Predictor-only mode is also viable. Worth exposing as a parameter, not baking in.
2. **Eclipse-boundary restart** is the path-of-least-resistance vs Encke correction. The architecture's event-driven scheduling supports both; the choice is mostly about whether SRP-discontinuity handling lives in the integrator or in an external Encke wrapper.
