# Apsis — Design Overview

> **Purpose**: alignment checkpoint. Lowest-cost point for direction changes — read here before investing in detailed planning or code.
> **Status**: 2026-05-05.
> **Audience**: anyone joining or returning to the project. Read CLAUDE.md first; this next.

This doc is a self-contained statement of what Apsis *is*, what it *will be*, the design decisions behind it, and the patterns it follows. Process / meta information about how the design was developed (deepenings landed, wiki state, reading order through the corpus) lives in [[wiki/synthesis/development-state-2026-05-05]].

## What Apsis is

A **flight-dynamics-grade spaceflight simulator**. C++17 core, Python (pybind11) scenario layer. Four use cases targeted simultaneously:

1. Single-spacecraft mission simulation at engineering fidelity (multi-body dynamics, full force model, closed-loop GNC, interplanetary).
2. GNC development — swappable controllers and estimators with continuous-discrete plant separation.
3. Monte Carlo verification — thousands of deterministic trials with statistical aggregation.
4. Catalog-scale conjunction screening — ~50,000 SGP4-propagated objects screened against active spacecraft.

Apsis sits in the gap between game-engine physics packages (which lack precision over multi-decade arcs) and research-grade tools like MONTE / GMAT / Orekit (which deliver precision but aren't architected for swappable GNC + Monte Carlo + catalog work in one package).

## Where Apsis is going

A v1.0 release ships when all Must-priority requirements are implemented and tested, plus:

- A validation report reproduces at least three published reference cases within tolerance.
- Performance targets are met on reference hardware: ~1000× real-time cruise on a single core; 50,000-object catalog in under one second per epoch; under sixty seconds for a 7-day Pc screen on a 50,000-object catalog.
- Example scenarios run end-to-end: LEO orbit propagation, GEO stationkeeping, lunar transfer, Mars transfer, attitude slew, conjunction avoidance.
- A 1,000-trial Monte Carlo reference campaign produces correctly-shaped dispersion statistics.
- Conjunction screening reproduces a known historical close-approach event from public TLE data within published miss-distance tolerance.
- The documentation suite is complete: API reference, architecture overview, subsystem design, user guide, validation report.
- CI passes on Linux and Windows.

## Build sequence

Approximate phasing, ~6-8 months sustained effort:

1. **Propagator core** — single spacecraft, force models with the Variational Equations contract, integrators including the Encke wrapper, SPICE integration.
2. **ECS world + catalog** — entity-component-system framework, SGP4 catalog, conjunction screening pipeline.
3. **Multi-body / URDF** — Pinocchio, floating-base coupling, effectors, flexible-body and slosh degrees of freedom.
4. **GNC layer** — sensors, AttitudeEstimator manager (MEKF + USQUE + policy), controllers, message bus.
5. **Monte Carlo** — snapshot/restore, RNG management, parallel trials, columnar result aggregation.
6. **Scenario layer** — Python bindings, scenario DSL, examples.
7. **Hardening** — higher-fidelity force models, advanced GNC, validation campaigns.

End of phase 6 produces a usable simulator. Phases 1-2 alone (~10 weeks) suffice for trajectory studies and conjunction analysis without GNC.

## Design decisions

Five accepted ADRs (all in `docs/wiki/decisions/`):

- **ADR-001** — ICRS↔ITRS transformations use the CIO-based pipeline (X, Y, s, ERA), not equinox-based. SOFA-mediated.
- **ADR-002** — Variational equations (state-transition matrix Φ) are propagated between measurements, not as augmented natural state.
- **ADR-003** — `Time` is type-tagged with its scale (TAI / TT / UTC / UT1 / TDB / optional TCG / TCB). Scale mixing is a compile-time error; conversions explicit and SOFA-mediated.
- **ADR-004** — Attitude estimation uses hybrid mode logic — boot-USQUE for acquisition, NIS-monitored MEKF for nominal operation, automatic fallback on inconsistency. Direct covariance hand-off via shared MRP-flavoured GRP parameterisation. Mission FSM may pin a specific mode.
- **ADR-005** — Broad-phase pair filtering in conjunction screening is a strategy interface. Spatial-hash default, sort-and-sweep required alternative, trajectory-tube as future extension.

Other load-bearing choices (not ADRs but project-wide):

- **f64 throughout, no wider precision.** Numerical robustness comes from frame partitioning, two-component time, Encke-style perturbation propagation, compensated summation, and structure-preserving integrators.
- **SPICE for ephemerides and frames; SOFA for time and IAU models; Pinocchio for multi-body dynamics; Vallado SGP4 (WGS-72 constants) for catalog propagation.** Build what doesn't exist; reuse battle-tested libraries for what does.
- **No global mutable state.** RNGs, time, force-model state passed explicitly. Per-trial determinism is non-negotiable for Monte Carlo.

## Patterns to follow

**Deep modules with internal seams.** One outer module with a small public interface; multiple inner-stage modules behind the seam, each independently testable. Strategy interfaces only where two or more adapters are real (one-adapter "seams" are just indirection).

**Vocabulary discipline.** Module / Interface / Depth / Seam / Adapter / Leverage / Locality used everywhere. Never "component" / "service" / "boundary."

**Conformance via the interface, not past it.** Define contracts at the seam where consumers cross; one parameterised conformance test covers all implementations. The interface is the test surface.

**Frames are first-class.** Every state, position, velocity, attitude claim names its frame. ICRF and J2000 are distinct (frame bias). Frame transitions happen at precision-rich boundaries (atmosphere edge, sphere-of-influence crossings), not mid-orbit.

**Time scales are first-class.** TAI / TT / UTC / UT1 / TDB are not interchangeable. Type-tagged at compile time; SOFA is the only conversion authority.

**Conservation invariants are validation.** Angular momentum for multi-body dynamics; energy for symplectic gravity-only integration; mass for fueled craft. Every dynamics component names which invariant is its validation hook.

**Tiered fidelity per entity.** Active spacecraft (full multi-body) ≠ catalog (SGP4) ≠ debris (J2-only). Don't conflate them.

**ADR shape.** Real choice + non-obvious rejection of an alternative + future-reader confusion about why ⇒ write an ADR. Cosmetic or obvious choices ⇒ don't.

**Wiki citation discipline.** Every factual claim in the wiki traces to a source under `docs/raw/`. Inline `[[sources/<slug>]]` at the claim. `code:<path>:<line>` for code references when source exists.

**Numerical conditioning matters more than raw precision.** Frame partitioning, Encke deviation propagation, compensated summation, structure-preserving integrators — discipline at the formulation level, not wider word sizes.

**Determinism is non-negotiable for Monte Carlo.** No global RNG, no within-trial parallelism, no hash-iteration-order dependencies. Trials are single-threaded; parallelism is across trials.

**Per-process service vs per-screen module.** Persistent state (the catalog) lives in services with process-level lifetime, queried by transient pipelines (a screening pass) that hold no state between calls. Apply wherever lifetime scopes differ between producer and consumer.

**Pure-logic policy classes.** Non-trivial decision logic (mode-switching, fallback selection, threshold-driven escalation) factors out as a small, dependency-free class taking synthetic inputs and producing decisions. Trivially unit-testable.

**Method registry with validity predicates.** When multiple algorithms cover overlapping but distinct regimes, register them with self-declared validity predicates; pipeline picks the first valid method in registry order. The selection is recorded as a diagnostic on the output for audit.

## Out of scope (v1)

- No real-time 3D visualisation (recorded telemetry is the output; visualisation is a companion tool).
- No mission-design optimisation (Lambert / pork-chop / low-thrust optimal control — use GMAT or PyKEP offline).
- No thermal, power, or comms-budget modelling beyond geometric visibility.
- No re-entry aerothermo / hypersonic CFD.
- No FDIR controller logic (failure injection is in scope; response logic is deferred).
- No contact-constrained MBD (capture, berthing, surface contact deferred — math is researched, implementation is not v1).
- No sub-tracking-threshold orbital-debris flux (statistical MMOD modelling deferred).
- No multi-panel high-fidelity drag or SRP.

## Risks and open questions

- **Performance budget viability.** The 60-second screening target on a 50,000-object catalog at 7-day window on a single core is aggressive. Phase-1+2 implementation will reveal whether the strategy-pluggable broad-phase plus Φ-based covariance roll-forward fit the budget. Fallbacks: tighter pre-filter thresholds, parallelism within screening, sample-rate compromise.
- **Long-arc precision validated empirically.** The two-component-time + Encke + compensated-summation trifecta achieves millimetre-level position precision out to 50 AU on paper. Phase 1's regression tests against published reference cases (JPL DE round-trip over ten years; ISS state vectors) are the empirical check.
- **CSPICE thread-unsafety.** SPICE access must be serialised or use per-thread instances. Must be wired correctly in the orchestration layer.
- **Catalog-data redistribution licensing.** Space-Track's terms-of-service constrain what Apsis can ship as example data. Settle for the v1 example scenarios.
- **Cluster-scale Monte Carlo.** v1 targets local-machine 32-core parallelism; cluster-scale (MPI / Ray / Celery) is a v1.x extension.

## Extending this doc

**Budget: ~200 rendered lines at 100-char wrap.** This is a hard ceiling, not a target. When adding content, condense or remove existing material to stay under budget. Verify with `awk '{c+=int((length+99)/100)+(!length)} END{print c}' docs/00-design-overview.md`. If content can't be expressed within budget, it doesn't belong here — substantive design material lives in the spec docs and wiki concept pages.

Triggers for editing: new ADR → add to "Design decisions". Major design choice changes → update the relevant section. Phase boundary reached → update build sequence. Risks resolved or surfaced → update "Risks and open questions". Process / meta updates (deepenings, wiki state, etc.) → update [[wiki/synthesis/development-state-2026-05-05]] instead.

This is the alignment artifact, not a spec. Substantive design content is in REQUIREMENTS / architecture / subsystems and the wiki. This doc orients; it does not authorise.
