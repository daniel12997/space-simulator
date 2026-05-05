---
type: source
title: "14th Generation International Geomagnetic Reference Field (IGRF-14) — Schmidt semi-normalised spherical-harmonic coefficients"
raw_path: docs/raw/specs/igrf14-2024-coefficients.txt
source_type: spec
reliability: authoritative
ingested: 2026-05-05
authors: [IAGA Division V Working Group V-MOD]
publication_date: 2024
venue: "International Association of Geomagnetism and Aeronomy (IAGA), distributed via NOAA NCEI / BGS"
---

# IGRF-14 (2024) — International Geomagnetic Reference Field

The **14th generation IGRF**, released 2024 by IAGA Division V Working Group V-MOD. Provides **Schmidt semi-normalised spherical-harmonic Gauss coefficients** (`g_n^m`, `h_n^m`) for the Earth's main internal magnetic field from 1900 to 2030 in 5-year DGRF (Definitive Geomagnetic Reference Field) snapshots, plus a 5-year extrapolation rate-of-change (Secular Variation, SV) for 2025–2030. Apsis uses IGRF for any magnetic-field-dependent model: magnetic torque ([[concepts/farrenkopf-gyro-model|magnetorquer]] actuators, magnetic disturbance torque), magnetometer simulation, charged-particle environment along field lines.

## File format

The artifact is a single ASCII text file with header rows + coefficient rows:

```
# 14th Generation IGRF Schmidt semi-normalised spherical harmonic coefficients
c/s deg ord IGRF IGRF ... DGRF ... DGRF IGRF SV
g/h n m 1900.0 1905.0 ... 2020.0 2025.0 2025-30
g  1  0 -31543 -31464 ... -29403.41 -29350.0    12.6
g  1  1  -2298  -2298 ... ...
h  1  1   5922   5909 ... ...
...
```

- **g/h**: cosine ('g') or sine ('h') coefficient.
- **n, m**: spherical-harmonic degree and order (1 ≤ n ≤ 13 for IGRF; up to n=8 for SV).
- **Columns**: epoch year (1900.0 through 2025.0 as DGRF/IGRF; final column is 5-year SV in nT/yr).
- **Units**: nanoTesla (nT) for coefficients; nT/yr for SV.

To evaluate the field at year `t` between epochs:

```
coeff(t) = coeff(epoch_lower) + (t - epoch_lower) · linear_rate_to_epoch_upper

For 2025 ≤ t ≤ 2030: coeff(t) = coeff(2025.0) + (t - 2025.0) · SV_2025_30
```

Then the **scalar potential** is the standard Schmidt-normalized spherical-harmonic series:

```
V(r, θ, φ, t) = a · Σ_{n=1}^{13} (a/r)^{n+1} Σ_{m=0}^{n} [g_n^m(t) cos(mφ) + h_n^m(t) sin(mφ)] · P̄_n^m(cos θ)
```

with `a = 6371.2 km` (IGRF reference radius, *not* the Earth equatorial radius), `P̄_n^m` Schmidt-normalized associated Legendre function. **B = -∇V** in nT.

## Why IGRF rather than higher-degree models

For higher-precision applications (geophysical surveying, near-surface), the **World Magnetic Model (WMM)**, **EMM**, or **Enhanced Magnetic Model** go to higher degree (~720) and include crustal anomalies. For spaceflight at LEO altitudes and above, **the crustal field is largely smoothed by altitude** and IGRF's degree 13 is adequate. Apsis defaults to IGRF; users can override with WMM via the same coefficient-file interface.

## Apsis relevance

- **REQ-PHY-011** (Earth magnetic field model) — IGRF is the canonical reference. Subsystems §2.6 specifies "IGRF degree 13 main field model".
- **REQ-SC-008** (magnetorquer actuator simulation) — magnetic torque `τ = m × B` requires `B` from this model evaluated at the spacecraft position.
- **REQ-SEN-008** (magnetometer simulation) — measurement model is `B_meas = B(r,t) + bias + noise`; the truth `B(r,t)` is from this model.
- **REQ-SC-009** (magnetic disturbance torques) — residual spacecraft magnetic dipole interacting with `B` produces a disturbance torque this model quantifies.

## Items for human review

- Subsystems §2.6 should specify whether **degree 8 SV extrapolation** beyond 2030 is acceptable for forward-projecting scenarios, or whether Apsis must require the user to supply an updated IGRF generation. IGRF generations are released every 5 years; running a 2031+ scenario with IGRF-14 unconditionally produces increasingly stale fields.
- Apsis's IGRF implementation should be a **simple parser** of this text-file format; many Earth-magnetic-field libraries embed coefficients hard-coded, which makes upgrading to IGRF-15 a code change rather than a data change.

## Cross-references

- [[concepts/spherical-harmonic-geopotential]] — same mathematical structure (Schmidt semi-normalization differs from the geopotential's Kaula 4π normalization).
- The geopotential references ([[sources/pavlis-2012-egm2008]]) — analogous-format coefficient files for gravity vs magnetic.
