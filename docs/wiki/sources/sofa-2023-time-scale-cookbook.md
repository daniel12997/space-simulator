---
type: source
title: "SOFA Time Scale and Calendar Tools (cookbook)"
raw_path: docs/raw/specs/sofa-2023-time-scale-cookbook.pdf
source_type: spec
reliability: authoritative
ingested: 2026-05-04
authors: [IAU SOFA Board]
publication_date: 2023-05
venue: "International Astronomical Union — Standards of Fundamental Astronomy (software v18, document rev 1.63)"
---

# IAU SOFA (2023) — Time-scale and calendar cookbook

The companion to the [[sources/sofa-2023-earth-attitude-cookbook|SOFA Earth-attitude cookbook]] — covers the **time-scale and calendar functions** in the SOFA C library. Defines the relationships between TAI, UTC, UT1, TT, TCG, TCB, TDB; specifies the [[concepts/time-scales|standard astronomical time scales]] Apsis must support per REQ-INT-001. Includes a **comprehensive worked example** (§4.4) converting UTC to every other supported time scale that Apsis must reproduce.

## The seven time scales

| Scale | Definition | Apsis use |
|---|---|---|
| **TAI** | International Atomic Time — physical realization (averaged H-masers) | Reference for atomic-second-counted scales |
| **UTC** | Coordinated Universal Time — TAI with leap seconds to track UT1 | Operator-facing; TLE epochs; CDM TCAs |
| **UT1** | Universal Time — actual Earth-rotation angle, derived from VLBI | Earth-rotation transformations (ERA) |
| **TT** | Terrestrial Time — TAI + 32.184 s, ideal Earth-bound atomic time | Precession/nutation polynomial argument |
| **TCG** | Geocentric Coordinate Time — relativistic time at geocenter | GCRS/CIRS dynamics in spacetime |
| **TCB** | Barycentric Coordinate Time — relativistic time at solar-system barycenter | BCRS dynamics, JPL DE ephemeris time |
| **TDB** | Barycentric Dynamical Time — TCB scaled to track TT mean | JPL DE ephemeris argument; planetary calculations |

Differences are critical:

- **TAI - UTC** = integer leap seconds (37 as of 2026; check IERS Bulletin C).
- **TT - TAI** = 32.184 s exactly (definitional offset).
- **TT - UT1** = ΔT — slowly varying (~70 s in 2026), depends on Earth rotation; supplied by IERS finals2000A.data.
- **TDB - TT** ≈ periodic ~1.6 ms with annual + monthly + planetary terms; computable analytically.
- **TCG - TT** and **TCB - TDB** are linear in time with definitional rates.

## Critical things Apsis must get right

1. **Leap seconds** (§3.5.1): UTC has discontinuities at leap-second insertions (typically 30 June or 31 Dec). Apsis must use the SOFA leap-second table (`iauDat`) which is **build-time data** — Apsis must update this table when new leap seconds are announced (~every few years). Ignoring leap seconds produces ~37 s errors in 2026.

2. **TDB ↔ TT** (§3.6.5): JPL ephemerides are tabulated in TDB; Earth-attitude functions consume TT. The conversion is `TDB - TT ≈ 0.001658 sin(g) + 0.000014 sin(2g) + ...` where g is the Earth's mean anomaly. SOFA's `iauDtdb` does this with the full FB2001 expansion.

3. **GCRS vs BCRS spacetime units** (§3.6.3): GCRS coordinates are in TCG-second units; BCRS in TCB-second units. The IAU 2000 resolution defined a "TT-rate" GCRS variant (rates scaled to match TT) which is what most engineering software actually uses. Apsis must document which it uses; mixing TCG-rate and TT-rate in the same code path is a common bug class.

4. **JD precision** (§2.3): A double-precision Julian Date provides ~10 µs resolution near J2000 — enough for most uses, but Apsis should use SOFA's "two-part JD" representation `(jd1, jd2)` for sub-µs precision. Many SOFA functions take this signature for that reason.

## Recommended Apsis time-scale architecture

Internal: store time as **(TAI seconds since J2000 TT epoch, two doubles)** for canonical precision; convert to/from other scales at boundaries.

API surface: take UTC (operator-facing), TAI (catalog-facing), or TT (analyst-facing) at boundaries; convert immediately.

Compute and cache: ΔUT1 from finals2000A.data (linear interpolation between bulletin entries is acceptable for most uses); TDB-TT analytically per call (fast).

## Apsis relevance

- **REQ-INT-001** (time-scale support: TAI, UTC, UT1, TT, TCG, TCB, TDB) — this cookbook defines all seven.
- **REQ-INT-002** (leap-second handling) — `iauDat` lookup is the canonical method; Apsis must keep the table current.
- **REQ-INT-003** (TDB ↔ TT for JPL ephemerides) — `iauDtdb` is the canonical recipe.
- **REQ-OBS-***  validation — cookbook §4.4 worked example is a conformance test case.

## Cross-references

- [[sources/sofa-2023-earth-attitude-cookbook]] — companion Earth-attitude cookbook.
- [[sources/folkner-2009-de421]] / [[sources/park-2021-de440-de441]] — JPL DE ephemerides that consume TDB.
- [[concepts/time-scales]] — Apsis concept page summarizing the time-scale graph.
- IERS Conventions 2010 chapters in `docs/raw/specs/iers-conventions-2010/` — underlying spec.
