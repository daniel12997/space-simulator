---
type: source
title: "ICGEM — International Centre for Global Earth Models (GFZ Potsdam landing page)"
raw_path: docs/raw/articles/icgem-gfz-landing-snapshot.html
source_type: article
reliability: authoritative
ingested: 2026-05-05
authors: [Barthelmes, Franz; Ince, Elmas Sinem; Reißland, Sven]
publication_date: 2024
venue: "ICGEM (icgem.gfz.de), GFZ German Research Centre for Geosciences, Potsdam"
---

# ICGEM — International Centre for Global Earth Models

A snapshot of the **ICGEM landing page** at GFZ Potsdam — the canonical public repository for **global Earth gravity-field models** distributed as Stokes coefficients (the [[concepts/spherical-harmonic-geopotential|spherical-harmonic geopotential]] format Apsis consumes). ICGEM hosts ~200 named models including the EGM family ([[sources/pavlis-2012-egm2008|EGM2008]]), GOCE-derived models, GRACE-derived models, GRACE-FO updates, and combined gravity models.

For Apsis, ICGEM is the **download source** for any Earth-gravity model coefficient file beyond what the architecture's vendored copy of EGM2008 covers.

## What ICGEM provides

- **Static gravity models**: degree-truncated `.gfc` files for each named model (e.g., `EGM2008.gfc`, `EIGEN-6C4.gfc`, `XGM2019e_2159.gfc`). Each file contains:
  - GM, reference radius, normalization convention.
  - Stokes coefficients `C_nm, S_nm` to the model's max degree.
  - Per-coefficient standard deviations (where the underlying solution provides them).
- **Time-variable gravity models**: GRACE / GRACE-FO monthly solutions; secular trends; seasonal harmonics.
- **Computational services**: online evaluation of geoid heights, gravity disturbances, gravity anomalies on user-specified grids — useful for validation but not for Apsis runtime.
- **Validation services**: comparison against GPS leveling data, against altimetry-derived geoids, etc.

## Standard `.gfc` file format

ICGEM's `.gfc` text format is widely supported (also by the GeographicLib Geoid utilities). Apsis must parse:

```
product_type             gravity_field
modelname                EGM2008
earth_gravity_constant   3.986004415e+14
radius                   6378136.3
max_degree               2190
norm                     fully_normalized
errors                   formal
tide_system              tide_free

key   L   M       C                        S                       sigma_C            sigma_S
gfc   2   0   -0.484165143790815e-03    0.000000000000000e+00   ...
gfc   2   1   -0.206615509074176e-09    0.138441389137979e-08   ...
...
```

Apsis's reader must respect:

- **Norm** field: `fully_normalized` (Kaula 4π) is standard; `unnormalized` requires renormalization on load.
- **Tide_system** field: `tide_free` vs `zero_tide` vs `mean_tide` — different conventions for the C_2,0 coefficient. Mismatch produces ~30 cm offset; Apsis must pass the convention through to its evaluator.
- **Errors** field: `formal`, `calibrated`, or `none` — affects whether the per-coefficient sigmas are usable for Monte Carlo.
- **Time-variable** extensions (`gfct`, `gfsc`, `dot`, `asin`, `acos` keys) for GRACE-style models.

## Apsis relevance

- **REQ-PHY-003** (spherical-harmonic gravity, user-selectable degree/order) — ICGEM is where users get coefficient files for models other than the vendored EGM2008.
- **REQ-MC-004** (Monte Carlo over gravity-field uncertainty) — `.gfc` per-coefficient sigmas (when present) are the input for Monte Carlo perturbation of the gravity field.
- **REQ-INT-008** (interop with public Earth-science data) — ICGEM is the canonical public source.
- **Subsystems §2.2** (Earth gravity): Apsis ships EGM2008 as the default; users can drop in any `.gfc` model from ICGEM via the same coefficient-file interface.

## Items for human review

- The **tide-system convention** is critical for cm-level gravity work and is easy to get wrong silently. Apsis's loader should make the convention explicit in the in-memory model and warn loudly if mixing different conventions across models in one scenario.
- **Time-variable gravity** (GRACE / GRACE-FO trends, annual cycles) is rarely needed for typical Apsis scenarios but should be plumbed through if any subsystem needs sub-cm-level precision over multi-year arcs.

## Cross-references

- [[sources/pavlis-2012-egm2008]] — vendored Earth gravity model.
- [[sources/lemoine-2014-grgm900c]] — lunar gravity (different repository, but analogous coefficient-file format).
- [[concepts/spherical-harmonic-geopotential]] — the underlying mathematics.
