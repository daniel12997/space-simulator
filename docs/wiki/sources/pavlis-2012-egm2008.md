---
type: source
title: "The development and evaluation of the Earth Gravitational Model 2008 (EGM2008)"
raw_path: docs/raw/papers/pavlis-2012-egm2008.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Pavlis, Nikolaos K.; Holmes, Simon A.; Kenyon, Steve C.; Factor, John K.]
publication_date: 2012-04
venue: "J. Geophysical Research 117:B04406"
doi: 10.1029/2011JB008916
---

# Pavlis et al. (2012) — Earth Gravitational Model 2008 (EGM2008)

The reference paper for **EGM2008** — the high-resolution spherical-harmonic Earth gravity model that Apsis uses for [[concepts/spherical-harmonic-geopotential|spherical-harmonic geopotential]] computation per REQ-PHY-003. EGM2008 is **complete to degree and order 2159** with additional coefficients to degree 2190 / order 2159, derived from a least-squares combination of the GRACE03S satellite-only model and a 5-arcminute global grid of free-air gravity anomalies (terrestrial + altimetry-derived + airborne).

## Accuracy

- Geoid undulation commission error: ±5 to ±10 cm over areas with high-quality input data.
- Vertical deflections over USA and Australia: ±1.1 to ±1.3 arcseconds.
- 6× resolution improvement and 3-6× accuracy improvement over predecessor EGM96.

## What Apsis uses it for

- **REQ-PHY-003** ("spherical-harmonic gravity to user-selectable degree and order, supporting EGM2008 for Earth"): Apsis's geopotential force module loads EGM2008 normalized coefficients up to a user-selected truncation degree N×N. Subsystems §2.2 lists typical configurations: 70×70 for high-fidelity LEO, 12×12 for GEO, 8×8 for game-grade.
- **REQ-PHY-004** ("singularity-free formulation (Pines or equivalent), pre-normalized coefficients"): EGM2008 distribution is in fully-normalized form. Pines recursion (or equivalent Cunningham recursion) is the implementation.

## Coefficient distribution

EGM2008 coefficients are hosted at the **ICGEM** service (`icgem.gfz-potsdam.de`) — see [[sources/icgem-gfz-landing-snapshot]] (article snapshot) for the catalog page. Native format is a tabular file with `(n, m, C̄_nm, S̄_nm, σ_C, σ_S)` records. Apsis loads these once at startup; the recursion at evaluation time multiplies them by Pines' polynomials.

## Storage cost

Truncation to degree N requires `(N+1)² ≈ N²` coefficients. For N=70 (high-fidelity LEO): ~5000 coefficients × 32 bytes (C̄, S̄, σ_C, σ_S) ≈ 160 KB. For full N=2159: 4.7 million coefficients (paragraph 19) ≈ 150 MB. Apsis should store the full file once and load only the truncation needed at runtime.

## Cross-references

- [[concepts/spherical-harmonic-geopotential]] — the underlying mathematical method.
- [[sources/lemoine-2014-grgm900c]] — the lunar analogue (GRGM900C).
- [[sources/icgem-gfz-landing-snapshot]] — coefficient hosting service.

## Surfaced for human review (no silent spec edits)

The architecture's build-vs-reuse table (§5) lists "EGM2008 coefficients | Free, ICGEM hosts." Worth one line in subsystems §2.2 noting the Pines vs Cunningham choice for the recursion algorithm — both are mathematically equivalent and singularity-free; the choice is implementation. (No action needed; just visibility for the implementer.)
