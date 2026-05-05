---
type: source
title: "CelesTrak TLE and GP Data Formats (Kelso)"
raw_path: docs/raw/specs/kelso-celestrak-tle-format.html
source_type: spec
reliability: authoritative
ingested: 2026-05-05
authors: [Kelso, T. S. (CelesTrak)]
publication_date: 2024
venue: "celestrak.org documentation pages"
---

# Kelso / CelesTrak — TLE and GP data formats

T. S. Kelso's **CelesTrak** documentation pages describing the **two-line element set (TLE) format** and the modern **General-Perturbations (GP) data formats** (XML, KVN, CSV) that USSPACECOM has been migrating to as TLE successors. CelesTrak is the de facto public mirror for Space Surveillance Network catalog data and the most-cited operational reference for the TLE format. The corpus contains two HTML snapshots:

- `kelso-celestrak-tle-format.html` — classic 80-character × 2-line TLE format reference.
- `kelso-celestrak-gp-data-formats.html` — modern GP-data formats covering the same content in machine-friendly serializations.

## Classic TLE format

The 80-column-fixed-width two-line format defined in [[sources/hoots-roehrich-1980-spacetrack-report-3|Spacetrack Report No. 3]] §12, but Kelso's CelesTrak documentation is the most cited *operational* reference for it because it documents:

- **Field widths and column positions** with the same semantics most parsers actually implement.
- **Line-1 fields**: NORAD ID, classification, international designator, epoch (year + day), mean motion 1st/2nd derivatives, B*, ephemeris type, element-set number, line-1 checksum.
- **Line-2 fields**: NORAD ID (repeated for sanity), inclination, RAAN, eccentricity (no decimal point — implied), argument of perigee, mean anomaly, mean motion (revs/day), revolution number at epoch, line-2 checksum.
- **Checksum algorithm** — modulo-10 sum of digits (with `-` counting as 1, all other characters 0). Critical for validating ingest.
- **Implicit decimal points** in `B*` and eccentricity (commonly mis-parsed).
- **Two-digit year handling** (00-56 → 2000-2056; 57-99 → 1957-1999).

These quirks are the source of most TLE-parser bugs. Apsis must handle them all to claim REQ-INT-005 compliance.

## Modern GP-data formats

USSPACECOM has been migrating to **CCSDS-aligned GP data formats** that carry the same Brouwer-Lyddane mean-element content as a TLE plus richer metadata:

- **OMM (Orbit Mean Elements Message)** — CCSDS 502.0-B-2. Superset of TLE; includes uncertainty / covariance fields, multiple element-set theories, additional metadata.
- **GP-XML / GP-KVN / GP-CSV** — Kelso's CelesTrak alternative serializations of the same content for download.

For Apsis: ingest both classic TLE and modern OMM / GP formats; produce them by serializing the internal Brouwer-Lyddane mean-element representation.

## Apsis relevance

- **REQ-INT-005** (SGP4 propagation) — these formats are how Apsis ingests TLE/GP data into the SGP4 propagator.
- **REQ-CAT-001** (catalog ingest) — bulk ingest of CelesTrak / Space-Track GP data files for the 50k-object catalog.
- **REQ-INT-007** (CCSDS message I/O) — OMM is the CCSDS-aligned format Apsis must support alongside the classic TLE.
- **Subsystems §1** build-vs-reuse table — TLE / OMM parsers should be small custom code (the format is fully specified here); not worth a third-party library dependency.

## Items for human review

- TLE checksum failures should be **logged but not necessarily fatal** — many real-world catalog feeds have occasional bad-checksum lines that operational consumers are expected to flag-and-pass-through. Apsis should make this configurable.
- The two-digit year wraparound is a **Y2056 issue** — Apsis should switch to OMM (which uses 4-digit years) before then to avoid ambiguity. Document the policy in subsystems §1.

## Cross-references

- [[sources/hoots-roehrich-1980-spacetrack-report-3]] — original TLE format spec.
- [[sources/vallado-2006-revisiting-spacetrack-3]] — modern SGP4 implementation that consumes these formats.
- [[concepts/sgp4]] — propagator family that consumes these formats.
- [[sources/ccsds-508-0-b-1-cdm]] — CCSDS CDM, sister format for conjunction data (CCSDS 508 vs 502).
