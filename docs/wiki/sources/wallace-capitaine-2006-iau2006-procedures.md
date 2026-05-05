---
type: source
title: "Precession-nutation procedures consistent with IAU 2006 resolutions"
raw_path: docs/raw/papers/wallace-capitaine-2006-iau2006-procedures.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Wallace, P. T.; Capitaine, N.]
publication_date: 2006-12
venue: "Astronomy & Astrophysics 459:981-985"
doi: 10.1051/0004-6361:20065897
---

# Wallace & Capitaine (2006) — IAU 2006 procedures

The practical implementation reference for [[concepts/iau-2006-precession]] + [[concepts/precession-nutation|IAU 2000A nutation]] in software libraries. Recommends two procedures that achieve <1 µas internal consistency for current-epoch dates, <10 µas at ±2 centuries:

1. **Angles-based** (recommended for SOFA-style algorithm collections): use the Fukushima-Williams 4-rotation parameterization (γ̄, φ̄, ψ̄, ε_A) for the bias-precession matrix, add IAU 2000A-with-IAU 2006-compatibility-correction nutation. Output the canonical PB and N matrices.
2. **X, Y series-based** (recommended for IERS Conventions / CEO-based pipelines): compute the [[concepts/celestial-intermediate-pole|CIP]] X, Y coordinates directly from explicit series. The X, Y series for the first time generate equinox-based as well as CIO-based products from the same canonical components — eliminates duplication of large nutation series.

Both procedures support **equinox-based and [[concepts/celestial-ephemeris-origin|CEO]]-based** transformations from a single canonical model set.

## Apsis relevance

- The recommended procedures map directly onto SOFA's `iauPnm06a`, `iauXys06a`, `iauC2t06a` etc. — Apsis's [[decisions/001-use-ceo-based-icrs-to-itrs]] uses the X, Y series-based procedure (procedure 2 above).
- Eq. 5 of this paper gives the **adjustment to IAU 2000A nutation to make it compatible with IAU 2006 precession**: `Δψ = Δψ_2000A · (1 + 0.4697e-6 + f)`, with `f = (J̇₂/J₂) · t = -2.7774e-6 t`. The adjustment is a 0.5 ppm scaling — small but mandatory for sub-µas consistency.
- Appendix gives a fully worked numerical example for an arbitrary test date — useful as an oracle for Apsis's frame-transformation tests.

## Cross-references

- [[sources/capitaine-2003-iau2000-precession-p03]] — IAU 2006 precession (P03) derivation
- [[sources/mathews-2002-mhb2000-nutation]] — IAU 2000A nutation derivation
- [[concepts/iau-2006-precession]], [[concepts/precession-nutation]], [[concepts/celestial-intermediate-pole]], [[concepts/celestial-ephemeris-origin]]
