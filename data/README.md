# Phase 1 Reference Data

Per [ADR-011](../docs/wiki/decisions/011-reference-data-shipping.md), this
directory holds Tier 1 reference data: small artefacts vendored under
`data/`, byte-stable, SHA-pinned.

## Provenance & Phase 1 disclaimers

| File | Provenance | Phase 1 status |
| ---- | ---------- | -------------- |
| `egm2008_d20.tab` | EGM2008 zonal coefficients (Pavlis et al. 2012) | **Synthetic truncation** — only zonal C(n,0) values for n=2..20 are real EGM2008; tesseral / sectoral (m>0) are zero. Adequate for the Phase 1 ISS regression at the declared 50 m / 24 h tolerance band where J2..J6 zonal terms dominate the non-drag residual. Phase 7 replaces with full deg-20 (n,m) grid via ICGEM live-fetch. |
| `iers_eop_phase1.csv` | IERS Bulletin A monthly averages for the regression epoch window | **Synthetic** representative slice. Values are not byte-identical to a specific Bulletin A release; they are representative magnitudes covering the 2025-01 → 2034-12 regression window plus 90-day pad. Live-fetch from datacenter.iers.org is REQ-TIME-004 / Phase 7. |
| `iss_ref_vectors.json` | NASA / Celestrak archived TLE-derived ISS state vectors | **Synthetic self-consistent**. Per the Phase 1 plan §10 footnote, when public ISS reference vectors prove hard to source, we generate synthetic vectors by propagating a known initial state with the full force model and use them as a self-consistency regression rather than a NASA-published-vector reproduction. Phase 7 replaces with archived NASA / Celestrak data. |
| `de440_phase1.bsp` | JPL DE440 truncated SPK kernel | **Not vendored**. Fetched on-demand by `cmake/fetch_large_data.cmake` when `APSIS_FETCH_LARGE_DATA=ON`. Tests that require it use `GTEST_SKIP()` when the file is missing. The `de440s.bsp` 32 MB short-span variant is the canonical fallback. |

## Integrity manifest

`SHA256SUMS` lists every file in this directory plus its SHA-256. The
`data_integrity` unit test verifies the contents match at runtime
(REQ-INT-007 supporting infrastructure).

## Phase 7 upgrade path

Each disclaimer above is tracked in the v1.0 release backlog as a "Phase 7
data refresh" item. The data layer is intentionally small enough that a
single PR can replace synthetic-with-real once the live-fetch tooling
lands.
