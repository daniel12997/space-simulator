---
type: source
title: "IERS Conventions (2010) — IERS Technical Note No. 36"
raw_path: docs/raw/specs/iers-conventions-2010/
source_type: spec
reliability: authoritative
ingested: 2026-05-04
authors: [Petit, Gérard; Luzum, Brian (editors)]
publication_date: 2010
venue: "International Earth Rotation and Reference Systems Service (IERS) Technical Note No. 36, Verlag des Bundesamts für Kartographie und Geodäsie, Frankfurt am Main"
---

# IERS Conventions (2010) — IERS Technical Note No. 36

The **canonical specification** for high-precision Earth-related spaceflight dynamics. Defines every model an analyst can be expected to use for celestial-to-terrestrial transformations, geopotential, solid-Earth and ocean tides, polar motion, atmospheric propagation, and relativistic corrections. Apsis treats this document as **the** authoritative reference any time a model choice has to be made; deviations require explicit justification.

The version in `docs/raw/specs/iers-conventions-2010/` comprises the front matter + 11 chapters as separate PDFs.

## Chapter map (matched to Apsis subsystems)

| Ch. | Title | File | Apsis subsystem |
|---|---|---|---|
| 0 | Introduction | `tn36_c0.pdf` | overview |
| 1 | General definitions and numerical standards | `icc1.pdf` | constants table (REQ-PHY-001) |
| 2 | Conventional celestial reference system and frame | `tn36_c2.pdf` | §2 — defines ICRS / [[sources/charlot-2020-icrf3\|ICRF3]] axes |
| 3 | Conventional dynamical realization of the ICRS | `tn36_c3.pdf` | §2 — JPL DE ([[sources/folkner-2009-de421\|DE421]], [[sources/park-2021-de440-de441\|DE440/441]]) realization |
| 4 | Terrestrial reference systems and frames | `icc4.pdf` | §2 — ITRS / ITRF (geodetic surface) |
| **5** | **Transformation between ITRS and GCRS** | **`icc5.pdf`** | **§2 — the ICRS↔ITRS pipeline ([[decisions/001-use-ceo-based-icrs-to-itrs]])** |
| 6 | Geopotential | `icc6.pdf` | §2 (gravity) — references [[sources/pavlis-2012-egm2008\|EGM2008]] |
| 7 | Displacement of reference points (tides) | `icc7.pdf` | §2 (gravity) — solid Earth + ocean + pole tides |
| 8 | Tidal variations in the rotation of the Earth | `icc8.pdf` | §2 — sub-daily polar motion / UT1 |
| 9 | Models for atmospheric propagation delays | `icc9.pdf` | §10 (sensors) — for VLBI / SLR / GNSS observables |
| 10 | General relativistic models for space-time coordinates and equations of motion | `tn36_c10.pdf` | §2 (force model) — Schwarzschild + Lense-Thirring + de Sitter |
| 11 | General relativistic models for propagation | `tn36_c11.pdf` | §10 (sensors) — Shapiro delay, light-time corrections |

## Chapter 5: the load-bearing one for Apsis

Chapter 5 specifies the **ITRS ↔ GCRS transformation** in the IAU 2000/2006 framework. Defines:

- §5.2: IAU 2000 + IAU 2006 resolutions framework.
- §5.3.3: IAU 2000/2006 realization of the [[concepts/celestial-intermediate-pole|CIP]].
- §5.3.4: **Procedures** for terrestrial-to-celestial transformation consistent with the resolutions.
- §5.4.1: Transformation matrix for **polar motion** — `W(t)` = R₁(-y_p) R₂(-x_p) R₃(s'(t)), where s' is the Terrestrial Intermediate Origin (TIO) locator.
- §5.4.2: **CIO-based transformation matrix** for Earth rotation — R₃(-ERA), with ERA from `iauEra00`.
- §5.4.3 (offscreen): GCRS-to-CIRS transformation = `Q(t)` from [[concepts/celestial-ephemeris-origin|X, Y, s]].

Combined: `[ITRS] = W(t) · R₃(-ERA) · Q(t)ᵀ · [GCRS]`. This is exactly the pipeline Apsis implements per [[decisions/001-use-ceo-based-icrs-to-itrs]] using SOFA functions.

## Chapter 6: geopotential

References [[sources/pavlis-2012-egm2008|EGM2008]] (and earlier EGM96) as the conventional Earth gravity model. Specifies:

- Spherical-harmonic expansion conventions (normalization, sign conventions).
- Time-dependent corrections: tidal effects on Stokes coefficients, secular drift.
- Treatment of the **permanent tide** (zero-tide vs tide-free vs mean-tide) — Apsis must pick one consistently.

## Chapter 7: solid-Earth, ocean, atmospheric tides

Modifies station positions and (via geocenter motion) satellite gravitational potential by mm-to-cm-level. Required for sub-cm orbit precision; can be omitted for typical Apsis use cases (REQ-PHY-003 doesn't mandate this fidelity), but the *option* should exist for high-precision scenarios.

## Chapter 10-11: relativistic corrections

- **Schwarzschild + Lense-Thirring + de Sitter** acceleration terms in the force model — important for very precise GNSS, GRACE, Lageos. ~1 mm/s² magnitude — small relative to J₂ but secular.
- **Shapiro delay** for light-time computations (relevant to VLBI, lunar laser ranging, deep-space tracking).

Apsis subsystems §2.5 should specify whether relativistic corrections are included by default (recommended: yes for the gravitational acceleration; off for light-time except for deep-space scenarios).

## Apsis relevance

- **REQ-PHY-001** (Earth fundamental constants) — Ch. 1 numerical standards table is the authoritative source.
- **REQ-INT-002, REQ-INT-003** (ICRS↔ITRS transformations) — **Ch. 5 is the spec**; SOFA cookbooks ([[sources/sofa-2023-earth-attitude-cookbook]]) are the implementation.
- **REQ-PHY-003** (Earth gravity model) — Ch. 6 + EGM2008.
- **REQ-PHY-004** (Lunar/solar third-body) — Ch. 3 dynamical realization + JPL DE.
- **REQ-PHY-005** (relativistic corrections) — Ch. 10.
- **REQ-OBS-***  validation: Apsis must reproduce the IERS Conventions test cases for transformations, geopotential evaluation, and tidal corrections to declared tolerances.

## Items for human review

- Subsystems §2.5 should explicitly select the **permanent-tide convention** (zero-tide is conventional per IERS; some software uses tide-free). Mismatch causes ~30 cm offset in C₂₀.
- Tides (Ch. 7-8) are not currently in subsystems §2.x — confirm whether REQ-PHY-003 implies their inclusion or whether they're an optional extension.
- Relativistic acceleration (Ch. 10) presence should be explicit in subsystems §2.5; default-on for gravitational, default-off for Shapiro/light-time.

## Cross-references

- [[sources/sofa-2023-earth-attitude-cookbook]] — implementation reference for Ch. 5.
- [[sources/sofa-2023-time-scale-cookbook]] — time scales (depends on Ch. 1 numerical standards).
- [[sources/pavlis-2012-egm2008]] — geopotential model referenced by Ch. 6.
- [[sources/folkner-2009-de421]] / [[sources/park-2021-de440-de441]] — ephemerides referenced by Ch. 3.
- [[decisions/001-use-ceo-based-icrs-to-itrs]] — Apsis decision implementing Ch. 5.
- [[sources/kaplan-2005-usno-circular-179]] — pedagogical companion (next ingest).
