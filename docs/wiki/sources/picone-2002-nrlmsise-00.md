---
type: source
title: "NRLMSISE-00 empirical model of the atmosphere: Statistical comparisons and scientific issues"
raw_path: docs/raw/papers/picone-2002-nrlmsise-00.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Picone, J. M.; Hedin, A. E.; Drob, D. P.; Aikin, A. C.]
publication_date: 2002-12
venue: "J. Geophysical Research 107(A12):1468"
doi: 10.1029/2002JA009430
---

# Picone et al. (2002) — NRLMSISE-00 empirical atmosphere model

The reference paper for **NRLMSISE-00**, the empirical thermospheric mass-density and composition model that Apsis uses by default per REQ-PHY-006 ("atmospheric drag using NRLMSISE-00 as the default density model"). Major upgrade of MSISE-90 with three new data sets: orbital-decay-derived total mass density (Jacchia and Barlier sets), incoherent-scatter-radar temperature 1981-1997, and Solar Maximum Mission O₂ number density.

## What's new vs MSISE-90 / Jacchia-70

- **Drag-derived total mass density** included for the first time in an MSIS model — closes the previous deficiency where MSIS systematically underestimated drag relative to Jacchia.
- **"Anomalous oxygen" component**: O⁺ and hot atomic oxygen contributions to total mass density at high altitudes (>500 km), high latitudes, summer, low solar activity. Eliminates the historical Jacchia-vs-MSIS disagreement of factor of 2 at 200 km in those conditions.
- **Updated temperature** based on continuous ISR data covering ~1.5 solar cycles.

## Inputs and outputs

NRLMSISE-00 takes:
- Date (year, day-of-year, UT)
- Geocentric altitude, latitude, longitude
- Local apparent solar time (derived from above)
- F10.7 daily, F10.7 81-day average (solar activity)
- Ap daily and 3-hour history (geomagnetic activity)

Returns:
- Total mass density `ρ` (kg/m³)
- Temperature (K) at altitude and exospheric T_∞
- Number densities of N₂, O₂, O, He, H, Ar, N, anomalous O

For Apsis's drag-force calculation (REQ-PHY-007) the total mass density is what's consumed; species composition is useful for high-fidelity panel-model drag at very high altitudes.

## Apsis relevance

- **REQ-PHY-006**: NRLMSISE-00 as default atmosphere model. ✓ this paper.
- **REQ-PHY-007** (relative velocity in rotating atmosphere frame): NRLMSISE-00 returns density at the geographic location; Apsis converts spacecraft velocity to atmosphere-relative by subtracting `ω_Earth × r`. This paper is silent on the velocity computation; the standard treatment is in Vallado / Montenbruck (paywalled, not yet in our corpus).
- **REQ-ENV-005, REQ-ENV-006** (solar/geomagnetic indices, time-varying space weather): F10.7 and Ap inputs come from CelesTrak / NOAA archives; this paper documents what the model expects.

## Implementation

NRLMSISE-00 is distributed as **C/Fortran reference code** (Picone group, freely available — `ccmc.gsfc.nasa.gov/modelweb/atmos/nrlmsise00.html`). Apsis links the reference C version directly via the architecture's build-vs-reuse table. There's also a Python port (`pymsis`) for prototyping.

## Alternative: JB2008

[[sources/bowman-2008-jb2008]] (next ingest) is the alternative density model called for by REQ-PHY-008. JB2008 is slightly better at high altitudes; NRLMSISE-00 is the conservative default for the full altitude range.

## Cross-references

- [[sources/bowman-2008-jb2008]] — alternative density model.
- [[sources/us-standard-atmosphere-1976]] is the simpler, static reference Apsis offers as a sanity-check / toy mode.
