---
type: source
title: "Concise CIO based precession-nutation formulations"
raw_path: docs/raw/papers/capitaine-wallace-2008-concise-cio.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Capitaine, N.; Wallace, P. T.]
publication_date: 2008-01
venue: "Astronomy & Astrophysics 478:277-284"
doi: 10.1051/0004-6361:20078811
---

# Capitaine & Wallace (2008) — Concise CIO precession-nutation formulations

Companion to [[sources/wallace-capitaine-2006-iau2006-procedures]] that develops **truncated** versions of the [[concepts/celestial-ephemeris-origin|CIO]]-based GCRS↔CIRS rotation matrix for applications where the full IAU 2006/2000A's microarcsecond precision is unnecessary. Three example models (Table 2) trade off accuracy against compute cost:

| Model | Coefficients | Frequencies | RMS accuracy (1995-2050) | Speed vs full |
|---|---|---|---|---|
| Reference | 4006 | 1309 | — | 1× |
| IAU 2000B (McCarthy & Luzum 2003) | 354 | 77 | 0.28 mas | 7.6× |
| CPN_b | 229 | 90 | 0.28 mas | 15.3× |
| CPN_c | 45 | 18 | 5.4 mas | 138× |
| CPN_d | 6 | 2 | 160 mas | 890× |

The biggest savings come from truncating the X, Y, and (s + XY/2) series; further gains from simplifying the matrix-element expressions and subsuming long-period effects into bias quantities.

## Apsis relevance

Apsis's primary use case is high-fidelity simulation, so the **full IAU 2006/2000A** model (via SOFA, [[decisions/001-use-ceo-based-icrs-to-itrs]]) is the canonical choice. But concise formulations are appropriate where lower precision is genuinely sufficient:

- **Catalog propagation (REQ-CAT-002, REQ-PERF-003)**: 50,000 SGP4-propagated objects evaluated each epoch. TLE position accuracy is hundreds of meters to kilometers; the CPN_c model (5 mas ≈ 100 m at GEO) is more than enough. Speedup of 138× over full model is a major win for the inner loop.
- **Conjunction broad-phase screening (REQ-CAT-007)**: spatial-hash voxel size is ~100 km; CPN_d is sufficient.
- **Active spacecraft (REQ-PERF-001)**: full model still used.

## Cross-references

- [[sources/wallace-capitaine-2006-iau2006-procedures]] — full-precision procedures.
- [[concepts/iau-2006-precession]], [[concepts/precession-nutation]], [[concepts/celestial-ephemeris-origin]].

## Surfaced for human review (no silent spec edits)

The architecture/subsystems docs treat the IAU 2006/2000A frame transformation as a single capability. **Tiered precision is appropriate**: full model for active spacecraft, concise model for catalog propagation. Worth one sentence in subsystems §1.4 noting that the transformation cost is configurable and the catalog tier should default to a concise (≥1 mas) formulation.
