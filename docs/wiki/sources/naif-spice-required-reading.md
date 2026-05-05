---
type: source
title: "NAIF SPICE Required Reading documents"
raw_path: docs/raw/specs/naif-spice-required-reading/
source_type: spec
reliability: authoritative
ingested: 2026-05-04
authors: [NASA Navigation and Ancillary Information Facility (NAIF), JPL]
publication_date: 2024
venue: "NAIF SPICE Toolkit documentation collection"
---

# NAIF SPICE Required Reading

The **NAIF SPICE Toolkit "Required Reading" documents** — the canonical specifications for **SPICE kernels** (SPK ephemerides, CK attitude, FK frames, PCK planetary constants, SCLK spacecraft clock, EK events) and the SPICE library API. Apsis interfaces with SPICE in two directions: **consuming** SPICE kernels (planetary ephemerides, planetary orientation, spacecraft trajectories from existing missions) and optionally **producing** SPICE kernels for output.

The directory contains 28 HTML documents covering individual subsystems. The most load-bearing for Apsis are listed below.

## Most-relevant docs (mapped to Apsis)

| Doc | Topic | Apsis use |
|---|---|---|
| `spk.html` | **SPK** — position-velocity ephemerides | Planetary ephemerides, spacecraft truth trajectories |
| `ck.html` | **CK** — attitude (orientation) data | Spacecraft attitude history kernels (input/output) |
| `pck.html` | **PCK** — planetary constants (radii, GM, orientation) | Planet/satellite parameters; cites [[sources/archinal-2018-iau-wgccre-2015\|IAU WGCCRE]] |
| `frames.html` | **FK** — reference-frame definitions | Custom mission frames (LVLH, body, instrument) |
| `time.html` | Time scales in SPICE | TDB / TT / UTC / SCLK conversions |
| `sclk.html` | **SCLK** — spacecraft clock kernels | Mapping SCLK ticks to UTC/TDB |
| `naif_ids.html` | NAIF body / object ID conventions | -1 (Voyager 2), 399 (Earth), 301 (Moon), etc. |
| `kernel.html` | Kernel architecture overview | Cross-cutting kernel-management background |
| `rotation.html` | Rotation conventions | Body-fixed frame rotations |
| `cspice.html` | C language interface | API surface for Apsis's SPICE wrapper |
| `error.html` | SPICE error handling | Exception model Apsis must adapt to its native model |
| `gf.html` | Geometry Finder (event search) | Eclipse, occultation, contact-window finding |
| `abcorr.html` | Aberration corrections (light-time, stellar) | Planetary observations, deep-space tracking |

The remaining docs (cells, das, dla, dsk, ek, ellipses, planes, problems, scanning, sets, spc, symbols, windows, daf) are SPICE-internal data-structure or utility specs that Apsis consumes implicitly via the SPICE library; not required reading unless implementing a SPICE-format file from scratch.

## Why Apsis uses SPICE rather than re-rolling

- **Planetary ephemerides** — the JPL DE series ([[sources/folkner-2009-de421|DE421]], [[sources/park-2021-de440-de441|DE440/441]]) is *distributed* as SPICE SPK kernels. Apsis loads them via the SPICE toolkit rather than parsing the binary format directly.
- **Planet orientation** — IAU WGCCRE polynomial models (e.g., Mars rotation) are distributed as SPICE PCK kernels.
- **Spacecraft truth data** — every mission with a SPICE bundle (Voyager, Cassini, MER, MSL, MRO, Juno, OSIRIS-REx, Europa Clipper, etc.) provides its trajectory as SPK kernels Apsis can consume directly.
- **Frame chains** — SPICE handles arbitrarily-deep frame trees (J2000 ↔ EME2000 ↔ planet ↔ planet-body-fixed ↔ spacecraft-bus ↔ instrument) automatically. Re-implementing this is a major engineering project.

## Apsis architecture

Apsis links the **CSPICE** library (C interface) and exposes a thin C++ wrapper. Kernels are loaded at scenario start (LSK, SPK, PCK, FK, SCLK as needed) and queried per simulation tick. The SPICE library is thread-unsafe — Apsis must serialize access from worker threads or use one CSPICE instance per thread.

## Apsis relevance

- **REQ-INT-005** (planetary ephemeris consumption) — SPK kernels via SPICE.
- **REQ-INT-006** (planet orientation consumption) — PCK kernels via SPICE; cites [[sources/archinal-2018-iau-wgccre-2015|Archinal 2018]] for the underlying polynomial models.
- **REQ-INT-007** (interop with JPL/NASA mission data) — SPICE kernel ingest is the canonical path.
- **REQ-OBS-***  geometry queries: SPICE Geometry Finder (`gf.html`) for eclipse, occultation, contact-window predictions.
- **Subsystems §1** build-vs-reuse table — Apsis links CSPICE; should not re-implement.

## Items for human review

- CSPICE thread-safety — subsystems §1 / §11 (parallel scaling) must specify the serialization or per-thread-instance approach. This is a constraint on multi-threaded Monte Carlo execution (REQ-MC-001).
- SPICE kernel-pool memory: each loaded kernel set occupies kernel-pool memory; long-running Monte Carlo with many ephemeris kernels can exhaust the default pool. Apsis must size the kernel pool explicitly.

## Cross-references

- [[sources/folkner-2009-de421]] / [[sources/park-2021-de440-de441]] — JPL DE ephemerides distributed as SPK kernels.
- [[sources/archinal-2018-iau-wgccre-2015]] — IAU planetary orientation polynomials distributed as PCK kernels.
- [[concepts/time-scales]] — SPICE time-scale conversion API.
- [[sources/sofa-2023-time-scale-cookbook]] — IAU SOFA time-scale spec; SPICE implements equivalent conversions.
