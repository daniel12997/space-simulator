---
type: concept
canonical_name: "Celestial Ephemeris Origin"
aliases: [CEO, CIO, NRO, non-rotating origin, Celestial Intermediate Origin]
created: 2026-05-04
updated: 2026-05-04
---

# Celestial Ephemeris Origin (CEO / CIO)

The origin of right ascension on the [[concepts/celestial-intermediate-pole|CIP]]'s instantaneous equator, defined as the **non-rotating origin (NRO)** of Guinot (1979) and recommended by IAU 2000 Resolution B1.8.

## Why it exists

Defined kinematically: from one moment to the next, the CEO moves at right angles to the instantaneous equator, with no rotation about the CIP itself. This makes the relationship between Earth's rotation angle and Universal Time **simple and linear**, eliminating the equation-of-equinoxes complications inherent to the equinox-based formulation ([[sources/capitaine-2003-iau2000-precession-p03]] §2).

Earth Rotation Angle (ERA) — see [[concepts/earth-rotation-angle]] — is the angle from the CEO to the Terrestrial Intermediate Origin (TIO) measured along the CIP equator.

## Naming

The same point goes by several names:

- **NRO** (non-rotating origin) — Guinot's original 1979 term
- **CEO** (Celestial Ephemeris Origin) — IAU 2000 Resolution B1.8 (the term used in [[sources/capitaine-2003-iau2000-precession-p03]])
- **CIO** (Celestial Intermediate Origin) — IAU 2006 renaming, now standard

Apsis treats CIO as the canonical modern name; CEO/NRO retained as aliases when reading older literature. The corresponding terrestrial-side point is the **TIO** (Terrestrial Intermediate Origin).

## The quantity s

The position of the CEO along the CIP equator, relative to the GCRS, is encoded in a small periodic function **s(t)** — typically of order arcseconds, with the t·X·Y/2 secular term separated out. Used together with the CIP coordinates (X, Y) and the [[concepts/earth-rotation-angle|ERA]] in the CEO-based GCRS↔ITRS transformation per [[sources/capitaine-2003-iau2000-precession-p03]] Eq. 1.

SOFA's `iauS06` routine evaluates s; `iauXys06a` returns (X, Y, s) jointly.

## Apsis use

The CEO-based path is the proposed Apsis convention — see [[decisions/001-use-ceo-based-icrs-to-itrs]]. ERA replaces sidereal time as the time-varying input to the GCRS↔ITRS rotation, which is computationally simpler and avoids equation-of-equinoxes plumbing.
