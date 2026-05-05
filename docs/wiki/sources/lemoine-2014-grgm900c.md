---
type: source
title: "GRGM900C: A degree 900 lunar gravity model from GRAIL primary and extended mission data"
raw_path: docs/raw/papers/lemoine-2014-grgm900c.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Lemoine, Frank G.; Goossens, Sander; Sabaka, Terence J.; Nicholas, Joseph B.; Mazarico, Erwan; Rowlands, David D.; Loomis, Bryant D.; Chinn, Douglas S.; Neumann, Gregory A.; Smith, David E.; Zuber, Maria T.]
publication_date: 2014-05
venue: "Geophysical Research Letters 41:3382-3389"
doi: 10.1002/2014GL060027
---

# Lemoine et al. (2014) — GRGM900C lunar gravity model

The NASA GSFC GRGM900C lunar gravity model — degree-and-order 900 [[concepts/spherical-harmonic-geopotential|spherical-harmonic geopotential]] derived from GRAIL Primary + Extended Mission tracking data. Reference radius 1738 km; based on the [[sources/folkner-2009-de421|DE421]] lunar and planetary ephemerides. Complete to degree 900 = 6 km spatial resolution; degree strength 575-675 over central nearside/farside; up to 900 over polar regions.

For Apsis: the canonical lunar gravity model when satisfying REQ-PHY-003 ("spherical-harmonic gravity to user-selectable degree and order") for lunar-orbiting spacecraft. Subsystems §2.2 names "GRGM1200A to 165×165 for low orbits"; GRGM900C is the predecessor, with 900×900 instead of 1200×1200, but otherwise the same methodology and provider.

## Modeling notes (§3)

- **Force model**: standard spherical-harmonic series Eq. 1, with degree-2 Love number `k₂` estimated.
- **Solver**: NASA GSFC GEODYN II, fixed-step Cowell integrator at 0.5 s step size, scaled to support degree-900 sampling at GRAIL's 1.7 km/s orbital velocity.
- **Kaula constraint** of `3.6e-4/ℓ²` applied for ℓ > 600 to stabilize the high-degree solution.
- **Calibration**: a posteriori covariance scaled to match observed residual variance.

## Apsis relevance

- **REQ-PHY-003**: lunar gravity model for lunar-orbit propagation. Use 165×165 (subsystems §2.2 default) for low lunar orbit; truncate to 50×50 for higher altitudes; Apsis loads coefficients from the ICGEM hosting service or NASA GSFC repository.
- **REQ-MC-004**: Monte Carlo trials over GM and lunar-gravity uncertainties — covariance reported in this paper provides the σ values for sampling.
- A sister model **GL0660B** (JPL, Konopliv et al. 2013) exists at degree 660 and was developed independently; differs in orbit-determination software, a-priori models, and parameter-estimation strategy. Either is acceptable; GRGM family is more widely used in the GSFC tooling.

## Successor

GRGM900C was followed by **GRGM1200A** (degree 1200 — [[sources/lemoine-2014-grgm900c|this paper]]'s authors and team, methodology unchanged), which subsystems §2.2 names. Apsis should use GRGM1200A or later when available; methodology is unchanged from this paper.

## Cross-references

- [[concepts/spherical-harmonic-geopotential]] — the umbrella concept.
- [[sources/pavlis-2012-egm2008]] — the Earth analogue.
- [[sources/folkner-2009-de421]] — the lunar/planetary ephemeris underlying this model's solution.
