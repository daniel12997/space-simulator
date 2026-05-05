---
type: source
title: "Modeling of nutation and precession: New nutation series for nonrigid Earth and insights into the Earth's interior"
raw_path: docs/raw/papers/mathews-2002-mhb2000-nutation.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Mathews, P. M.; Herring, T. A.; Buffett, B. A.]
publication_date: 2002-04
venue: "J. Geophysical Research 107(B4):2068"
doi: 10.1029/2001JB000390
---

# Mathews, Herring & Buffett (2002) — MHB2000 nutation model

The geophysical-theory derivation of the **MHB2000 nutation series**, adopted by IAU 2000 Resolution B1.6 as the **IAU 2000A nutation model** — the "/2000A" half of Apsis's "IAU 2006/2000A precession-nutation" specification (architecture §3 Foundation > Frames; subsystems §1.3). Companion to [[sources/capitaine-2003-iau2000-precession-p03]] which provides the IAU 2006 precession side of the same composed transformation.

## What it provides

- Analytical theory of nutation/wobble for a non-rigid Earth, including: anelasticity of the mantle, ocean tide effects, electromagnetic couplings between fluid outer core / solid inner core / mantle (FOC-CMB and SIC-ICB), and nonlinear forcing terms previously omitted (§§Introduction, 1).
- Best-fit values for **seven Earth parameters** estimated by least-squares against VLBI nutation observations: dynamic ellipticities of Earth and FOC, two complex electromagnetic coupling constants, and others.
- The **MHB2000 nutation series** itself — 678 luni-solar terms + 687 planetary terms, with nutation amplitudes accurate to ~0.2 mas (vs ~0.5 mas for the prior Wahr 1981 / IAU 1980 model that this superseded).
- A reduced **resonance-formula** approximation that reproduces the full series for almost all frequencies, with errata listed for the few frequencies where it doesn't.

## Apsis relevance

- **REQ-TIME-006** ("right-handed body-fixed frames using IAU 2006/2000A precession-nutation") — this paper is the IAU 2000A nutation half. The IAU 2006 precession half is [[sources/capitaine-2003-iau2000-precession-p03]].
- Cross-cited from [[concepts/precession-nutation]] (umbrella concept) and [[concepts/iau-2006-precession]] (precession side notes the nutation companion).
- SOFA implements the 678+687-term series via `iauNut00a` / `iauPnm06a`. Apsis delegates to SOFA at the boundary; this paper is the *why-we-trust-SOFA* paper.

## What's not load-bearing for Apsis

The geophysical-interpretation half of the paper (Earth interior structure, magnetic fields at the CMB and ICB, free-core-nutation resonance frequency) is not directly relevant to Apsis. The *output* — the nutation series and its amplitude accuracy — is.

## References

Cited extensively in [[sources/capitaine-2003-iau2000-precession-p03]] (their Eq. 10 gives the linear precession-rate corrections derived from MHB2000). Cross-cited from [[concepts/precession-nutation]].
