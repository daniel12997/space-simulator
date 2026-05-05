---
type: concept
canonical_name: "IAU 2006 Precession"
aliases: [P03, IAU 2006 precession model]
created: 2026-05-04
updated: 2026-05-04
---

# IAU 2006 Precession

The polynomial precession model adopted by IAU GA 2006 Resolution B1 as the IAU standard, replacing the earlier IAU 2000 precession (which itself was a linear-rate correction to IAU 1976). "IAU 2006 precession" and the symbol **P03** are the same model, derived in [[sources/capitaine-2003-iau2000-precession-p03]] §7.1 (Eqs 37-38).

This naming equivalence is easy to lose: a future reader of Apsis's architecture text "IAU 2006/2000A precession-nutation" should know that the "IAU 2006" half **is the P03 polynomial set in this 2003 paper, adopted three years later by the IAU**. The 2006 part of the label is the date of adoption, not the date of derivation.

## What it provides

Closed-form polynomials in TT-centuries-since-J2000 (denoted t) for:

- Primary equatorial quantities **ψ_A, ω_A** ([[sources/capitaine-2003-iau2000-precession-p03]] Eq. 37)
- Primary ecliptic quantities **P_A, Q_A** (Eq. 38)
- Derived classical quantities **ε_A, χ_A, p_A, ζ_A, z_A, θ_A** (Eqs 39-40)
- Ecliptic angles **π_A, Π_A** (Eq. 41)
- Revised **GMST** expression compatible with the new precession (Eqs 42-43)
- CIP coordinates **X_P03, Y_P03** for the polynomial-only contribution (Eqs 49-50)

Coefficients are in arcseconds; t is in Julian centuries of TT.

## Validity

Designed to be valid to **~1 μas** within ±a few centuries of J2000, degrading to **~10 μas** at ±a millennium ([[sources/capitaine-2003-iau2000-precession-p03]] §7.1). Beyond that, polynomial extrapolation is not trustworthy.

## Frame bias is separate

P03 supplies precession only. The constant **frame-bias rotation** between GCRS and the J2000 mean equatorial frame (~17 mas) is a separate step — see [[concepts/frame-bias]]. The full bias-precession matrix is **R = P · B**, where B is built from (η₀, ξ₀, dα₀) per IAU 2006 conventions.

## Improvements over IAU 2000

P03 corrects the IAU 2000 precession by ~5 μas/cy² in ψ_A and ~25 μas/cy² in ω_A by propagating the MHB precession-rate corrections through the higher-degree (t², t³, …) terms rather than the linear term only ([[sources/capitaine-2003-iau2000-precession-p03]] §6.2.5, p. 579), and by fitting the ecliptic motion to the more recent JPL DE406 ephemeris.

## Implementation

Apsis delegates to SOFA. Relevant routines:

- `iauP06e` — IAU 2006 precession angles
- `iauPmat06` — IAU 2006 precession matrix (frame-bias × precession)
- `iauPnm06a` — full precession-nutation matrix (IAU 2006 precession + IAU 2000A nutation, see [[sources/mathews-2002-mhb2000-nutation]] for the nutation series derivation)
- `iauXys06a` — CIP X, Y, s for the CEO-based pipeline

(See SOFA's `tools` documentation, to be ingested separately.)
