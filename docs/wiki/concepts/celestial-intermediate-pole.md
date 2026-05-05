---
type: concept
canonical_name: "Celestial Intermediate Pole"
aliases: [CIP]
created: 2026-05-04
updated: 2026-05-04
---

# Celestial Intermediate Pole (CIP)

The pole defined by IAU Resolution B1.7 (2000) as the "intermediate" axis whose motion in the GCRS is described by IAU 2006/2000A precession-nutation for periods longer than two days, with shorter-period (high-frequency) nutation contributions provided by IERS as additional time-dependent corrections.

The CIP is the rotation axis used as the z-axis of the **intermediate frame** sitting between the inertial GCRS and the terrestrial ITRS. The CCI ↔ CCF transformation in Apsis's subsystems §1.3 fundamentally rotates GCRS into a frame whose z-axis is the CIP.

## CIP coordinates in GCRS

The CIP's direction in GCRS is given by two coordinates conventionally denoted **X** and **Y**. Their polynomial parts (precession + frame bias contribution) are provided directly by [[sources/capitaine-2003-iau2000-precession-p03]] Eqs 49-50; the periodic (nutation) parts come from the IAU 2000A series. These X, Y coordinates are the natural inputs to the [[concepts/celestial-ephemeris-origin|CEO]]-based GCRS↔ITRS transformation.

In the equinox-based pipeline, the same pole is reached by composing rotations using the angles ψ_A, ω_A, ε_A, χ_A (4-rotation form) or ζ_A, z_A, θ_A (3-rotation form). All routes converge to the same CIP direction; the choice is computational.

## Pre-2003 nomenclature

The CIP replaced the older "Celestial Ephemeris Pole" (CEP) of IAU 1976/1980 conventions. They are different conceptually (CIP is defined kinematically with periods > 2 days reserved for precession-nutation; CEP was defined dynamically). Apsis uses CIP exclusively.
