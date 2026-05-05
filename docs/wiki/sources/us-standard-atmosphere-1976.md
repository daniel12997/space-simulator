---
type: source
title: "U.S. Standard Atmosphere, 1976"
raw_path: docs/raw/specs/us-standard-atmosphere-1976.pdf
source_type: spec
reliability: authoritative
ingested: 2026-05-05
authors: [Committee on Extension to the Standard Atmosphere (COESA), NOAA / NASA / USAF]
publication_date: 1976
venue: "NASA TM-X-74335 (NOAA-S/T 76-1562); supersedes US Std Atm 1962, US Std Atm Supplements 1966"
---

# US Standard Atmosphere (1976)

The **time-invariant, latitudinally-and-longitudinally-uniform reference atmosphere model** from sea level to 1000 km altitude, jointly specified by NOAA, NASA, and the USAF via the Committee on Extension to the Standard Atmosphere (COESA). Provides tabulated and analytic profiles of **temperature, pressure, density, mean molecular weight, and species number densities** vs geometric/geopotential altitude, all under "moderate solar activity" conditions.

For Apsis: **not a real-time atmosphere model** — for that use [[sources/picone-2002-nrlmsise-00|NRLMSISE-00]] (default) or [[sources/bowman-2008-jb2008|JB2008]] (alternative). USStd76 is used for:

1. **Sanity / toy mode** — quick approximate density without space-weather inputs.
2. **Cross-validation reference** — verify Apsis's atmosphere code by reproducing USStd76 tables for the standard solar/geomagnetic conditions.
3. **Below 86 km** — USStd76's lower-atmosphere model is well-mixed homosphere physics and is broadly accepted; thermospheric models (NRLMSISE-00, JB2008) extend up from ~120 km but don't deeply specialize for the homosphere.
4. **Backup / fallback** — when space-weather indices (F10.7, Ap, Dst) aren't available, USStd76 provides a deterministic deflate-to fallback density.

## Structure

The atmosphere is divided into:

- **Homosphere** (0–86 km): well-mixed, constant composition, 7 atmospheric layers with linear temperature gradients. Hydrostatic equation closed-form integrable.
- **Heterosphere** (86–1000 km): species diffuse separately by molecular mass; hydrostatic equilibrium per-species. Atomic O, He, H become the dominant species at increasing altitudes.

The heterosphere model assumes **F10.7 = 150 sfu, Ap = 4** (moderate solar activity). At those conditions the model matches mean thermospheric densities; at solar max or geomagnetic storms the actual density can be 5-10× higher than USStd76.

## Apsis relevance

- **REQ-PHY-006** alternative — Apsis offers USStd76 as a third atmosphere choice (NRLMSISE-00 default, JB2008 alternative, USStd76 toy/fallback). Subsystems §2.4 should specify the selection precedence.
- **REQ-OBS-***  validation — USStd76's published density tables are exact reference values that Apsis's atmosphere implementation can reproduce as a regression test.
- **Mission-design phase** when space-weather forecasts aren't yet committed — USStd76 provides a deterministic baseline.

## Items for human review

- Subsystems §2.4 should explicitly state when USStd76 vs NRLMSISE-00 vs JB2008 is the appropriate choice. Recommendation: real-time / mission-ops uses NRLMSISE-00 or JB2008 with current space weather; mission-design / quick-look uses USStd76; below 86 km use USStd76 (or a current weather-aware model for high-altitude balloons / re-entry trajectories).
- USStd76 has no nightside / dayside or storm response. Drag estimates for storm-day predictions must use NRLMSISE-00 or JB2008.

## Cross-references

- [[sources/picone-2002-nrlmsise-00]] — Apsis default; supersedes USStd76 for thermospheric drag at typical LEO altitudes.
- [[sources/bowman-2008-jb2008]] — Apsis alternative; better at high altitudes during high solar activity.
