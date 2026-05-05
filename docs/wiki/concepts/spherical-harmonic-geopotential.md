---
type: concept
canonical_name: "Spherical-Harmonic Geopotential"
aliases: [spherical harmonic gravity, geopotential, Pines recursion, Cunningham recursion]
created: 2026-05-04
updated: 2026-05-04
---

# Spherical-Harmonic Geopotential

The standard mathematical decomposition of a body's gravitational potential outside its surface, expressed as a series in associated Legendre polynomials of latitude and Fourier components of longitude. For Earth: REQ-PHY-003 mandates degree-and-order-selectable spherical-harmonic gravity supporting [[sources/pavlis-2012-egm2008|EGM2008]].

## The expansion

```
U(r, λ, φ) = (μ/r) [ 1 + Σ_{n=2}^{N} Σ_{m=0}^{n} (R/r)^n P̄_nm(sin φ) (C̄_nm cos mλ + S̄_nm sin mλ) ]
```

with `μ` the body's gravitational parameter, `R` a reference radius (6378.1363 km for EGM2008), `(r, λ, φ)` the spacecraft's body-fixed spherical coordinates, `P̄_nm` the fully-normalized associated Legendre polynomials, and `C̄_nm`, `S̄_nm` the (normalized) potential coefficients. The acceleration is `a = -∇U`.

## Models Apsis uses

| Body | Model | Source | Typical truncation |
|---|---|---|---|
| Earth | EGM2008 | [[sources/pavlis-2012-egm2008]] | 70×70 LEO, 12×12 GEO, 8×8 game-grade |
| Moon | GRGM900C / GRGM1200A | [[sources/lemoine-2014-grgm900c]] | 165×165 low orbit, 50×50 high |
| Mars | GMM-3 / MRO110C | (paywalled, not yet ingested) | 50×50 |

Coefficient files hosted at **ICGEM** (`icgem.gfz-potsdam.de`) for Earth and most other bodies.

## Singularity-free recursions

The naïve recursion via `tan φ` and `sin mλ`/`cos mλ` becomes ill-conditioned near the poles and at high degree. Two equivalent fixes (REQ-PHY-004):

- **Pines (1973)**: introduces a body-fixed Cartesian coordinate system rooted in the body equator. Singularity-free at the poles. The standard for satellite applications. Paywalled — see paywalled-papers section of `docs/raw/INDEX.md`.
- **Cunningham (1970)**: similar derivation via complex variables. Mathematically equivalent. Paywalled.

Apsis can use either; they produce identical accelerations to round-off. Pines is more common in space-flight software; Cunningham slightly faster for certain degree/order ranges.

## Computational cost

The recursion is **O(N²)** in degree N — the dominant cost of high-fidelity force-model evaluation. For N=70 (typical LEO), this is ~5000 evaluations per acceleration call. For N=12 (GEO), ~150 — much cheaper. Subsystems §2.2 enumerates the operational truncation choices.

For [[concepts/gauss-jackson-integration|GJ8]] at 60-second steps, the geopotential dominates compute time. Apsis can use a **dual-truncation** scheme: low N for the predictor pass, full N for the corrector — saves ~half compute at the price of slightly larger predictor error (caught by the corrector).

## Pre-normalized vs unnormalized

EGM2008 coefficients are distributed in **fully-normalized form**, where `P̄_nm = √((2-δ_m0)(2n+1)(n-m)!/(n+m)!) · P_nm`. The normalization factor grows like `n^n` for high degree; without it the unnormalized recursion overflows IEEE double around degree 50. Always work in normalized form internally.

## Variational equations

REQ-PHY-016 calls for `∂a/∂r` partial derivatives for variational equations. The geopotential's partial derivatives have the same recursive structure as the potential itself; computing them adds ~3× to the acceleration cost. Useful for orbit-determination filtering and sensitivity analysis.

## References

- **[[sources/pavlis-2012-egm2008]]** — EGM2008 derivation and accuracy.
- **[[sources/lemoine-2014-grgm900c]]** — lunar gravity model.
- **Pines (1973)** — *Uniform representation of the gravitational potential and its derivatives*, AIAA J 11(11):1508-1511. INDEX-only paywalled.
- **Cunningham (1970)** — *On the computation of the spherical harmonic terms needed during the numerical integration of the orbital motion of an artificial satellite*, Celestial Mech. 2:207-216. INDEX-only paywalled.
- **[[sources/icgem-gfz-landing-snapshot]]** — coefficient hosting service.
- **[[sources/igrf14-2024-coefficients]]** — sister model for Earth's geomagnetic field using Schmidt-normalized spherical-harmonic expansion.
