---
type: decision
title: "Reference data: small artefacts vendored, large artefacts fetched at configure time"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: [folkner-2009-de421, park-2021-de440-de441, pavlis-2012-egm2008, iers-conventions-2010]
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Phase 1 prerequisite; Phase 2 (full catalog) and
Phase 7 (validation campaign) revisit the policy as data volumes grow.

## Context

The simulator depends on external reference data:

- **JPL planetary ephemerides.** DE440 / DE441 SPK kernels
  ([[sources/park-2021-de440-de441]]) are 100 MB to 3 GB depending on
  span. Phase 1 regression cases need a narrow time window (a few years).
- **Earth Orientation Parameters.** IERS finals
  ([[sources/iers-conventions-2010]]); a few MB. Updated weekly upstream.
  Phase 1 regression cases use a frozen slice covering test epochs.
- **EGM2008 gravity coefficients.** Full set ([[sources/pavlis-2012-egm2008]])
  is ≈45 MB to degree 2190. Phase 1 needs degree 20 — a few KB.
- **ISS reference state vectors.** Public NASA-published; tens of KB.
- **Test inputs / golden outputs** for regression suites.

Repo-bloat versus reproducibility versus first-build friction must be
balanced.

## Decision

A two-tier policy:

- **Tier 1 — vendored under `data/`** (committed, byte-stable, small):
  - DE440 truncated kernel covering the Phase 1 regression-test epoch
    (`data/de440_phase1.bsp`, ≈10 MB).
  - EOP slice covering test epochs (`data/iers_eop_phase1.csv`).
  - EGM2008 coefficients truncated to degree-and-order 20
    (`data/egm2008_d20.tab`).
  - ISS reference vectors (`data/iss_ref_vectors.json`).
  - Synthetic golden outputs for unit tests.
  - SHA-256 manifest at `data/SHA256SUMS`; checked at test runtime.
- **Tier 2 — fetched at configure time, gated by
  `APSIS_FETCH_LARGE_DATA=ON`** (not committed):
  - Full DE441 (≈3 GB).
  - Full EGM2008 to degree 2190.
  - Live IERS finals.

Fetching uses CMake's `file(DOWNLOAD … EXPECTED_HASH SHA256=…)` — same
SHA-pinning discipline as the SOFA / CSPICE vendoring of
[[decisions/008-vendor-sofa-and-cspice]]. Cached under
`build/_deps/data/`.

Tests that depend on Tier 2 data are tagged `requires_large_data` and
skipped automatically when the option is OFF.

## Rationale

- Phase 1 must be runnable in CI on a clean machine without 3 GB of
  download. Vendoring the small slices that regression tests actually
  need keeps CI clean and deterministic.
- Larger data shouldn't bloat the repo. Tier 2 lets us validate against
  full DE441 in dedicated long-running CI runs without paying repo-size
  cost on every clone.
- SHA-pinning of fetched artefacts keeps reproducibility intact —
  upstream re-issuing a kernel doesn't silently change our test
  outcomes.
- Two tiers give a clean upgrade path: a regression case that grows
  beyond Tier 1 simply moves to Tier 2 without rearchitecting.

## Alternatives considered

- **All-vendor (commit everything).** Rejected; repo grows past 5 GB
  with full DE441. Clones become unwieldy.
- **All-fetch (commit nothing).** Rejected; every CI run depends on
  upstream availability. JPL NAIF and IERS occasionally have outages
  long enough to break PR CI. Fragile.
- **Git LFS.** Adds tooling friction (every clone needs LFS) and many CI
  providers have data caps on LFS bandwidth. Rejected.
- **Out-of-tree submodule with all data.** Equivalent to all-vendor with
  extra cloning ceremony. Rejected.

## Consequences

- `data/SHA256SUMS` is verified in a `data_integrity` unit test so that
  silent corruption (or accidental edit) of vendored data fails CI.
- A short `data/README.md` documents the provenance and licence terms
  for each tier-1 file (most are public-domain or open-licence; the
  Space-Track risk in the design overview applies only to TLE catalog
  data, which is Phase 2 and not in Tier 1).
- Tier-2 fetches go through a single `cmake/fetch_large_data.cmake`
  module so that bumping a kernel version is a one-file change.
- Phase 2 (catalog) revisits the policy for TLE distributions — those
  may need a third tier (user-provided, not vendored at all) given
  Space-Track's terms-of-service constraints.
