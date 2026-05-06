---
type: source
title: "Orekit — HolmesFeatherstoneAttractionModel.java"
raw_path: (external; not vendored — visited via gitlab.orekit.org)
source_type: spec
reliability: secondary-reputable
ingested: 2026-05-05
authors: [Orekit Contributors]
publication_date: 2002-2024
venue: "Orekit project (CS GROUP), source file `src/main/java/org/orekit/forces/gravity/HolmesFeatherstoneAttractionModel.java`"
license: "Apache-2.0"
url: "https://gitlab.orekit.org/orekit/orekit/-/blob/main/src/main/java/org/orekit/forces/gravity/HolmesFeatherstoneAttractionModel.java"
---

# Orekit — `HolmesFeatherstoneAttractionModel.java` (analytical SH gradient reference)

Orekit is the open-source flight-dynamics library maintained by the CS GROUP. Its `HolmesFeatherstoneAttractionModel` class implements high-degree spherical-harmonic gravitational attraction with analytical first and second partial derivatives of the potential (i.e. the acceleration and its position-Jacobian), using the **Holmes-Featherstone (2002)** modified-forward-row recursion of fully-normalised associated Legendre functions.

This page is the wiki source citation for the file — Apsis's [[components/spherical-harmonic|SphericalHarmonic]] component cites Orekit as the open-source reference implementation for its analytical SH gradient (Phase-1A §C2). The Apsis adapter uses the **Cunningham V/W** formulation rather than Holmes-Featherstone (the Cunningham gradient cleanly extends the existing V/W skeleton with one extra row; a Holmes-Featherstone port would have required rewriting the whole basis), but Orekit provides the canonical Apache-2.0-licensed open-source sibling whose correctness is well-validated against external orbit-determination products and published reference orbits.

## What Orekit's class implements

- **Acceleration**: `−∇U` for `U = (μ/r) Σ_{n,m} (R/r)^n P̄_{nm}(sin φ) (C̄_{nm} cos mλ + S̄_{nm} sin mλ)` evaluated up to user-selected `(degree, order)`.
- **Position-Jacobian**: `∂a/∂r` analytically, by recurrence on the same scaled-Legendre basis used for the value (no FD).
- **Gradient with respect to potential coefficients**: useful for orbit-determination filtering against the geopotential.
- **Reference frame**: input position is in the body-fixed frame (Orekit handles ICRF↔ITRF rotation upstream of the model itself, just like Apsis).

## Algorithm — modified forward-row recursion

Holmes & Featherstone 2002 (Journal of Geodesy 76:279–299, "A unified approach to the Clenshaw summation and the recursive computation of very high degree and order normalised associated Legendre functions") give a recurrence on `P̄_{nm} / u^m` where `u = sin θ` (co-latitude). Pre-computed coefficients (`g_{n,m}`, `h_{n,m}`, `e_{n,m}` per the paper's eq 19, 22, 27, 30) make the recurrence stable to very high degree. Orekit's class:

- Stores `P̄_{nm}/u^m` and propagates row-by-row in `m`.
- Uses precomputed `sectorial[m]` arrays for `P̄_{mm}/u^m`.
- First-derivative recurrence (eq 30) and second-derivative recurrence (eq 30 differentiated) both expressed in the same scaled basis.
- Outer loop: `m` (descending); inner: `n` (ascending).

## Why Apsis chose Cunningham V/W instead

Apsis already had a Cunningham V/W skeleton from Phase 1 (`acceleration_body` in `src/force/spherical_harmonic.cc`). The Cunningham gradient (Montenbruck-Gill §3.2.5; M-G eq 3.33) expresses the second derivatives of the potential as the same V/W table read at `(n+2, m'')`, so the gradient is one extra row of the existing recursion plus the same per-(n, m) assembly loop. Porting Holmes-Featherstone would have meant a full rewrite of the basis. Both formulations are mathematically equivalent and produce the same acceleration and Jacobian to round-off; Holmes-Featherstone is preferred for very high degree (≥ 1500) where the Cunningham un-normalisation factor overflows. Apsis's working ceiling is degree 70 (LEO) / 165 (lunar) per [[concepts/spherical-harmonic-geopotential]], well within Cunningham's stable range.

The Orekit file is cited in `src/force/spherical_harmonic.cc` as the open-source sibling whose correctness was studied while implementing the Cunningham gradient. Both approaches were checked against the same FD oracle in Apsis's conformance test (`tests/conformance/force_model_ve_contract.cc`).

## License compatibility

Orekit is Apache-2.0; Apsis is Apache-2.0. The Apsis source file carries an attribution comment per the Apache-2.0 NOTICE conventions; this wiki page is the corresponding citation entry.

## References

- Holmes, S. A., & Featherstone, W. E. (2002). *A unified approach to the Clenshaw summation and the recursive computation of very high degree and order normalised associated Legendre functions*. Journal of Geodesy 76: 279–299.
- Cunningham, L. E. (1970). *On the computation of the spherical harmonic terms needed during the numerical integration of the orbital motion of an artificial satellite*. Celestial Mechanics 2: 207–216. (Source of the V/W formulation that Apsis uses.)
- Montenbruck, O. & Gill, E. (2000). *Satellite Orbits — Models, Methods, and Applications*. Springer. §3.2 (Cunningham V/W gradient explicitly worked out in §3.2.5).
- Vallado, D. A. (2013). *Fundamentals of Astrodynamics and Applications*, 4th ed. §8.6 (Cunningham gradient cross-derived).
- Orekit project home: https://www.orekit.org/
- Source file: https://gitlab.orekit.org/orekit/orekit/-/blob/main/src/main/java/org/orekit/forces/gravity/HolmesFeatherstoneAttractionModel.java

## Cross-references

- [[concepts/spherical-harmonic-geopotential]] — the algorithm context.
- [[decisions/009-hand-rolled-integrator-family]] — Phase 1 / Phase 1A Implementation Note records the C2 closure that this source justifies.
- [[sources/pavlis-2012-egm2008]] — the coefficient set Apsis evaluates with this code path.
