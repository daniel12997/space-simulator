---
type: concept
canonical_name: Astronomical time scales
aliases: [time scales, TAI, UTC, UT1, TT, TCG, TCB, TDB]
---

# Astronomical time scales

The seven canonical time scales used in spaceflight dynamics, and the conversions between them. Apsis must support all seven per REQ-INT-001. The definitions and conversion functions are realized in the [[sources/sofa-2023-time-scale-cookbook|SOFA library]].

## The graph

```
        physical realization
              │
              ▼
            TAI ──── + leap seconds ────► UTC   (operator-facing)
              │
              + 32.184 s
              │
              ▼
              TT ──── + ΔT (~70 s) ────► UT1   (Earth rotation)
              │ │
              │ + L_G·(t-T₀)
              │ │
              │ ▼
              │ TCG  (GCRS coordinate time)
              │
              + analytical periodic ~1.6 ms
              │
              ▼
              TDB ──── + L_B·(t-T₀) ────► TCB   (BCRS coordinate time)
              (JPL DE ephemeris arg)
```

Notes:
- **TAI - UTC** = integer leap seconds (37 in 2026); discontinuities at insertion epochs.
- **TT - TAI** = 32.184 s exactly (definitional).
- **TT - UT1** = ΔT (slowly varying); from IERS finals2000A.data.
- **TCG - TT** = L_G · (t - 1977-01-01 TAI) where L_G = 6.969290134e-10.
- **TCB - TDB** = L_B · (t - T₀) + small periodic; L_B = 1.550519768e-8.
- **TDB - TT** = ~1.6 ms periodic, dominated by Earth's annual orbit; computed via `iauDtdb`.

## Where each is used

| Scale | Used by Apsis for |
|---|---|
| **TAI** | Internal canonical time representation |
| **UTC** | Operator inputs, TLE epochs ([[concepts/sgp4|SGP4]]), CCSDS CDM/OEM messages |
| **UT1** | Earth rotation angle ([[concepts/earth-rotation-angle|ERA]]) — direct argument of `iauEra00` |
| **TT** | [[concepts/iau-2006-precession|Precession]], [[concepts/precession-nutation|nutation]], [[concepts/celestial-ephemeris-origin|CIO]] polynomials |
| **TCG** | Strict relativistic GCRS dynamics (rare; most engineering uses TT-scaled coordinates) |
| **TCB** | Strict BCRS dynamics |
| **TDB** | JPL DE ephemeris time argument ([[sources/folkner-2009-de421|DE421]], [[sources/park-2021-de440-de441|DE440/441]]) |

## Critical bug classes

- **Forgetting leap seconds**: 37-second error in 2026 if TAI ≡ UTC anywhere. Always use `iauDat`.
- **Confusing TT and TDB for ephemeris evaluation**: ~1.6 ms error → ~12 m position error for a LEO at 7.5 km/s.
- **Mixing TCG-rate and TT-rate GCRS**: subtle 0.7 ppm drift accumulates over decades; almost no engineering tools care, but flagged here for completeness.
- **Linear interpolation of ΔUT1 across leap-second epochs**: ΔUT1 has a 1-second discontinuity at every leap-second insertion. Always interpolate UT1-TAI, not UT1-UTC, across leap-second boundaries.
- **Single-double JD precision**: ~10 µs near J2000, ~ms for far-from-J2000 epochs. Use SOFA's two-part JD `(jd1, jd2)` for sub-µs precision.

## See also

- [[decisions/003-tagged-time-scale-types]] — why scale-mixing is a compile error.
- [[decisions/010-phantom-typed-time-and-state]] — how Apsis implements the tagging (phantom tag types over `Time<Scale>` templates).
- [[sources/sofa-2023-time-scale-cookbook]] — implementation cookbook.
- [[sources/sofa-2023-earth-attitude-cookbook]] — Earth-attitude transforms that consume time scales.
- [[concepts/earth-rotation-angle]] — UT1 → ERA function.
- IERS Conventions 2010 Ch. 5 (in `docs/raw/specs/iers-conventions-2010/`) — underlying spec.
