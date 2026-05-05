---
type: source
title: "Small Eccentricities or Inclinations in the Brouwer Theory of the Artificial Satellite"
raw_path: docs/raw/papers/lyddane-1963-small-eccentricity-inclination.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Lyddane, R. H.]
publication_date: 1963-10
venue: "The Astronomical Journal 68(8):555-558"
---

# Lyddane (1963) — Small eccentricities or inclinations in Brouwer's theory

Companion paper to [[sources/brouwer-1959-artificial-satellite-theory]] that **eliminates the small-eccentricity and small-inclination singularities** of the Brouwer mean-element formulation by re-expressing the perturbation theory in **Poincaré canonical variables** instead of Brouwer's Delaunay variables. The combined Brouwer-Lyddane formulation is what SGP4 actually uses; "Brouwer-Lyddane mean elements" is the modern term for TLE-compatible state.

## The problem Lyddane solves

Brouwer's Delaunay variables `(L, G, H, l, g, h)` and the resulting expansions have **singularities at e = 0** (eccentricity zero — circular orbit) and **at i = 0** (inclination zero — equatorial orbit) because:

- `g` (argument of pericenter) is undefined when `e = 0`.
- `h` (longitude of ascending node) is undefined when `i = 0`.

Brouwer noted these and several investigators tried Taylor-expanding the Delaunay-variable perturbations in `e` or `i` — first-order terms are regular, higher-order terms are singular and must be ignored, which is unjustified (§Introduction). Smith (1961) tried a different approach but it is shown to be erroneous in §4.

## Lyddane's fix: Poincaré variables

Define new canonical variables (Eq. 2):
```
x₁ = L,                  y₁ = l + g + h
x₂ = [2(L-G)]^½ cos(g+h), y₂ = -[2(L-G)]^½ sin(g+h)
x₃ = [2(G-H)]^½ cos h,    y₃ = -[2(G-H)]^½ sin h
```

These have no singularity at `e = 0` (since `L = G` then) or at `i = 0` (since `G = H` then). The perturbation Hamiltonian written in `(x, y)` is regular everywhere except at retrograde-equatorial (`i = π`), which is handled by substituting `π - i` for `i`.

The transformation between primed and unprimed variables uses the same determining function `S` as Brouwer; only the variable-set is changed. **Brouwer's S₁ and S₁* are reused as-is** — the only computational change is the final transformation of variables (§3).

## Apsis relevance

- **Subsystems §2 last paragraph**: "Brouwer-Lyddane mean elements, TLE-compatible" — this paper is the Lyddane half of the pair.
- **REQ-INT-005** (SGP4 propagation) — SGP4's general-perturbations theory uses Brouwer-Lyddane mean elements throughout, particularly for the small-eccentricity LEO satellites that constitute most of the catalog.
- **State conversions** between Cartesian, Keplerian, modified equinoctial, and Brouwer-Lyddane mean elements (subsystems §2) — Lyddane's variables are essentially equivalent to **modified equinoctial elements** when restricted to two-body part. Modified equinoctial elements likewise avoid the e=0 / i=0 singularities and are commonly used in modern orbit propagators.

## Cross-references

- [[sources/brouwer-1959-artificial-satellite-theory]] — the foundational mean-element theory this extends.
- [[sources/vallado-2006-revisiting-spacetrack-3]] — modern SGP4 implementation built on Brouwer-Lyddane.
- [[sources/hoots-roehrich-1980-spacetrack-report-3]] (in `docs/raw/specs/`) — the original Spacetrack Report No. 3.
