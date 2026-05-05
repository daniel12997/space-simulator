---
type: source
title: "The third realization of the International Celestial Reference Frame by very long baseline interferometry"
raw_path: docs/raw/papers/charlot-2020-icrf3.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Charlot, P.; Jacobs, C. S.; Gordon, D.; Lambert, S.; et al.]
publication_date: 2020-12
venue: "Astronomy & Astrophysics 644:A159"
doi: 10.1051/0004-6361/202038368
---

# Charlot et al. (2020) — ICRF3, the third realization of the ICRF by VLBI

The current operational realization of the **International Celestial Reference Frame (ICRF3)** — adopted by the IAU at its 30th General Assembly (August 2018) and replacing ICRF2 on January 1, 2019. ICRF3 comprises 4536 extragalactic sources observed by VLBI at 8.4 GHz (S/X), 24 GHz (K), and 32 GHz (Ka/X), with three-frequency positions for 600 sources. Aligned to the prior ICRS within 10 µas; individual source noise floor 0.03-0.06 mas at S/X; defining sources (303) uniformly distributed on the sky.

For Apsis: the GCRS (Geocentric Celestial Reference System) — the inertial root frame Apsis uses for orbital state per [[concepts/precession-nutation]], REQ-TIME-005, architecture §3 Foundation > Frames — is *realized* by ICRF3. SOFA's frame transformations to ICRS implicitly assume an ICRF realization; ICRF3 is the current one.

## What's new vs ICRF2

- **More sources, more bands**: 4536 vs ICRF2's 3414. K-band (24 GHz) and Ka/X (32 GHz) added to the traditional S/X (2.3/8.4 GHz). Three-frequency positions for ~600 sources.
- **Galactocentric acceleration** (Titov et al. 2011): for the first time, the ICRF3 reference epoch incorporates the solar system's acceleration toward the galactic centre (~5 µas/yr), which produces a detectable proper-motion field across the 40-year VLBI data span. Source coordinates in ICRF3 are referred to a specific epoch (2015.0); for high-precision applications ≥centuries from this, Galactic-acceleration proper motions must be applied.
- **Sub-mas noise floor**: 0.03 mas individual-source uncertainty for the best 500 sources.
- **Alignment**: ICRF3 axes consistent with ICRF2 at the 0.1 mas / 0.2 mas level (RA / Dec); consistent with Gaia-CRF2 within the ICRF3 noise level.

## Apsis relevance

- **REQ-TIME-005** (right-handed inertial frames including ICRF/J2000, GCRF) — ICRF3 is the operational realization of the ICRS that GCRF is referred to.
- **REQ-ENV-001** (ephemerides via SPICE SPK kernels DE440 or later) — DE440 is referred to ICRF3 (per [[sources/park-2021-de440-de441]] when ingested next).
- For Apsis's typical mission durations (<100 yr), **galactic-acceleration proper motion is negligible** for orbital dynamics (~5 µas/yr accumulates to 0.5 mas over 100 yr — millimetres at LEO, irrelevant for orbit propagation). Important only for VLBI-class astrometry, not for spacecraft sim.
- For interplanetary missions over decades-to-centuries, ICRF3's galactic-acceleration framework matters for inertial pointing budgets but not for trajectory accuracy.

## Cross-references

- [[concepts/precession-nutation]] — GCRS↔ITRS transformation chain that ICRF3 anchors at the celestial end.
- Will be re-cited from [[sources/park-2021-de440-de441]] (planetary ephemeris DE440 referred to ICRF3).
