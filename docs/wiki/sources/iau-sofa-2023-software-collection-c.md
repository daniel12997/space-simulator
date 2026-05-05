---
type: source
title: "IAU SOFA Software Collection (C version, 2023)"
raw_path: docs/raw/specs/iau-sofa-2023-software-collection-c.zip
source_type: spec
reliability: authoritative
ingested: 2026-05-05
authors: [IAU SOFA Board]
publication_date: 2023-05
venue: "International Astronomical Union — Standards of Fundamental Astronomy software v18"
---

# IAU SOFA C software distribution (v18, 2023)

The **C source distribution** of the IAU SOFA library — the implementation primitives Apsis links for time-scale conversions, precession-nutation, polar motion, and Earth rotation per [[decisions/001-use-ceo-based-icrs-to-itrs|decision 001]]. This artifact is a ZIP archive of the canonical distribution from `iausofa.org`; the cookbook documents ([[sources/sofa-2023-earth-attitude-cookbook]], [[sources/sofa-2023-time-scale-cookbook]]) describe the API.

## What's in the archive

- `src/` — C source for ~200 SOFA functions (`iau<Name>` C API + Fortran-style sub-name aliases).
- `t_sofa_c.c` — comprehensive test program covering every public function.
- `LICENSE` / `README` — terms of use (permissive; attribution required, function names must be preserved if modified).
- `makefile` — builds `libsofa_c.a`; integrates cleanly into Apsis's CMake build.

## Function families (Apsis-relevant subset)

| Prefix | Family | Apsis use |
|---|---|---|
| `iauTai*`, `iauUtc*`, `iauUt1*`, `iauTt*`, `iauTcg*`, `iauTcb*`, `iauTdb*` | Time-scale conversions | [[concepts/time-scales]] |
| `iauDat` | Leap-second table | UTC↔TAI difference |
| `iauDtdb` | TDB - TT analytical correction | JPL DE ephemeris consumers |
| `iauPnm06a`, `iauPmat06`, `iauNum06a` | IAU 2006/2000A precession-nutation matrices | [[concepts/iau-2006-precession]] |
| `iauXys06a`, `iauC2ixys` | CIP X,Y,s + GCRS-to-CIRS | [[concepts/celestial-ephemeris-origin]] |
| `iauEra00` | Earth Rotation Angle | [[concepts/earth-rotation-angle]] |
| `iauPom00`, `iauSp00` | Polar motion + TIO locator | ICRS↔ITRS pipeline |
| `iauJd2cal`, `iauCal2jd`, `iauD2dtf`, `iauDtf2d` | Calendar / Julian-date conversions | Operator I/O |
| `iauAnp`, `iauAnpm` | Angle normalization to [0, 2π) and [-π, π) | Utility |
| `iauC2t06a` | Combined GCRS-to-ITRS one-call wrapper | Convenience |

## License notes (must comply)

- Distribute the complete unmodified collection or modify with a clearly different name.
- Preserve function-name conventions if forking.
- Cite the SOFA Board in derivative work.
- No commercial-use restrictions.

For Apsis: link the unmodified upstream library; do not fork or rename. Keep the version pin (`SOFA v18, 2023-05-31`) in build metadata for reproducibility.

## Apsis architecture

The SOFA C library is statically-linkable, thread-safe (no global mutable state in the public API except for the leap-second table which is initialized once), and has no external dependencies. Apsis builds it via the supplied makefile or a CMake wrapper.

Wrap with a thin Apsis C++ namespace (e.g., `apsis::sofa`) that:

- Converts SOFA's two-part Julian-date `(jd1, jd2)` to/from Apsis's canonical time representation.
- Wraps return-code error reporting in Apsis's exception/Result type.
- Provides RAII-style scoped guards for any state that needs cleanup.

## Apsis relevance

- **REQ-INT-001/002/003** (time scales, Earth attitude) — implementation primitives.
- **Subsystems §1** build-vs-reuse table — link SOFA, do not re-implement.

## Cross-references

- [[sources/sofa-2023-earth-attitude-cookbook]] — Earth-attitude API documentation.
- [[sources/sofa-2023-time-scale-cookbook]] — time-scale API documentation.
- [[sources/iers-conventions-2010]] — the underlying spec that SOFA realizes.
- [[decisions/001-use-ceo-based-icrs-to-itrs]] — Apsis decision implemented via SOFA.
