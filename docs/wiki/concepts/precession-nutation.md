---
type: concept
canonical_name: "Precession-Nutation"
aliases: [precession, nutation]
created: 2026-05-04
updated: 2026-05-04
---

# Precession-Nutation

Umbrella concept for the time-varying orientation of Earth's celestial pole and equator with respect to an inertial reference frame. The transformation from the inertial GCRS to a frame whose z-axis is along Earth's rotation axis at epoch decomposes into:

- **Frame bias** ([[concepts/frame-bias]]) — the constant ~17 mas rotation between GCRS and the J2000 mean equatorial frame.
- **Precession** — the secular motion of the mean pole, described by polynomials in TT centuries.
- **Nutation** — the periodic motion of the true pole around the mean pole, described by a long Fourier-style series.

The composed rotation is the **bias-precession-nutation matrix**, which takes a vector from GCRS into the true-of-date frame whose z-axis is the [[concepts/celestial-intermediate-pole]] (CIP).

## The IAU 2006/2000A standard

Apsis's architecture (§3 Foundation > Frames) and subsystems (§1.3) specify "IAU 2006/2000A precession-nutation". This is the composition of two separately-published models:

| Component | Model | Reference |
|---|---|---|
| Precession | IAU 2006 (≡ P03) | [[sources/capitaine-2003-iau2000-precession-p03]] |
| Nutation   | IAU 2000A         | Mathews, Herring & Buffett (2002) — not yet ingested |

The architecture's "/" is a **product**, not an "/or": both apply together. See [[concepts/iau-2006-precession]] for the precession side specifically.

## Two equivalent transformation pipelines

The IAU framework offers two mathematically equivalent ways to rotate GCRS → terrestrial frame ([[sources/capitaine-2003-iau2000-precession-p03]] §2):

- **Equinox-based:** apply bias B, precession P (using ζ_A, z_A, θ_A), nutation N, then Greenwich Sidereal Time (GMST + equation of equinoxes).
- **CEO-based:** apply the X, Y coordinates of the CIP in GCRS (which absorb bias + precession + nutation polynomial parts), the position s of the [[concepts/celestial-ephemeris-origin]], and the [[concepts/earth-rotation-angle]] (a linear function of UT1).

Apsis's choice: see [[decisions/001-use-ceo-based-icrs-to-itrs]].

## SOFA delegation

Per the architecture (§3 Foundation), Apsis uses SOFA at frame-transformation boundaries rather than re-implementing the polynomial expansions. SOFA provides matched IAU 2006/2000A routines for both pipelines.

## Validity and precision

For Apsis's Earth-orbit work, the precession-nutation models are sub-microarcsecond at epoch and grow to milliarcsecond level over millennial timescales — well below the orbital-position precision target (millimetre at LEO per REQ-TIME-009) but load-bearing for any sub-arcsecond pointing budget.
