# Apsis — Design Overview

> **Purpose**: alignment checkpoint. This is the lowest-cost point for direction changes — read here before investing in detailed planning or code.
> **Status**: 2026-05-05. After four architecture-review deepenings; before code.
> **Audience**: anyone joining the project, or returning to it after time away. Read CLAUDE.md first; this next; then drill into REQUIREMENTS / architecture / subsystems / wiki.

## What Apsis is

Apsis is a **flight-dynamics-grade spaceflight simulator**. C++17 core, Python (pybind11) scenario layer. Targets four use cases simultaneously:

1. Single-spacecraft mission simulation at engineering fidelity (multi-body, full force model, closed-loop GNC, interplanetary).
2. GNC development — swappable controllers/estimators with proper continuous-discrete plant separation.
3. Monte Carlo verification — thousands of deterministic trials with statistical aggregation.
4. Catalog-scale conjunction screening — ~50,000 SGP4-propagated objects screened against active spacecraft.

Sits in the gap between game-engine physics (lacks precision over multi-decade arcs) and research-grade tools like MONTE / GMAT / Orekit (precision but not architected for swappable GNC + Monte Carlo + catalog work in one package).

## Current state (2026-05-05)

**No source code exists yet.** What exists is a thoroughly-developed design corpus:

- **Three authoritative spec documents at `docs/` root** (REQUIREMENTS, architecture, subsystems), all currently at v0.6.
- **Wiki at `docs/wiki/`** with 47 source pages (research corpus — every paper / spec / standard the design depends on, summarised), 30 concept pages (algorithms + architectural patterns), 5 ADRs, 4 audit synthesis pages.
- **All 50 raw research artifacts** (32 papers, 17 specs, 1 article) ingested under `docs/raw/`.

Four deepenings (per the `improve-codebase-architecture` skill) have turned previously-scattered spec fragments into named modules with explicit interfaces, conformance discipline, and load-bearing ADRs:

| Deepening | What it consolidated |
|---|---|
| **Variational Equations** (v0.3) | Force-model partials contract + Φ propagation — was 1 line of REQ-PHY-016, now a named subsystem with conformance harness |
| **Long-arc state conditioning** (v0.4) | Two-component time + Encke + compensated summation — three pillars under one architectural principle |
| **Attitude Estimator Family** (v0.5) | MEKF + USQUE + hybrid mode logic — was a 1-line subsystems aside, now a manager-plus-policy module |
| **Conjunction Screening Pipeline** (v0.6) | 14 REQ-CAT IDs + scattered subsystems content → one named pipeline with 4 phases and a strategy-pluggable broad-phase |

What's **still scattered** (remaining audit candidates, by priority):

- **GNC Message Bus Contract** (premature without code).
- **Deterministic Monte Carlo Contract** (bounded; assemble-existing-pieces).
- **Floating-Base Coupling** (lowest-risk extension of existing concept page).
- **Spacecraft Fidelity Scope** (ADR-shaped, not a deepening).
- **CAM Planning** (downstream of Conjunction Screening; planned future deepening).

## Desired end state (v1.0)

Per REQUIREMENTS §18, v1.0 ships when **all M (Must) requirements** are implemented and tested, plus:

- Validation report reproduces ≥3 published reference cases within tolerance.
- Performance requirements met on reference hardware (1000× real-time cruise on single core; 50k-object catalog in <1 sec/epoch; <60 sec for 7-day Pc screen on 50k catalog).
- All example scenarios run end-to-end (LEO, GEO stationkeeping, lunar/Mars transfers, attitude slew, conjunction avoidance).
- 1k-trial Monte Carlo reference campaign runs and produces correctly-shaped dispersion statistics.
- Conjunction screening reproduces a known historical close-approach event from public TLE data within published miss-distance tolerance.
- Documentation suite complete: API reference (Doxygen), architecture overview, subsystem design, user guide, validation report.
- CI passes on Linux + Windows.

Phased build plan (from `01-architecture.md` §7, ~6-8 months sustained):

1. **Propagator core** (6-8 wk) — single spacecraft, force models with Variational Equations contract, integrators (incl. Encke wrapper), SPICE.
2. **ECS world + catalog** (3-4 wk) — EnTT/flecs, conjunction-screening pipeline.
3. **Multi-body / URDF** (6-8 wk) — Pinocchio, floating-base coupling, effectors, flexible/slosh DOFs.
4. **GNC layer** (4-6 wk) — sensors, AttitudeEstimator manager, controllers, message bus.
5. **Monte Carlo** (3-4 wk) — snapshot/restore, RNG management, parallel trials, Parquet aggregation.
6. **Scenario layer** (3-4 wk) — pybind11, Python DSL, examples.
7. **Hardening** (ongoing) — higher-fidelity force models, advanced GNC, validation campaigns.

End of Phase 6 produces a usable simulator. Phases 1-2 (~10 weeks) alone are sufficient for trajectory studies and conjunction analysis without GNC.

## Design decisions

Five accepted ADRs (in `docs/wiki/decisions/`):

- **ADR-001** — ICRS↔ITRS uses the **CIO-based pipeline** (X, Y, s, ERA), not equinox-based. SOFA `iauC2t06a`. Rationale: ERA is linear in UT1; no equation-of-equinoxes; modern IAU recommendation; new code has no legacy obligation.
- **ADR-002** — Variational equations are propagated **between measurements**, not as augmented natural state. Decouples Φ accuracy from natural-state step controller; avoids state-vector bloat.
- **ADR-003** — `Time` is **type-tagged with its scale** (`TtTime`, `TaiTime`, etc.). Scale mixing is a compile-time error. Conversions explicit, route through SOFA.
- **ADR-004** — Attitude estimation uses **hybrid mode logic** — boot-USQUE plus NIS-monitored MEKF. Direct covariance hand-off (MRP-flavoured GRP for both filters). Mission FSM may pin via `pin_mode()`.
- **ADR-005** — Broad-phase pair filtering is a **strategy interface**. Spatial-hash default, sort-and-sweep required alternative, trajectory-tube as future extension. v0.5's implicit "spatial hash only" replaced with two adapters at the seam.

Other load-bearing choices that aren't ADRs but appear repeatedly across the design:

- **f64 throughout, no wider precision.** Numerical robustness comes from frame partitioning, two-component time, Encke, compensated summation, and structure-preserving integrators (Long-arc state conditioning concept page).
- **SPICE for ephemerides + frames; SOFA for time + IAU; Pinocchio for MBD; Vallado SGP4 (WGS-72) for catalog.** Build the things that don't exist; don't rebuild the things that do.
- **No global mutable state.** RNGs, time, force-model state passed explicitly. Per-trial determinism is non-negotiable for Monte Carlo.

## Patterns to follow

**Deep modules with internal seams** (LANGUAGE.md vocabulary). The architectural style established across all four deepenings:

- One **outer module** with a small public interface (e.g. `ConjunctionScreeningPipeline::screen`, `AttitudeEstimator::predict`/`update`, `VariationalEquationsIntegrator::propagate`).
- Multiple **inner-stage modules** behind the seam, each independently testable (e.g. `TcaRefiner`, `Mekf`, `OrbitElementPreFilter`).
- **Strategy interfaces** introduced only where ≥2 adapters are real (broad-phase, base integrator wrapped by Encke). One-adapter "seams" are just indirection.

**Vocabulary discipline**. The `improve-codebase-architecture` skill establishes a fixed vocabulary — Module / Interface / Depth / Seam / Adapter / Leverage / Locality — used in every concept page, ADR, and audit. Never "component" / "service" / "boundary." Worth maintaining as the project grows; consistent vocabulary is what lets a new contributor read any page and immediately know what's load-bearing.

**Conformance via the interface, not past it**. The Variational Equations conformance harness (a single CI gate over every registered force model checking analytical partials against finite differences) is the canonical pattern. New strategy-pluggable subsystems should follow: define the contract; one conformance test parameterised over implementations.

**Frames are first-class.** Every state, position, velocity, attitude claim names its frame. ICRF and J2000 are distinct (frame bias). CCI vs CCF transitions happen at precision-rich boundaries, not mid-orbit. Body-fixed vs body-centered-inertial confusion is a load-bearing failure mode.

**Time scales are first-class.** TAI / TT / UTC / UT1 / TDB are not interchangeable — type-tagged per ADR-003. SOFA is the only conversion authority.

**Conservation invariants are validation.** Angular momentum for MBD; energy for symplectic gravity-only; mass for fueled craft. Every dynamics component should name which invariant is its validation hook.

**Tiered fidelity per entity.** Active spacecraft (full multi-body) ≠ catalog (SGP4) ≠ debris (J2-only). Don't conflate them across concept / requirement / subsystem boundaries.

**ADR shape.** A real choice + non-obvious rejection of an alternative + future-reader confusion ⇒ write an ADR. Cosmetic or obvious choices ⇒ don't. The five existing ADRs are the model.

**Wiki citation discipline.** Every factual claim in the wiki traces to a source under `docs/raw/`. Inline `[[sources/<slug>]]` at the claim. `code:<path>:<line>` for code references (when source code starts existing). Concept pages cite multiple sources; source pages cite multiple consumer concepts.

**Project-specific guidance** (CLAUDE.md):
- Numerical conditioning matters more than precision.
- Determinism is non-negotiable for Monte Carlo (no global RNG, no within-trial parallelism, no hash-iteration-order dependencies).
- Monte Carlo trials are single-threaded; parallelism is across trials.

## How to extend this doc

- New ADR accepted → add to "Design decisions" §.
- Major deepening landed → update the deepenings table in "Current state" + add to remaining-candidates list if new ones surface.
- Phase boundary reached → update "Desired end state" phased plan.
- Doc-version bump → update the v0.x marker.

This is the alignment artifact, not a spec. Specs are REQUIREMENTS / architecture / subsystems. Wiki concept and decision pages are the substantive design content. This doc orients; it does not authorise.
