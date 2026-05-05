---
type: synthesis
title: "Development state of the Apsis design corpus — 2026-05-05"
date: 2026-05-05
---

# Development state of the Apsis design corpus — 2026-05-05

A snapshot of the Apsis **design corpus** at 2026-05-05: what artifacts exist, how they were produced, how they're organised, and how a reader navigates them. This is process / meta-documentation about the design materials themselves — the project-design content (what Apsis-the-system *is* and what its design decisions *are*) lives in `docs/00-design-overview.md` and the spec docs at `docs/` root.

## Overall state

**No source code exists yet.** What exists is a design corpus produced through deliberate research-then-deepening cycles:

| Artifact | Count / status |
|---|---|
| Authoritative spec documents at `docs/` root | 3 (REQUIREMENTS, architecture, subsystems) at v0.6, plus `00-design-overview.md` (alignment) |
| Wiki source pages at `docs/wiki/sources/` | 47 (every paper / spec / standard the design depends on, summarised) |
| Wiki concept pages at `docs/wiki/concepts/` | 30 (algorithms, theoretical primitives, architectural patterns) |
| Wiki ADRs at `docs/wiki/decisions/` | 5 (all accepted) |
| Wiki audit syntheses at `docs/wiki/synthesis/` | 4 audit reports (REQUIREMENTS / architecture / subsystems / summary) plus this page |
| Wiki lint reports | 1 (post-ingest, 2026-05-05) |
| Raw research artifacts at `docs/raw/` | 50 (32 papers, 17 specs, 1 article) |

Substantive design content is in the spec docs and the wiki. This corpus exists as a foundation for code; the next phase moves into implementation per the build plan in `01-architecture.md` §7.

## Deepenings landed

Per the `improve-codebase-architecture` skill, four design-deepening cycles consolidated previously-scattered spec fragments into named modules with explicit interfaces, conformance discipline, and load-bearing ADRs. Each ran the explore → present-candidates → grilling → write-deliverables flow.

| # | Deepening | Doc version | Concept page(s) | ADR | What was consolidated |
|---|---|---|---|---|---|
| 1 | Variational Equations | v0.2 → v0.3 | `variational-equations` | ADR-002 | Force-model partials contract + Φ propagation. Was 1 line of REQ-PHY-016 (S-priority); now a named subsystem with conformance harness, M-priority partials interface, between-measurement Φ propagation, named consumers (orbit EKF, Pc covariance). Resolved audit's HIGH finding F2.1. |
| 2 | Long-arc state conditioning | v0.3 → v0.4 | `long-arc-state-conditioning` | ADR-003 | Two-component time + Encke perturbation propagation + compensated summation. Three pillars under one architectural principle, each previously appearing as a scattered fragment across architecture §2 / a requirement / a subsystems section. Plus `Time` becomes type-tagged with its scale (compile-time scale-mixing prevention). |
| 3 | Attitude Estimator Family | v0.4 → v0.5 | `attitude-estimation-policy`, `usque` | ADR-004 | MEKF + USQUE + hybrid mode logic. Was a 1-line subsystems aside (USQUE mentioned in MEKF page only); now a manager-plus-policy module with documented switching triggers, direct covariance hand-off via shared MRP-flavoured GRP, mission-FSM override. |
| 4 | Conjunction Screening Pipeline | v0.5 → v0.6 | `conjunction-screening` | ADR-005 | 14 REQ-CAT IDs → one named pipeline with 4 phases (catalog state, propagation, pair reduction, per-pair assessment). Strategy-pluggable broad-phase. Pc method registry with validity predicates. CAM split out as future deepening. |

## Remaining deepening candidates

From the audit-derived candidate list:

| # | Title | Status |
|---|---|---|
| 4 | GNC Message Bus Contract | Premature without code — needs at least skeletal code to validate the contract. |
| 6 | Deterministic Monte Carlo Contract | Bounded; assemble-existing-pieces shape. |
| 7 | Floating-Base Coupling | Lowest-risk; extends existing `concepts/floating-base-dynamics` page with explicit invariants. |
| 8 | Spacecraft Fidelity Scope | ADR-shaped; not really a deepening. |
| (9) | CAM Planning | New candidate — downstream of Conjunction Screening; planned as the natural next deepening once code starts producing CA events to consume. |

## Audit history

The design corpus's structural soundness was first audited 2026-05-05 (`synthesis/audit-summary-2026-05-05`). 1 HIGH finding, 17 MEDIUM, 44 LOW. The HIGH finding (F2.1, REQ-PHY-016 priority) was resolved by Deepening 1; the MEDIUM clusters were addressed across Deepenings 1-4 (frame precision, tides + Pc, atmosphere indices, SGP4 spec, flex/slosh, MEKF robustness, CAM optimization, CMG terminology). LOW items mostly resolved as text fixes during the v0.1 → v0.2 audit-application pass.

The four audit synthesis pages remain as historical record of the v0.1 baseline; they accurately describe the state at the time they were written and are not updated as deepenings land. Future audits should be quick deltas against the current spec version.

## How the design corpus is organised

```
docs/
├── 00-design-overview.md      ← alignment artifact (~200 lines, hard-budget)
├── REQUIREMENTS.md            ← feature requirements with MoSCoW priorities
├── 01-architecture.md         ← high-level architecture
├── 02-subsystems.md           ← per-subsystem detailed design
├── raw/                       ← research corpus (papers, specs, articles)
└── wiki/
    ├── index.md               ← wiki catalog
    ├── log.md                 ← operation log (append-only)
    ├── sources/               ← per-source-paper summaries
    ├── concepts/              ← algorithms, primitives, architectural patterns
    ├── decisions/             ← ADRs
    ├── synthesis/             ← audits, cross-cutting analyses, this page
    └── lint-reports/          ← wiki-lint outputs
```

The three authoritative spec docs at `docs/` root are spec-level: substantive changes are deliberate human-or-jointly-authored edits. Wiki pages are the agent's writable territory under the wiki skills.

## Reading order

For someone joining or returning to the project:

1. **`CLAUDE.md`** — project schema and conventions (~15 min).
2. **`docs/00-design-overview.md`** — alignment / orientation (~10 min).
3. **`docs/REQUIREMENTS.md`** — feature requirements (~20 min).
4. **`docs/01-architecture.md`** — high-level architecture (~15 min).
5. **`docs/02-subsystems.md`** — detailed design (~30 min).
6. **`docs/wiki/index.md`** — wiki catalog; jumping-off point.
7. **Wiki concept pages** — drill into specific algorithms or patterns. The five "deep concept" pages from the deepenings (`variational-equations`, `long-arc-state-conditioning`, `attitude-estimation-policy`, `usque`, `conjunction-screening`) and their ADRs are the most load-bearing.
8. **Wiki source pages** — drill when a citation needs verification.
9. **`docs/wiki/synthesis/`** — audit reports + this page (development state).

For a contributor implementing one subsystem: CLAUDE.md → 00-design-overview → relevant subsystems §X → relevant concept page(s) → source pages when verifying. Skip what's not on path; the corpus is wide.

## How this page is maintained

This page is a snapshot. When the corpus state changes meaningfully — new deepening landed, new candidate surfaced, new ADR accepted, doc version bumped — either update this page in place (preserving the date in a "last updated" line) or write a successor page (e.g., `development-state-2026-08-15.md`) that supersedes this one. The successor pattern is appropriate when the previous snapshot is worth retaining as a historical record; in-place update is appropriate for incremental drift.

The design overview at `docs/00-design-overview.md` cross-references this page for navigation; that pointer should be kept current if a successor is written.
