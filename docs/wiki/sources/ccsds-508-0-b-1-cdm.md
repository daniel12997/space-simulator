---
type: source
title: "Conjunction Data Message — CCSDS 508.0-B-1"
raw_path: docs/raw/specs/ccsds-508-0-b-1-cdm-2021.pdf
source_type: spec
reliability: authoritative
ingested: 2026-05-04
authors: [CCSDS Conjunction Data Working Group]
publication_date: 2013-06
venue: "CCSDS Recommended Standard (Blue Book) 508.0-B-1, with Technical Corrigendum 2 dated October 2021"
---

# CCSDS 508.0-B-1 — Conjunction Data Message (CDM)

The **CCSDS Blue Book** specifying the **Conjunction Data Message (CDM)** format used by space-surveillance providers (USSPACECOM 18 SDS, ESA Space Debris Office, etc.) to disseminate close-approach warnings to satellite operators. CDMs are the standard ingest format for any modern conjunction-assessment / collision-avoidance pipeline. Apsis must consume CDMs (REQ-INT-007 / REQ-CAT-005) and may produce them in scenario-replay or simulation modes.

## What's in a CDM

The CDM bundles everything an operator needs for a [[sources/foster-estes-1992-jsc-25898-pc|Foster-method Pc]] computation and maneuver decision:

- **Header**: CDM identifier, originator, message-creation TCA, version.
- **Relative metadata**: TCA (Time of Closest Approach), miss distance, relative position/velocity in RTN frame at TCA, collision-probability value (if computed by originator).
- **Object 1 / Object 2 sections** (one per conjunction body): each contains:
  - Object designator (NORAD ID), name, international designator
  - Object-type metadata (active payload, debris, etc.)
  - State at TCA: position, velocity in inertial frame
  - Position covariance (6×6, in RTN frame typically)
  - Drag-related parameters: Cd, area, mass; SRP coefficient, area
  - Maneuverability indicator
  - Reference frame, time system

## Why a separate Blue Book

CDMs are **operationally critical** — wrong or ambiguous units, frames, or covariance ordering produce wrong Pc values which produce wrong maneuver decisions. The standard locks down:

- **Frame conventions**: explicit naming (RTN, EME2000, etc.) with no ambiguity.
- **Unit conventions**: SI throughout; degrees vs radians explicit.
- **Covariance ordering**: 6×6 lower-triangular by row (21 entries); meaning of each entry unambiguous.
- **Time conventions**: UTC ISO-8601 with `Z` suffix.

## File format

Two equivalent serializations are defined:
- **KVN (Key-Value Notation)** — human-readable text, line-oriented, deterministic.
- **XML** — schema-validated, machine-friendly, used by some operators.

Apsis must accept both as input and at least KVN as output.

## Apsis relevance

- **REQ-INT-007** (CCSDS message I/O) — CDM is the canonical conjunction format.
- **REQ-CAT-005** (Pc computation, maneuver decision): consumes the relative state + joint covariance + HBR fields directly. The originator-provided Pc field is a sanity check, not authoritative — Apsis re-computes Pc independently from the state + covariance for traceability.
- **Subsystems §6** (CA pipeline): CDM ingest is the first stage of the pipeline; downstream Pc + CAM planning depends on the parsed contents.
- **REQ-OBS-***  validation: Apsis should round-trip CDMs (parse, re-serialize) and compare for byte-equality on the KVN form to catch parser bugs.

## Items for human review

- Subsystems §6 should explicitly state that Apsis-computed Pc supersedes any originator-provided Pc value in the CDM (originator may have used different math). The originator value should still be displayed for cross-reference.
- HBR (Hard-Body Radius) is *not* a standard CDM field in B-1 — it's typically supplied separately (operator-set per spacecraft). Apsis must maintain a per-spacecraft HBR config alongside CDM ingest.

## Cross-references

- [[sources/foster-estes-1992-jsc-25898-pc]] — the Pc algorithm that consumes CDM fields.
- [[sources/newman-2022-cara-best-practices]] — operational context for CDM use at NASA CARA.
- [[sources/bombardelli-2015-collision-avoidance]] — CAM design that consumes the same fields.
- CCSDS 502.0-B-2 OEM (Orbit Ephemeris Message) — the format operators submit *to* USSPACECOM (not currently in corpus; mentioned for context).
