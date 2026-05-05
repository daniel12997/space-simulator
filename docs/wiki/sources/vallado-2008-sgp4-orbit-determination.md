---
type: source
title: "SGP4 Orbit Determination"
raw_path: docs/raw/papers/vallado-2008-sgp4-orbit-determination.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Vallado, David A.; Crawford, Paul]
publication_date: 2008-08
venue: "AIAA/AAS Astrodynamics Specialist Conference, Honolulu — AIAA 2008-6770"
---

# Vallado & Crawford (2008) — SGP4 Orbit Determination

Companion to [[sources/vallado-2006-revisiting-spacetrack-3]] providing the **differential-correction (DC) orbit-determination code** that produces TLE data from externally derived ephemerides or raw observations. Closes a gap that has existed since STR#3: prior to this paper, no public code did the inverse problem of fitting [[concepts/sgp4|SGP4]] mean elements to a given trajectory.

## What it adds over Vallado 2006

The 2006 paper provides the SGP4 *propagator* (forward direction: TLE → state at time t). This paper provides the *fitter* (inverse direction: ephemeris/observations → TLE). Differential correction is multi-dimensional Newton-Raphson root-finding on `y = f(x)` with least-squares treatment of observed `y`.

## Algorithm 1: SGP4 Differential Correction

The procedure (Algorithm 1 in §II):

```
loop until converged:
    for each observation i:
        propagate nominal state X₀ to obs time using SGP4 → X_i in TEME
        transform X_i to observation frame
        b_i = obs_i - calculated_i             # residual
        form A_i (Jacobian: ∂obs/∂X₀)         # finite-diff or analytic
        accumulate AᵀWA and AᵀWb
    solve δx = (AᵀWA)⁻¹ AᵀWb (or via SVD)
    update X₀ ← X₀ + δx
covariance P = (AᵀWA)⁻¹
```

Two key choices:

- **Jacobian generation**: finite-differences (brute force, easy) vs analytical partials `A = H · Φ` (fast, but requires SGP4-state-transition-matrix derivation).
- **Linear solve**: Gauss-Jordan elimination on the normal equations vs Singular Value Decomposition (SVD) on the weighted Jacobian. SVD is more robust for near-singular cases.

## Apsis relevance

- **Not directly required** by REQ-INT-005 / REQ-CAT-002, which call only for SGP4 *propagation*. Apsis ingests TLEs, doesn't produce them.
- **REQ-CAT-005 (conjunction screening with covariance)**: this paper's covariance output `P = (AᵀWA)⁻¹` is exactly what conjunction analyses need but TLE-only catalog data lacks. If Apsis ever extends to *covariance-augmented* catalog propagation (currently only the SP catalog is covariance-bearing), this fitter is the path: re-fit each TLE against an external propagated ephemeris and capture the resulting `P`. Kelso (2007) showed this works.
- **Initial Orbit Determination interface (subsystems §3.4)**: Apsis's IOD path (Lambert, Gibbs, Herrick-Gibbs, Gooding) produces an initial state estimate that this paper's DC algorithm refines. Even if Apsis doesn't produce TLEs, the same DC algorithm with a numerical-propagator inner loop is the standard OD workflow.

## TLE fit-arc considerations

§II observes that fit-arc length depends on which forces are unmodeled or partially modeled by SGP4:

- Whole day for unmodeled / partially modeled forces (J₂.₂ tesserals, solar gravity).
- Several days for accurate atmospheric drag at higher altitudes.

This matters because Apsis can degrade SGP4 accuracy by feeding it TLEs fit over the wrong arc length, even with the canonical propagator.

## Cross-references

- [[sources/vallado-2006-revisiting-spacetrack-3]] — the propagator companion paper.
- [[concepts/sgp4]] — SGP4 family overview.
- [[sources/hoots-roehrich-1980-spacetrack-report-3]] (in `docs/raw/specs/`) — original STR#3.
