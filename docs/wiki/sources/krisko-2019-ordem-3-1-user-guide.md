---
type: source
title: "NASA Orbital Debris Engineering Model ORDEM 3.1 — Software User Guide"
raw_path: docs/raw/specs/krisko-2019-ordem-3-1-user-guide.pdf
source_type: spec
reliability: authoritative
ingested: 2026-05-05
authors: [Vavrin, Andrew; Manis, Alyssa; Seago, John; Gates, Drake; Anz-Meador, Phillip; Xu, Yu-Lin; Barahona, Ronald; Malachi, Avery; Bigger, Ian; Matney, Mark; Liou, J.-C.]
publication_date: 2019-12
venue: "NASA/TP-2019-220448, Orbital Debris Program Office, NASA Johnson Space Center"
---

# Vavrin et al. (2019) — ORDEM 3.1 user guide

NASA's **Orbital Debris Engineering Model 3.1** — the canonical engineering reference for **debris flux** as a function of altitude, inclination, debris size, and material in LEO/MEO/GEO. Where SSN-tracked catalog data covers debris ≥ 10 cm (LEO) / ~50 cm (GEO), ORDEM 3.1 statistically extends down to **µm-class** particles via flux models calibrated against radar (Goldstone, Haystack), telescope, and in-situ (LDEF, HST, returned hardware) measurements.

ORDEM is what spacecraft designers use to estimate **micrometeoroid + orbital debris (MMOD) impact rate, penetration probability, and shielding requirements** during mission design, and what mission-rate trade studies (per [[sources/newman-2022-cara-best-practices|Newman & Mashiku 2022]]) compare to in their conjunction-rate plots.

## What ORDEM 3.1 outputs

For a user-specified spacecraft (orbit + duration + cross-section), ORDEM produces:

- **Cross-section flux** of debris encounters (impacts / m² / yr) as a function of debris size bin (10 µm → 1 m) and material (Al, steel, NaK, paint, etc.).
- **Velocity distribution** of impacts (1-15 km/s typical, peaks differ by orbit geometry).
- **Directionality** of impacts (in spacecraft local frame — predominantly ram-direction for LEO).
- **Future projections** out to ~2050 based on traffic + breakup-rate models.

## Why a separate model from the SSN catalog

- SSN catalog is **deterministic** (each TLE is a specific tracked object with a state estimate); ORDEM is **statistical** (probability density over orbital element space and size).
- SSN covers ≥ 10 cm; ORDEM covers ≥ 10 µm.
- SSN updates daily; ORDEM updates ~5-yearly.
- For [[sources/foster-estes-1992-jsc-25898-pc|Pc-based conjunction screening]], use the SSN catalog. For shield-sizing, expected-MMOD-strike-rate, and lifetime-impact-risk analysis, use ORDEM.

## Apsis relevance

- **REQ-CAT-007** (debris environment characterization) — ORDEM is the canonical model. Apsis subsystems §6 should support both:
  - **Catalog-based screening** (REQ-CAT-005) using TLEs + Foster Pc.
  - **Statistical-flux assessment** using ORDEM for sub-tracking-threshold debris.
- **REQ-MC-006** (Monte Carlo over debris environment uncertainty) — ORDEM supplies population uncertainty bands; Monte Carlo over them produces lifetime-risk distributions for MMOD shielding sizing.
- **REQ-SCN-***  (mission-design scenarios) — ORDEM provides the input for "what's the expected MMOD strike rate over the 7-year mission?" type queries.
- **Subsystems §1** build-vs-reuse table — Apsis links the ORDEM 3.1 distribution as a black-box library with a config-file interface; not re-implemented.

## Distribution

ORDEM 3.1 is distributed as a **standalone Windows + Linux executable** with input/output text files. Apsis wraps it via subprocess invocation rather than direct linking (no public API library). License: free for US persons via NASA Software Catalog; export-controlled per ITAR/EAR as a debris-environment model.

## Items for human review

- ORDEM 3.1 is **export-controlled** — Apsis distribution model and access policy must accommodate this. International contributors may not have access; the architecture should make ORDEM an optional plug-in rather than a hard dependency.
- ORDEM future projections rely on assumed traffic + breakup-rate models that are themselves uncertain — Apsis users should treat post-2030 projections as scenario-dependent.

## Cross-references

- [[sources/foster-estes-1992-jsc-25898-pc]] — Pc method for tracked-debris encounters.
- [[sources/newman-2022-cara-best-practices]] — operational CA practices that consume both SSN catalog and ORDEM.
- [[concepts/sgp4]] — the deterministic-catalog propagator that complements ORDEM's statistical model.
