---
type: source
title: "SOFA Tools for Earth Attitude (cookbook)"
raw_path: docs/raw/specs/sofa-2023-earth-attitude-cookbook.pdf
source_type: spec
reliability: authoritative
ingested: 2026-05-04
authors: [IAU SOFA Board]
publication_date: 2023-05
venue: "International Astronomical Union — Standards of Fundamental Astronomy (software v18, document rev 1.7)"
---

# IAU SOFA (2023) — Earth-attitude cookbook

The **IAU-blessed recipe book** for implementing precession-nutation-Earth-rotation-polar-motion transformations using the **SOFA C library** functions. Walks through the equinox-based and CIO-based forms of every supported model (IAU 1976/1980/1982/1994, IAU 2000A, IAU 2006/2000A) with worked numerical examples (§5.2–5.6) and complete function-by-function specifications (§8). This is the **authoritative implementation reference** for [[decisions/001-use-ceo-based-icrs-to-itrs|decision 001]] (use CEO/CIO-based pipeline for ICRS↔ITRS).

## Why SOFA matters for Apsis

[[concepts/iau-2006-precession|IAU 2006]] precession + [[sources/mathews-2002-mhb2000-nutation|IAU 2000A]] nutation defines the official Earth-attitude model in physics terms; **SOFA defines it operationally** — the constants, the function signatures, the recommended call sequences. Apsis must produce numerical results bit-identical (or within documented tolerance) to SOFA's reference implementation to be conformance-validated against IERS Conventions 2010 and JPL precision-orbit tools.

The cookbook §5 walked examples are the **gold-standard test cases**: identical inputs to Apsis's transformation pipeline must produce SOFA's reference outputs to ~1 nrad precision. Any deviation indicates a bug.

## Contents map (matched to Apsis)

| Cookbook § | Topic | Apsis subsystem |
|---|---|---|
| §2 | Celestial coordinates: stellar directions, precession-nutation, evolution of reference systems | §2 (time/frames) |
| §2.6 | CIO and TIO | [[concepts/celestial-ephemeris-origin]] |
| §2.8 | Equinox vs CIO | [[decisions/001-use-ceo-based-icrs-to-itrs]] |
| §2.9 | Celestial-to-terrestrial transformation | §2 (time/frames) — the canonical recipe |
| §3 | SOFA Earth-attitude models — classical pre/nut, CIP X,Y, CIO locator s, polar motion, Earth rotation | §2 (time/frames) implementation reference |
| §4 | Current models — canonical basis, SOFA functions | implementation map |
| §5 | Worked examples (5 variants of the transformation) | conformance test data |
| §7 | Long-term precession | extended-time-arc support |
| §8 | Function specifications (iauAnp, iauBi00, iauBp00, iauBp06, ...) | C API reference for all SOFA functions |

## Recommended Apsis call sequence (CIO-based, §5.5-5.6)

For the CEO/CIO-based pipeline of [[decisions/001-use-ceo-based-icrs-to-itrs]]:

1. **Time scales**: build TT, UT1, TAI, UTC from inputs (UTC + ΔUT1 from EOP series).
2. **CIP X, Y**: `iauXys06a(TT)` returns the CIP unit vector components and the [[concepts/celestial-ephemeris-origin|CIO locator]] `s` consistent with IAU 2006/2000A.
3. **GCRS-to-CIRS**: `iauC2ixys(X, Y, s)` returns the rotation matrix.
4. **Earth rotation**: `iauEra00(UT1)` returns the [[concepts/earth-rotation-angle|ERA]].
5. **CIRS-to-TIRS**: rotate by `-ERA` about the CIP.
6. **Polar motion**: `iauPom00(xp, yp, sp)` returns the W matrix.
7. **TIRS-to-ITRS**: apply W.

Apsis supplies `xp, yp, ΔUT1` from the IERS Bulletin A / Bulletin B finals2000A.data feed.

## Apsis relevance

- **REQ-INT-001/002** (time scales, frame transformations) — SOFA functions are the implementation primitives.
- **REQ-INT-003** (consistency with IAU 2006/2000A models) — must match SOFA cookbook §5.5-5.6 to declared tolerance.
- **Architecture build-vs-reuse table** (subsystems §1) — Apsis should link the SOFA C library directly rather than re-implement; the SOFA license permits this with attribution.
- **REQ-OBS-***  validation: Apsis's frame-transformation conformance tests should reproduce the cookbook §5 numerical results bit-for-bit.
- **Long-term precession** (§7) — for historical-arc / mission-design scenarios beyond ±200 years from J2000, use `iauLtp00b` and friends.

## License + distribution

SOFA C source is in `docs/raw/specs/iau-sofa-2023-software-collection-c.zip` (v18, May 2023). License is permissive (use freely with attribution; original-name + version preservation rules). Apsis links the unmodified upstream library.

## Cross-references

- [[sources/sofa-2023-time-scale-cookbook]] — companion cookbook covering time-scale conversions (TAI/TT/UTC/UT1/TDB/TCG/TCB) (next ingest).
- [[sources/wallace-capitaine-2006-iau2006-procedures]] — the underlying procedures paper.
- [[sources/capitaine-wallace-2008-concise-cio]] — concise CIO formulations the SOFA `iauXys06a` implements.
- [[decisions/001-use-ceo-based-icrs-to-itrs]] — the Apsis decision this cookbook implements.
- IERS Conventions 2010 (in `docs/raw/specs/iers-conventions-2010/`) — the underlying spec SOFA realizes.
