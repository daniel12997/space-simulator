---
type: source
title: "Revisiting Spacetrack Report #3: Rev 2"
raw_path: docs/raw/papers/vallado-2006-revisiting-spacetrack-3.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Vallado, David A.; Crawford, Paul; Hujsak, Richard; Kelso, T. S.]
publication_date: 2006-08
venue: "AIAA/AAS Astrodynamics Specialist Conference, Keystone CO — AIAA 2006-6753-Rev2"
---

# Vallado, Crawford, Hujsak & Kelso (2006) — Revisiting Spacetrack Report #3 (Rev 2)

The **modern reference implementation of SGP4**, synthesizing the various AFSPC-internal modifications that have accumulated since the original [[sources/hoots-roehrich-1980-spacetrack-report-3]] (in `docs/raw/specs/`). Provides public, non-proprietary source code, test cases, and analysis intended to be highly compatible with the operational AFSPC version. This is the [[concepts/sgp4|SGP4]] code Apsis links via the architecture's build-vs-reuse table for REQ-INT-005 / REQ-CAT-002.

## Why this paper exists

After STR#3 (1980), the operational AFSPC SGP4 code accumulated **decades of unpublished corrections** for rare cases (decay, near-resonance, deep-space transitions) that were never folded back into a public release. Independent researchers (notably Paul Crawford's "Dundee code" and the GSFC SeaWiFS-mission release) tracked many of these inferred from observations, but the public/operational divergence reached the point where the code in STR#3 *no longer matches the code used to produce the TLEs*. This paper closes that gap with a unified, modern, non-proprietary implementation.

## What's included

- **Reference C/C++ source code** for SGP4 + SDP4 (deep-space) merged as a single propagator, matching the modern AFSPC merging.
- **Comprehensive test cases** — the Vallado test set is the de facto standard SGP4 conformance suite. Apsis must pass every test in this set to claim REQ-INT-005 compliance.
- **Streamlined TEME ↔ ECEF/ITRF conversion** — Rev 2 added explicit treatment of the True-Equator-Mean-Equinox frame that SGP4 outputs in.
- **Differential-correction-friendly mode** — designed to integrate cleanly with downstream OD pipelines.
- **Bug history** — documents the changes inferred from operational observations and confirmed against subsequent DoD releases.

## Apsis relevance

- **REQ-INT-005** (SGP4 propagation for catalog objects) — this paper + its source code are *the* implementation reference. Apsis's build-vs-reuse table (subsystems §1) names "vallado/SGP4 reference implementation" specifically.
- **REQ-CAT-002** (50k-object catalog propagation): the Vallado code is the only public implementation that matches operational TLE-producing AFSPC code, so propagating TLEs through anything else risks systematic bias relative to the catalog-publishing authority.
- **TEME ↔ ECEF conversion** (subsystems §2): SGP4 returns position/velocity in TEME, an obscure intermediate frame. Vallado §VI gives the canonical recipe for TEME→PEF→ITRF→GCRS that Apsis must implement.
- **State conversions to/from "Brouwer-Lyddane mean elements"** (subsystems §2 last paragraph): TLEs *are* Brouwer-Lyddane mean elements; this paper documents the unpacking, including the "deep-space" SDP4 lunar-solar long-period perturbations needed for orbits with period > 225 minutes.

## Cross-references

- [[sources/brouwer-1959-artificial-satellite-theory]] + [[sources/lyddane-1963-small-eccentricity-inclination]] — the underlying analytical theory.
- [[sources/vallado-2008-sgp4-orbit-determination]] — companion paper on differential-correction OD with this code (next ingest).
- [[sources/hoots-roehrich-1980-spacetrack-report-3]] (in `docs/raw/specs/`) — the original 1980 STR#3 this paper updates.
- [[concepts/sgp4]] — concept page summarizing the algorithm family.
