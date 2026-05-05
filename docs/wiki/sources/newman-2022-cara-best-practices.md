---
type: source
title: "Conjunction Assessment: NASA Best Practices and Lessons Learned"
raw_path: docs/raw/papers/newman-2022-cara-best-practices.pdf
source_type: paper
reliability: authoritative
ingested: 2026-05-04
authors: [Newman, Lauri K.; Mashiku, Alinda K.]
publication_date: 2022-09
venue: "Advanced Maui Optical and Space Surveillance Technologies Conference (AMOS)"
---

# Newman & Mashiku (2022) — NASA CARA conjunction-assessment best practices

NASA Conjunction Assessment Risk Analysis (CARA) team's distillation of operational lessons learned, framed as a guide to using the **NASA Spacecraft Conjunction Assessment and Collision Avoidance Best Practices Handbook** (CA² Handbook, NASA/SP-20205011276, December 2020). Defines the design-time and operations-time practices Apsis must support if it's to be useful for real CA workflows, not just the [[concepts/sgp4|SGP4]]+[[sources/foster-estes-1992-jsc-25898-pc|Foster-Pc]] math.

## Topics covered (matched to Apsis requirements)

### Choosing a mission orbit

CARA recommends mission designers run **conjunction-rate trade studies** at design time — small altitude shifts (e.g., 480 km vs 500 km) can drastically reduce expected close-approach rate (Figure 1). This is not just "optimize for science"; it's "optimize for sustainable conjunction-mitigation cadence."

→ **REQ-CAT-002 + REQ-MC-006** (catalog-scale Monte Carlo for conjunction-rate prediction): Apsis should be capable of running conjunction-rate sweeps over candidate orbits as a mission-design aid.

### Trackability / catalog maintenance

Spacecraft must be trackable by USSPACECOM SSN sensors (radar/optical) for the entire orbital lifetime including post-passivation. Rule of thumb: **≥10 cm characteristic dimension for perigee < 2000 km**, **≥50 cm for perigee > 2000 km**. NASA case study: a low-inclination LEO mission was infrequently visible to SSN sensors → 10-day TLE update gaps → mission scramble post-launch. Now NASA requires a **trackability study during design**.

→ **REQ-SCN-001/002** (mission-design scenario authoring): Apsis should support trackability assessment as a scenario type — propagate truth, simulate SSN coverage geometry, compute revisit-interval statistics.

### Ephemeris production

Predicted ephemerides should be furnished to USSPACECOM **at least daily** for LEO, **immediately after planned/executed maneuvers**. Typical **7-day predictive duration**. **Sub-minute spacing for LEO**, **10-min for higher circular**, **regularized spacing in true anomaly for HEO**. Each ephemeris point must include a **6×6 (or larger) covariance** with realistic variance/covariance terms.

→ **REQ-INT-007 + CCSDS OEM/CDM compatibility** (in `docs/raw/specs/`): Apsis must export ephemerides in CCSDS Orbit Ephemeris Message format with full 6×6 (or 9×9 / coupled) covariances. Spacing rules above are the constraint.

### Covariance realism

CARA case study: a mission's ephemerides diverged from 18 SDS calculations because their covariance was unrealistic → many spurious alerts → wasted analyst time. **Covariance realism is more important than nominal-state accuracy** for downstream Pc consumers.

→ **REQ-MC-005 + REQ-OBS-005** (observability of covariance evolution): Apsis must validate covariance growth against Monte Carlo dispersions; nominal-only validation is insufficient.

### Maneuver-data sharing

Operators sometimes withhold predicted maneuver ephemerides as proprietary. CARA position: that protection is illusory because TLEs already reveal trajectory. Withholding *post-burn* predicted ephemerides actively *increases* CA risk for everyone.

### Non-Earth-centered CA

Cislunar / lunar-orbit CA is becoming relevant; existing tooling is Earth-centric. CA² Handbook §X identifies this as an open problem.

→ **Subsystems §6** (CA architecture) — Apsis should not hard-code Earth-only assumptions; the Pc / B-plane math is geometry, not Earth-specific.

## Apsis relevance summary

- **REQ-CAT-005** (Pc computation, Foster method) — this paper defines the *operational context* into which Foster sits.
- **REQ-CAT-008** (CAM planning) — operational triggering criteria, decision timelines, fuel-budget tracking.
- **REQ-INT-007** (CCSDS ephemeris IO) — exact OEM/CDM-message conformance requirements come from this paper + the CA² Handbook.
- **REQ-OBS-***  — covariance-realism validation is a first-class Apsis capability driven by this paper.
- **REQ-SCN-***  — trackability and conjunction-rate scenario types belong in the scenario catalog.

## Cross-references

- [[sources/foster-estes-1992-jsc-25898-pc]] — the underlying Pc method.
- [[sources/bombardelli-2015-collision-avoidance]] — analytical CAM Δv design that consumes Pc.
- CCSDS 508.0-B-1 CDM (in `docs/raw/specs/`) — the CDM format CARA produces.
- CCSDS 502.0-B-2 OEM (in `docs/raw/specs/`) — the OEM format O/Os submit.
- NASA/SP-20205011276 (CA² Handbook) — the authoritative reference this paper guides.
