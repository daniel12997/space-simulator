# Apsis — Design Overview

> **Purpose**: alignment checkpoint. Lowest-cost point for direction changes — read here before investing in detailed planning or code.
> **Status**: 2026-05-05. After four architecture-review deepenings; before code.
> **Audience**: anyone joining or returning to the project. Read CLAUDE.md first; this next; then drill into REQUIREMENTS / architecture / subsystems / wiki.

## What Apsis is

A **flight-dynamics-grade spaceflight simulator**. C++17 core, Python (pybind11) scenario layer. Targets four use cases simultaneously:

1. Single-spacecraft mission simulation at engineering fidelity (multi-body, full force model, closed-loop GNC, interplanetary).
2. GNC development — swappable controllers/estimators with continuous-discrete plant separation.
3. Monte Carlo verification — thousands of deterministic trials with statistical aggregation.
4. Catalog-scale conjunction screening — ~50,000 SGP4-propagated objects screened against active spacecraft.

Sits in the gap between game-engine physics (lacks long-arc precision) and research tools like MONTE / GMAT / Orekit (precise but not architected for swappable GNC + Monte Carlo + catalog work in one package).

## Current state (2026-05-05)

**No source code yet.** Design corpus:

- Three authoritative specs at `docs/` root (REQUIREMENTS, architecture, subsystems), all v0.6.
- Wiki at `docs/wiki/`: 47 source pages (research corpus), 30 concept pages, 5 ADRs, 4 audit syntheses.
- All 50 raw research artifacts ingested under `docs/raw/`.

Four deepenings (per the `improve-codebase-architecture` skill) have turned scattered spec fragments into named modules:

| Deepening | What it consolidated |
|---|---|
| **Variational Equations** (v0.3) | Force-model partials contract + Φ propagation — was 1 line of REQ-PHY-016 |
| **Long-arc state conditioning** (v0.4) | Two-component time + Encke + compensated summation under one principle |
| **Attitude Estimator Family** (v0.5) | MEKF + USQUE + hybrid mode logic — was a 1-line aside |
| **Conjunction Screening Pipeline** (v0.6) | 14 REQ-CAT IDs → one named pipeline with 4 phases |

Remaining candidates: GNC Message Bus (premature without code), Deterministic MC Contract (bounded), Floating-Base Coupling (low-risk), Spacecraft Fidelity Scope (ADR-shaped), CAM Planning (downstream of Conjunction Screening).

## Desired end state (v1.0)

Per REQUIREMENTS §18, v1.0 ships when **all M (Must) requirements** are implemented and tested, plus:

- Validation report reproduces ≥3 published reference cases within tolerance.
- Performance: 1000× real-time cruise on single core; 50k-object catalog <1 sec/epoch; <60 sec for 7-day Pc screen on 50k catalog.
- Example scenarios (LEO, GEO stationkeeping, lunar/Mars transfers, attitude slew, conjunction avoidance) all run end-to-end.
- 1k-trial Monte Carlo reference campaign produces correctly-shaped dispersion statistics.
- Conjunction screening reproduces a known historical close-approach event within published miss-distance tolerance.
- Documentation suite complete (Doxygen API, architecture, subsystem, user guide, validation report).
- CI passes on Linux + Windows.

Phased build plan (`01-architecture.md` §7, ~6-8 months sustained):

1. **Propagator core** (6-8 wk) — single spacecraft, force models with VE contract, integrators incl. Encke wrapper, SPICE.
2. **ECS world + catalog** (3-4 wk) — EnTT/flecs, conjunction-screening pipeline.
3. **Multi-body / URDF** (6-8 wk) — Pinocchio, floating-base coupling, effectors, flexible/slosh DOFs.
4. **GNC layer** (4-6 wk) — sensors, AttitudeEstimator manager, controllers, message bus.
5. **Monte Carlo** (3-4 wk) — snapshot/restore, RNG management, parallel trials, Parquet aggregation.
6. **Scenario layer** (3-4 wk) — pybind11, Python DSL, examples.
7. **Hardening** (ongoing) — higher-fidelity force models, advanced GNC, validation campaigns.

End of Phase 6 produces a usable simulator. Phases 1-2 alone (~10 weeks) suffice for trajectory studies and conjunction analysis without GNC.

## Design decisions

Five accepted ADRs (in `docs/wiki/decisions/`):

- **ADR-001** — ICRS↔ITRS uses the **CIO-based pipeline** (X, Y, s, ERA), not equinox. SOFA `iauC2t06a`.
- **ADR-002** — Variational equations propagated **between measurements**, not as augmented natural state.
- **ADR-003** — `Time` is **type-tagged with its scale**. Scale mixing is a compile-time error.
- **ADR-004** — Attitude estimation uses **hybrid mode logic** — boot-USQUE plus NIS-monitored MEKF.
- **ADR-005** — Broad-phase pair filtering is a **strategy interface**. Spatial-hash default, sort-and-sweep alternative.

Other load-bearing choices that aren't ADRs:

- **f64 throughout, no wider precision.** Robustness from frame partitioning, two-component time, Encke, compensated summation, structure-preserving integrators.
- **SPICE for ephemerides + frames; SOFA for time + IAU; Pinocchio for MBD; Vallado SGP4 (WGS-72) for catalog.** Build what doesn't exist; reuse what does.
- **No global mutable state.** RNGs, time, force-model state passed explicitly. Per-trial determinism is non-negotiable.

## Patterns to follow

**Deep modules with internal seams** (LANGUAGE.md vocabulary). Every deepening so far follows this:

- One outer module with a small public interface (e.g. `ConjunctionScreeningPipeline::screen`, `AttitudeEstimator::predict/update`).
- Multiple inner-stage modules behind the seam, each independently testable.
- Strategy interfaces only where ≥2 adapters are real (broad-phase, base integrator wrapped by Encke). One-adapter "seams" are just indirection.

**Vocabulary discipline.** Module / Interface / Depth / Seam / Adapter / Leverage / Locality used everywhere. Never "component" / "service" / "boundary."

**Conformance via the interface.** The Variational Equations harness — one CI gate over every registered force model checking analytical partials against finite differences — is the canonical pattern. New strategy-pluggable subsystems should follow.

**Frames are first-class.** Every state / position / velocity / attitude claim names its frame. ICRF and J2000 are distinct (frame bias). Frame transitions happen at precision-rich boundaries.

**Time scales are first-class.** TAI / TT / UTC / UT1 / TDB are not interchangeable — type-tagged per ADR-003. SOFA is the only conversion authority.

**Conservation invariants are validation.** Angular momentum for MBD; energy for symplectic gravity-only; mass for fueled craft. Every dynamics component names its validation hook.

**Tiered fidelity per entity.** Active spacecraft (full multi-body) ≠ catalog (SGP4) ≠ debris (J2-only). Don't conflate across concept / requirement / subsystem.

**ADR shape.** Real choice + non-obvious rejection of alternative + future-reader confusion ⇒ write an ADR. Otherwise, don't.

**Wiki citation discipline.** Every factual claim in the wiki traces to a source under `docs/raw/`. Inline `[[sources/<slug>]]` at the claim. `code:<path>:<line>` for code (when source exists).

**Project-specific (CLAUDE.md):** numerical conditioning > raw precision; determinism non-negotiable for Monte Carlo (no global RNG, no within-trial parallelism, no hash-iteration-order dependence); MC trials single-threaded; parallelism is across trials.

## Patterns from specific deepenings

- **Per-process service vs per-screen module.** `CatalogStore` lives outside the screening pipeline (process lifetime); `ConjunctionScreeningPipeline` is per-screen. Persistent state in services; transient pipelines query them.
- **Internal seams aren't external interfaces.** A deep module can have many testable internal seams without exposing them. Don't promote internal seams just because tests use them.
- **Pure-logic policy classes.** `AttitudeEstimationPolicy` takes synthetic inputs, produces decisions. No dependencies; trivially unit-testable. Use this shape wherever non-trivial decision logic appears.
- **Method registry with validity predicates.** Pc method registry (Foster + MC default; Patera optional) generalises: each method declares its own predicate; pipeline picks first-valid-in-order; selected method recorded in event for audit. Apply when adding alternative algorithms with overlapping regimes.

## What Apsis is explicitly NOT building (v1)

REQUIREMENTS §17 lists 13 OOS items. The most consequential:

- No real-time 3D visualisation (recorded telemetry is the v1 output).
- No mission-design optimisation (GMAT / PyKEP companion offline; outputs feed Apsis).
- No thermal / power / comms-budget modelling beyond geometric visibility.
- No re-entry aerothermo / hypersonic CFD.
- No FDIR controller logic (failure *injection* yes, response logic no).
- No contact-constrained MBD (capture, berthing, surface contact deferred).
- No sub-tracking-threshold MMOD flux (ORDEM ingested but not in v1 pipeline).
- No multi-panel high-fidelity drag/SRP.

## Risks and open questions

- **Performance budget viability.** REQ-PERF-004 (60 sec for 7-day 50k-catalog screening) is aggressive. Phase 1+2 implementation will reveal whether the strategy-pluggable broad-phase + Φ-based covariance roll-forward fit. Fallbacks: tighter pre-filter thresholds, parallelism within screening, sample-rate compromise.
- **Long-arc precision validated.** Two-component time + Encke + compensated summation achieves REQ-TIME-009 on paper; Phase 1's regression tests against published reference cases (JPL DE round-trip 10 yr; ISS state vectors) are the empirical check.
- **CSPICE thread-unsafety.** SPICE access must be serialised or use per-thread instances. Wire correctly in orchestration; surfaced in `[[wiki/sources/naif-spice-required-reading]]`.
- **Catalog-data redistribution licensing.** Space-Track terms-of-service constrain example-data shipping. Settle for the example scenarios (REQUIREMENTS Appendix B).
- **Cluster-scale Monte Carlo.** v1 targets local 32-core; cluster (MPI / Ray / Celery) is a v1.x extension.

## Reading order

CLAUDE.md → this file → REQUIREMENTS → 01-architecture → 02-subsystems → wiki/index. From wiki/index drill into specific concepts as needed. The five "deep concept" pages from the deepenings (`variational-equations`, `long-arc-state-conditioning`, `attitude-estimation-policy`, `usque`, `conjunction-screening`) and their ADRs are the most load-bearing. Source pages for citation verification only. Synthesis directory holds audits and cross-cutting analyses.

For a contributor implementing one subsystem: CLAUDE.md → this file → relevant subsystems §X → relevant concept page(s) → source pages when verifying. Skip what's not on path; the corpus is wide.

## How to extend this doc

**Budget: ~200 rendered lines at 100-char wrap.** This is a hard ceiling, not a target. When adding content, condense or remove existing material to stay under budget. Verify with `awk '{c+=int((length+99)/100)+(!length)} END{print c}' docs/00-design-overview.md` (~209 baseline as of 2026-05-05). If content can't be expressed within budget, it doesn't belong here — it belongs in REQUIREMENTS / architecture / subsystems / wiki concept pages, with a brief pointer from this doc.

Triggers for editing: new ADR → add to "Design decisions". Major deepening landed → update deepenings table + remaining candidates. Phase boundary reached → update phased plan. Doc-version bump → update v0.x marker. Risks resolved or surfaced → update Risks.

This is the alignment artifact, not a spec. Specs are REQUIREMENTS / architecture / subsystems. Wiki concept and decision pages are the substantive design content. This doc orients; it does not authorise.
