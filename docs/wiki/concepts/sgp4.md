---
type: concept
canonical_name: SGP4
aliases: [SGP, SGP4, SDP4, SGP4/SDP4]
---

# SGP4 — Simplified General Perturbations propagator

The general-perturbations analytical propagator that ingests **two-line element sets (TLEs)** from the US Space Force catalog and propagates them via a closed-form analytical solution rather than numerical integration. SGP4 is the *only* propagator that matches the model used to *produce* TLEs, so propagating TLEs with anything else introduces systematic error.

## Family

- **SGP** (Lane 1965) — original simplified general perturbations, periods < 225 min.
- **SGP4** (Lane & Cranford 1969; refined into [[sources/hoots-roehrich-1980-spacetrack-report-3|STR#3 1980]]) — current near-Earth analytical model. Uses [[sources/brouwer-1959-artificial-satellite-theory|Brouwer]] mean elements with [[sources/lyddane-1963-small-eccentricity-inclination|Lyddane's]] regularization for small e and small i.
- **SDP4** — deep-space extension for orbital periods ≥ 225 minutes; adds Sun + Moon long-period perturbations and resonance modeling for 12 h and 24 h orbits.
- Modern public implementations merge SGP4 + SDP4 into a single propagator that switches based on orbital period.

## Inputs

A **two-line element set (TLE)** containing:
- Epoch (year + day-of-year + fractional day)
- Mean motion `n`, eccentricity `e`, inclination `i`, RAAN `Ω`, argument of perigee `ω`, mean anomaly `M`
- Drag coefficient `B*` (units: 1/Earth-radii — empirically fit, not a physical drag coefficient)
- Mean motion derivatives (used in some variants)

Inputs are **Brouwer-Lyddane mean elements**, not Keplerian osculating elements. Converting an osculating state to TLE-style mean elements requires the inverse Brouwer-Lyddane transformation; converting back from a TLE for non-SGP4 propagation requires the forward transformation.

## Output frame

Position and velocity in the **True Equator Mean Equinox (TEME)** frame — an awkward intermediate frame using the equinox of date but no precession-of-equator, that does not match any of GCRS / ICRS / ITRS / TIRS / J2000. The standard pipeline is:

```
TEME → PEF (true-equator pseudo-Earth-fixed)
     → ITRF (apply polar motion)
     → GCRS (apply Earth rotation, nutation, precession, frame bias)
```

Vallado et al. 2006 §VI gives the canonical recipe.

## Apsis relevance

- **REQ-INT-005** (SGP4 propagation for TLE-defined objects) — Apsis must support SGP4 for any object provided as a TLE.
- **REQ-CAT-002** (50k-object catalog conjunction screening) — SGP4's analytical character makes it the only viable propagator at full-catalog scale; numerical integration of 50k objects is computationally infeasible for screening.
- **Subsystems §3** (analytical propagation as alternative to numerical) — for short-arc cruise of unperturbed-Keplerian-like objects, SGP4 (or its close relative the Brouwer mean-element propagator) is faster than numerical integration with no significant accuracy loss.

## Implementation in Apsis

Use the [[sources/vallado-2006-revisiting-spacetrack-3|Vallado et al. 2006 reference C/C++ code]] directly via the architecture's build-vs-reuse table. The Vallado test set is the conformance suite. For OD that produces TLEs (rare in Apsis but possible), see [[sources/vallado-2008-sgp4-orbit-determination]].

## See also

- [[sources/vallado-2006-revisiting-spacetrack-3]] — modern reference implementation.
- [[sources/brouwer-1959-artificial-satellite-theory]] + [[sources/lyddane-1963-small-eccentricity-inclination]] — underlying mean-element theory.
- [[sources/hoots-roehrich-1980-spacetrack-report-3]] (in `docs/raw/specs/`) — the original 1980 STR#3.
