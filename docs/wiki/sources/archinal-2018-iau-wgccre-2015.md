---
type: source
title: "Report of the IAU Working Group on Cartographic Coordinates and Rotational Elements: 2015"
raw_path: docs/raw/specs/archinal-2018-iau-wgccre-2015.pdf
source_type: spec
reliability: authoritative
ingested: 2026-05-05
authors: [Archinal, B. A.; Acton, C. H.; A'Hearn, M. F.; Conrad, A.; Consolmagno, G. J.; Duxbury, T.; Hestroffer, D.; Hilton, J. L.; Kirk, R. L.; Klioner, S. A.; McCarthy, D.; Meech, K.; Oberst, J.; Ping, J.; Seidelmann, P. K.; Tholen, D. J.; Thomas, P. C.; Williams, I. P.]
publication_date: 2018-02
venue: "Celestial Mechanics and Dynamical Astronomy 130:22"
doi: 10.1007/s10569-017-9805-5
---

# Archinal et al. (2018) — IAU WGCCRE 2015 cartographic coordinates and rotational elements

The **2015 triennial report** of the IAU Working Group on Cartographic Coordinates and Rotational Elements (WGCCRE), updating the recommended **rotation-axis directions, rotation rates, prime-meridian definitions, body shapes, and sizes** for planets, satellites, minor planets, and comets. This is the **canonical IAU spec** for planetary-body orientation models that NAIF SPICE PCK kernels distribute.

## What's in the 2015 report

Body-orientation polynomial expressions (right ascension, declination of the rotation pole; prime meridian angle) take the standard form:

```
α₀(T) = α_const + α_T · T + Σ a_i sin/cos(θ_i(T))    [pole RA]
δ₀(T) = δ_const + δ_T · T + Σ d_i sin/cos(θ_i(T))    [pole Dec]
W(T)  = W_const + W_d  · d + Σ w_i sin/cos(θ_i(T))   [prime-meridian angle]
```

with `T` in Julian centuries TDB, `d` in days TDB, both from J2000. Coefficients `θ_i` are body-specific harmonic arguments (e.g., for Mars: nutation arguments M1...M14).

The 2015 update revised:

- **Mercury** — based on MESSENGER observations.
- **Mars + Phobos + Deimos** — refined longitude definition.
- **Ceres, Europa, Ida, Šteins, Neptune, Pluto+Charon** — new/updated rotation models.
- **Comets** 9P/Tempel 1, 19P/Borrelly, 67P/Churyumov-Gerasimenko, 103P/Hartley 2 — added with epoch-of-validity caveats.
- **Body sizes/shapes** updated for many small bodies (Ceres, Europa, Psyche, Pluto, Charon, Itokawa, Vesta).
- **Earth low-precision expression and Moon orientation series removed** to avoid confusion — for Earth use [[sources/iers-conventions-2010|IERS Conventions]] / [[sources/sofa-2023-earth-attitude-cookbook|SOFA]] instead; for the Moon use the NAIF DE-bundled lunar libration kernel.
- **Cartographic vs orthoprojection / geophysical use** — for Moon and Titan, the *cartographic* radius differs from the *physical mean* radius — Apsis must pick the right one for each use case (mapping vs gravity).

## Conventions

- **Planetographic vs planetocentric latitude/longitude**: planetographic uses the local-normal-to-the-reference-spheroid (cartographic-friendly); planetocentric uses the radial direction (physics-friendly). Apsis must store body-fixed positions unambiguously labeled with their convention.
- **Right-hand rule** for cardinal directions on small bodies (revised in 2015).
- **Longitude convention**: positive eastward by default; some bodies (Moon historically) have used positive westward — Apsis must store the sign explicitly.

## Apsis relevance

- **REQ-INT-006** (planet orientation) — this report is the spec; NAIF PCK kernels are the implementation distribution.
- **REQ-PHY-004** (third-body gravity for Moon and major planets) — uses pole orientations from this report when computing body-fixed effects (e.g., lunar gravity in lunar body-fixed frame requires the lunar libration model from this report).
- **REQ-SCN-***  (mission-design scenarios at planets/moons): when authoring a Mars-orbit scenario, Apsis pulls Mars's orientation from this report (via PCK kernel).
- **Subsystems §2.2** (planetary body data): references this report for orientation; references companion DE ephemerides ([[sources/folkner-2009-de421]] / [[sources/park-2021-de440-de441]]) for translation.

## Items for human review

- Subsystems §2.2 should explicitly distinguish **cartographic vs geophysical radius** for Moon and Titan operations.
- Comet rotation models are valid only at specific epochs (per this report's footnotes); Apsis must check epoch-of-validity when using them, not assume they're good throughout a long mission.
- The IAU 76 / Davies-style polynomial system is itself a simplified model; for high-precision lunar orientation use the JPL DE-bundled libration model (a more detailed spectral expansion), not this report's series.

## Cross-references

- [[sources/naif-spice-required-reading]] — PCK kernels distribute these polynomial coefficients.
- [[sources/folkner-2009-de421]] / [[sources/park-2021-de440-de441]] — JPL DE planetary translation; pairs with this report's planetary rotation.
- [[sources/lemoine-2014-grgm900c]] — lunar gravity uses lunar body-fixed coordinates defined per this report.
- IERS Conventions 2010 — for Earth specifically (this report defers Earth to IERS).
