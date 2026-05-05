---
type: source
title: "Spacetrack Report No. 3: Models for Propagation of NORAD Element Sets"
raw_path: docs/raw/specs/hoots-roehrich-1980-spacetrack-report-3.pdf
source_type: spec
reliability: authoritative
ingested: 2026-05-04
authors: [Hoots, Felix R.; Roehrich, Ronald L.; Kelso, T. S. (compilation editor)]
publication_date: 1980-12
venue: "Project Spacetrack Report, Office of Astrodynamics, Aerospace Defense Center / NORAD (Kelso compilation 31 December 1988)"
---

# Hoots & Roehrich (1980) — Spacetrack Report No. 3 (STR#3)

The **original public NORAD specification** for the SGP/SGP4/SDP4/SGP8/SDP8 satellite propagators. Provides equations + FORTRAN IV source code for **five compatible models** that consume two-line element sets (TLEs). For decades this was the only public SGP4 reference; [[sources/vallado-2006-revisiting-spacetrack-3|Vallado et al. 2006]] supersedes it for modern use because the operational AFSPC code drifted from STR#3 in ways never publicly released, but **STR#3 remains historically authoritative** and is still cited as the spec for the TLE format and propagator family.

## Five propagation models (Contents §2)

| Model | Domain | Section |
|---|---|---|
| **SGP** | Original simplified general perturbations (Lane 1965) | §5 |
| **SGP4** | Refined SGP, near-Earth (period < 225 min) | §6 |
| **SDP4** | Deep-space variant of SGP4 (period ≥ 225 min) | §7 |
| **SGP8** | Higher-accuracy near-Earth (drag-modeling improvements) | §8 |
| **SDP8** | Deep-space variant of SGP8 | §9 |

Plus §10 deep-space subroutine (shared by SDP4 and SDP8 for lunisolar long-period and resonance terms).

The TLE-producing AFSPC operational code uses **SGP4/SDP4** merged into one propagator that switches based on orbital period. SGP8/SDP8 were proposed alternatives that never displaced SGP4/SDP4 operationally — Apsis can ignore them.

## What's in §11-13 — implementation

- §11: Driver and function subroutines (FORTRAN IV).
- §12: Users guide, constants, symbols.
- §13: Sample test cases — these test cases plus their reference outputs are the **conformance suite** for any SGP4 implementation. Vallado et al. 2006 expands and modernizes them; this section is the historical baseline.

## Important warnings

- **WGS-72 constants** — STR#3 uses WGS-72 fundamental constants (Earth radius, J₂, GM). Modern SGP4 implementations sometimes switch to WGS-84 constants which causes subtle systematic differences — Apsis must use **WGS-72 specifically** for SGP4 to match operational AFSPC predictions.
- **Brouwer-Lyddane mean elements**, not osculating — converting an osculating state vector to TLE without going through the inverse Brouwer-Lyddane transformation produces wrong predictions. See [[sources/brouwer-1959-artificial-satellite-theory]] + [[sources/lyddane-1963-small-eccentricity-inclination]].
- **TEME output frame** — SGP4 returns position/velocity in True Equator Mean Equinox frame, *not* GCRS, *not* ITRS. See [[concepts/sgp4]] for the conversion pipeline.
- **B*** drag coefficient is a **fitted empirical parameter in units of 1/Earth-radii**, not a physical drag coefficient. Don't confuse with `Cd · A/m`.

## Apsis relevance

- **REQ-INT-005** (SGP4 propagation) — STR#3 is the historical spec; [[sources/vallado-2006-revisiting-spacetrack-3|Vallado 2006]] is the modern implementation. Apsis links Vallado but cites STR#3 as the originating spec.
- **TLE format**: STR#3 documents the canonical two-line element set format (also covered by the Kelso CelesTrak references in `docs/raw/specs/kelso-celestrak-tle-format.html`).
- **Conformance testing**: STR#3 §13 sample test cases must pass; Vallado expands the test suite.
- **Historical validation**: when comparing against legacy software (e.g., decades-old TLE-tracking codes), STR#3 is the spec they conformed to.

## Cross-references

- [[sources/vallado-2006-revisiting-spacetrack-3]] — modern reference implementation that supersedes the FORTRAN IV here.
- [[sources/vallado-2008-sgp4-orbit-determination]] — companion DC fitter.
- [[concepts/sgp4]] — algorithm-family overview.
- [[sources/brouwer-1959-artificial-satellite-theory]] / [[sources/lyddane-1963-small-eccentricity-inclination]] — underlying mean-element theory.
- [[sources/kelso-celestrak-tle-format]] — Kelso's plain-language TLE format docs.
