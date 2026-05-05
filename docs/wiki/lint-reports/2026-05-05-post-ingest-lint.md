---
type: lint-report
date: 2026-05-05
operator: agent
scope: full
---

# Lint report — 2026-05-05 (post-ingest)

First lint pass after completing the bibliography-pipeline ingest of all 50 raw artifacts (32 papers, 17 specs, 1 article). Run after the ingest reached steady state.

## Findings (resolved this pass)

### 1. Backslash-escaped pipes in wikilink-with-alias syntax (8 sites)

Pattern: a backslash-escaped pipe inside the alias separator caused the parser to treat the target as ending in a backslash and the alias as missing, producing a broken link. Source of 7 of the 18 broken-link findings. (The bad pattern itself is not reproduced inline here to avoid retriggering the lint check on the report describing it.)

**Affected pages:** `concepts/kalman-filter`, `concepts/time-scales`, `sources/iers-conventions-2010` (×4), `sources/likins-1970-flexible-space-vehicles`, `sources/naif-spice-required-reading`, `sources/urdf-xacro-pinocchio-docs`.

**Fix:** global `sed -i 's/\|/|/g'` across `docs/wiki/`. Resolved.

### 2. Undefined `composite-rigid-body-algorithm` concept (6 sites)

Pattern: 6 pages referenced `[[concepts/composite-rigid-body-algorithm]]` (the third Featherstone algorithm alongside ABA and RNEA), but no such concept page existed.

**Affected pages:** `concepts/analytical-rbd-derivatives`, `concepts/articulated-body-algorithm`, `concepts/pinocchio-library`, `concepts/recursive-newton-euler-algorithm`, `concepts/spatial-algebra`, `sources/carpentier-2018-rbd-analytical-derivatives`, `sources/carpentier-2019-pinocchio` (×2).

**Fix:** CRBA meets the 2-or-more cross-citations threshold for concept-page creation, so created `concepts/composite-rigid-body-algorithm`. Resolved.

### 3. Six orphan source pages from late-stage spec ingest

Source pages that had no inbound wikilinks (cited only in plain-text references):

- `sources/dicairano-2012-mpc-rendezvous`
- `sources/iau-sofa-2023-software-collection-c`
- `sources/igrf14-2024-coefficients`
- `sources/krisko-2019-ordem-3-1-user-guide`
- `sources/urdf-xacro-pinocchio-docs`
- `sources/us-standard-atmosphere-1976`

**Fix:** added inbound wikilinks from the canonical sister pages:
- `dicairano` + `krisko` ← `sources/newman-2022-cara-best-practices`
- `iau-sofa-c` ← `sources/sofa-2023-earth-attitude-cookbook`
- `igrf14` ← `concepts/spherical-harmonic-geopotential`
- `urdf-xacro-pinocchio-docs` ← `concepts/pinocchio-library`
- `us-standard-atmosphere-1976` ← `sources/picone-2002-nrlmsise-00`, `sources/bowman-2008-jb2008`

Resolved.

## Final state

- **72 pages** (47 sources, 24 concepts, 1 decision, 0 components, 0 synthesis, 0 queries).
- **0 broken wikilinks.**
- **0 orphans.**
- All 50 raw artifacts in `docs/raw/` are represented by at least one source page (some bundled under umbrella pages — IERS Conventions covers 11 chapter PDFs; NAIF SPICE covers 28 HTMLs; URDF/Xacro/Pinocchio covers 4 HTMLs; Kelso covers 2 HTMLs).

## Outstanding non-lint items (deferred)

These are surfaced from the ingest log but are *not* wiki lint findings — they are pending decisions or substantive work for the human:

1. **Decision 001 still in `proposed` status** — the CEO-based ICRS↔ITRS pipeline decision has never been accepted. Either accept (mark `accepted`) or actively supersede.
2. **22 spec-edit suggestions** logged under `## Items for human review` markers across log entries — substantive changes to `docs/01-architecture.md`, `docs/02-subsystems.md`, or `docs/REQUIREMENTS.md` that need human authorship per the CLAUDE.md scope guard.
3. **No component pages yet** — the wiki currently has zero entries under `components/` because no source code exists yet. Once Apsis source ships, components-side ingest begins.
4. **No synthesis pages yet** — the corpus is now broad enough to support cross-cutting synthesis pages (e.g., "MEKF vs USQUE for Apsis", "force-model fidelity tiers", "IAU 2006/2000A pipeline end-to-end"). Worth proposing once the human signals interest.
5. **Paywalled-gravity references (Pines 1973, Cunningham 1970)** are referenced from `concepts/spherical-harmonic-geopotential` but not in the corpus. Either ingest if accessible or remove the reference.
