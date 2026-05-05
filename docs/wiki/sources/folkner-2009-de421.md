---
type: source
title: "The Planetary and Lunar Ephemeris DE 421"
raw_path: docs/raw/papers/folkner-2009-de421.pdf
source_type: technical-report
reliability: secondary-reputable
ingested: 2026-05-04
authors: [Folkner, William M.; Williams, James G.; Boggs, Dale H.]
publication_date: 2009-08
venue: "JPL IPN Progress Report 42-178"
note: "Predecessor to the current DE440 — see [[sources/park-2021-de440-de441]]"
---

# Folkner, Williams & Boggs (2009) — JPL DE421 ephemeris

A historical reference for the JPL planetary and lunar ephemeris methodology. **Apsis uses DE440 or later per REQ-ENV-001**; DE421 is documented here for context, since the methodology has been incrementally refined rather than rebuilt for each successor.

## Methodology highlights (largely unchanged for DE440)

- **Frame**: ephemeris axes oriented to the [[sources/charlot-2020-icrf3|ICRF]] (DE421 used the original ICRF; DE440 is referred to ICRF3) within ~1 mas via Mars-spacecraft VLBI ties (§II first paragraph).
- **Time scale**: TDB consistent with IAU 2006 GA Resolution B3 definition. Conversion to atomic time uses Fairhead/Bretagnon 1990-style series, accurate to navigation requirements.
- **Dynamics**: PPN n-body integration with γ = β = 1 (general relativity values). Sun J₂ = 2.0e-7. Includes Newtonian effects of 67 "major" + 276 "minor" asteroids (the asteroids that perturb Mars's orbit appreciably) iteratively (§II).
- **Sun position** is *derived* at each integration step to keep the solar system barycentre at the coordinate origin — not integrated independently.
- **Mass parameters** used (Table 1) — these GM values are the canonical inputs for any ephemeris-consistent dynamics in Apsis. Earth: 398600.436233 km³/s² (this paper); Sun: 132712440040.944 km³/s²; Earth-Moon system: 403503.236310 km³/s².

## Reported orbit accuracies (for DE421, ~2009)

| Body | Accuracy |
|---|---|
| Moon | submeter (LLR fit) |
| Venus, Earth, Mars | subkilometer |
| Mercury | several km |
| Jupiter, Saturn | tens of km |
| Uranus, Neptune, Pluto | not as well determined |

DE440 supersedes these by an order of magnitude in most cases; see [[sources/park-2021-de440-de441]] for the current numbers.

## Apsis relevance

- **REQ-ENV-001** (DE440 or later via SPICE SPK kernels) — DE440 is the current canonical choice; DE421 documents the methodology and provides historical context.
- The mass-parameter table is useful as a reference for any place Apsis hard-codes a GM value (force-model documentation, test fixtures) — though canonical values should come from the SPICE PCK kernel, not from a paper.
- **Ephemeris coverage**: DE421 spans 1900-2050. DE440 spans much longer (DE441 extends to ±15000 yr). For Apsis's expected mission durations (decades) DE440 is sufficient; for long-baseline studies DE441 would be the correct choice.
