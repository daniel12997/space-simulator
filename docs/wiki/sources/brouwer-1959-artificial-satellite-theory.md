---
type: source
title: "Solution of the Problem of Artificial Satellite Theory Without Drag"
raw_path: docs/raw/papers/brouwer-1959-artificial-satellite-theory.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Brouwer, Dirk]
publication_date: 1959-11
venue: "The Astronomical Journal 64(1274):378-396"
---

# Brouwer (1959) — Artificial Satellite Theory Without Drag

The seminal paper on **mean-element analytical satellite theory** — solves the J₂-perturbed two-body problem in canonical Delaunay variables (L, G, H, l, g, h) via von Zeipel's averaging method, separating short-period, long-period, and secular contributions. Foundational for the **Brouwer mean elements** used by SGP4 and for any subsequent mean-element propagator.

## Method

Hamiltonian for a J₂-perturbed two-body orbit (Eq. 2):
```
F = μ²/(2L²) + (μ⁴ k₂/L⁶) [(-½ + (3/2) H²/G²) (a/r)³ + (3/2 - 3/2 H²/G²) (a/r)³ cos(2g + 2f)]
```

with `k₂` the J₂ coefficient (Brouwer's notation; equals `J₂ R_E²` in modern terms).

A **canonical transformation** via determining function S(L', G', H', l, g, h) maps the original variables (L, G, H, l, g, h) to "primed" mean variables (L', G', H', l', g', h') in which the short-period l-dependence and one of the long-period dependences are absorbed (Eqs 7-8). The determining function is expanded in powers of the small parameter k₂:
```
S = S₀ + S₁ + S₂ + ...
```
and the mean Hamiltonian F* such that L', G', H' are constants (or linear functions of time) is obtained by matching orders.

## Three classes of perturbation isolated

- **Short-period terms** in the original Hamiltonian (frequencies of order *l*, the mean anomaly) — absorbed by S₁.
- **Long-period terms** (frequencies of order *g*, the argument of pericenter) — absorbed by S₂.
- **Secular terms** — give rise to constant rates of change in mean anomaly, argument of pericenter, and node.

Brouwer obtains the secular motions to **O(k₂²)** and the periodic terms to **O(k₂)**.

## Critical inclination

The series expansions diverge at the **critical inclination** `i = 63.43°` where `1 - 5 cos²i = 0`, since the long-period denominators vanish. Brouwer notes this explicitly. **Lyddane (1963)** ([[sources/lyddane-1963-small-eccentricity-inclination|next ingest]]) extends the theory to handle the related singularities at small eccentricity and small inclination via a different choice of variables.

## Apsis relevance

- **REQ-INT-005** (SGP4 propagation for TLE-defined objects) — SGP4 builds on Brouwer-Lyddane mean elements. Vallado et al. 2006 ([[sources/vallado-2006-revisiting-spacetrack-3]]) is the modern reference implementation; this paper is the foundational theory.
- **State conversions to/from "Brouwer-Lyddane mean elements"** (subsystems §2 last paragraph: "Brouwer-Lyddane mean elements, TLE-compatible") — this paper plus Lyddane 1963 are the canonical references.
- **Catalog propagation (REQ-CAT-002)** — every TLE in the catalog is, in effect, a set of Brouwer-Lyddane mean elements; SGP4 extracts secular and periodic contributions from them.
- **Analytical propagation as alternative to numerical** (subsystems §3, §8 cruise-time fidelity downshift) — for short-arc cruise of a Keplerian-like orbit, Brouwer's mean-element propagator is faster than numerical integration with no significant accuracy loss.

## Cross-references

- [[sources/lyddane-1963-small-eccentricity-inclination]] — companion paper extending to small e and small i.
- [[sources/vallado-2006-revisiting-spacetrack-3]] — the modern SGP4 reference, which uses Brouwer-Lyddane mean elements.
- [[sources/hoots-roehrich-1980-spacetrack-report-3]] (in `docs/raw/specs/`) — the original Spacetrack Report No. 3 documenting SGP4.
