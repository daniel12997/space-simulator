---
type: source
title: "The IAU Resolutions on Astronomical Reference Systems, Time Scales, and Earth Rotation Models — Explanation and Implementation"
raw_path: docs/raw/specs/kaplan-2005-usno-circular-179.pdf
source_type: spec
reliability: authoritative
ingested: 2026-05-04
authors: [Kaplan, George H.]
publication_date: 2005-10
venue: "USNO Circular No. 179, US Naval Observatory"
---

# Kaplan (2005) — USNO Circular 179: IAU resolutions explained

The **pedagogical companion** to [[sources/iers-conventions-2010|IERS Conventions 2010]]. Where IERS Conventions reads like a normative spec, Kaplan reads like a textbook chapter — explains *why* each IAU 2000/2006 resolution made the choice it did, what changed from previous practice, and how to implement it correctly. The implementation-notes sections are particularly valuable when an Apsis developer needs to debug a discrepancy with reference data.

## Contents map

- **§1 Relativity** — BCRS vs GCRS, computing observables under general relativity. Frames Apsis force-model and light-time choices in their physical context.
- **§2 Time scales** — different "flavors of time", SI-second-based scales (TAI, TT, TCG, TCB, TDB), Earth-rotation-based scales (UT0, UT1, UTC), the leap-second debate ("To Leap or Not to Leap"), explicit conversion formulas. Companion to [[sources/sofa-2023-time-scale-cookbook]] but with motivation, not just function calls.
- **§3 The Fundamental Celestial Reference System** — ICRS, ICRF, HCRF history; how the modern celestial frame replaces the old equator-and-equinox-of-J2000 system; what changed in IAU 1991 and 1997 leading to the current state.
- (Beyond what's pictured): chapters on IAU 2000 precession-nutation, the CIP, the CIO, the equation of the origins, and worked examples.

## Why this matters even with IERS Conventions in the corpus

IERS Conventions 2010 tells you **what to do**. Kaplan tells you **why** and **what to watch out for**:

- The IERS Conventions specify polynomial coefficients to many decimals; Kaplan explains why those *particular* polynomials were chosen and what the prior values were.
- IERS Conventions specifies the CIO-based and equinox-based pipelines as alternatives; Kaplan §2.5 ("To Leap or Not to Leap") and §3 explain the history that led to *both* being kept.
- IERS Conventions assumes you understand BCRS / GCRS distinction; Kaplan §1 walks you through it from first principles.

For Apsis development, this means: **read Kaplan first** when introducing yourself or a new contributor to a piece of the celestial-frame stack; **read IERS Conventions** when implementing or validating.

## Key sections for Apsis

- **§1.2 The BCRS and the GCRS** — the relativistic distinction Apsis subsystems §2.5 must respect.
- **§2.6 Formulas** — direct implementation recipes (alongside the SOFA cookbook).
- **§3.4.3 Standard Algorithms** — the algorithm-by-algorithm summary Apsis can use as a sanity check during implementation review.

## Apsis relevance

- **REQ-INT-001/002/003** (time scales, Earth attitude) — the explanatory companion to the SOFA cookbooks and IERS Conventions Ch. 5.
- **Onboarding / training material** — Apsis docs/02-subsystems.md §2 should cite Kaplan as recommended reading for new GNC/dynamics developers; it's much more accessible than IERS Conventions for first-pass understanding.

## Cross-references

- [[sources/iers-conventions-2010]] — the normative spec this explains.
- [[sources/sofa-2023-earth-attitude-cookbook]] — implementation cookbook.
- [[sources/sofa-2023-time-scale-cookbook]] — time-scale implementation cookbook.
- [[concepts/iau-2006-precession]] / [[concepts/celestial-ephemeris-origin]] — the concepts Kaplan motivates.
