---
type: decision
title: "Broad-phase pair filtering is a strategy interface; spatial-hash default, sort-and-sweep required alternative, trajectory-tube as future extension"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: [foster-estes-1992-jsc-25898-pc, newman-2022-cara-best-practices]
components: []
requirements: [REQ-CAT-007, REQ-CAT-017]
---

## Status

**Accepted** 2026-05-05. Surfaced during the architecture-review grilling on the *Conjunction Screening Pipeline* deepening (audit candidate #1). Cited from [[concepts/conjunction-screening]] §3.3 and from REQUIREMENTS.md REQ-CAT-007 / REQ-CAT-017 (v0.6).

## Context

The conjunction-screening pipeline ([[concepts/conjunction-screening]]) reduces O(N×M) pair count via three layers — orbit-element pre-filters (REQ-CAT-006), analytical orbit-distance bounds, and a geometric broad-phase. The broad-phase handles the residual ~10³-10⁴ candidate pairs surviving the cheaper pre-filters; for each pair it identifies sample-epoch intervals where the pair is geometrically close enough to merit Phase 4 (TCA + Pc) evaluation.

Multiple algorithms can implement this stage:

- **Uniform spatial hash (voxels)** — simple 3D voxel grid; per-epoch O(N + K) where K is the close-pair count. Cache-friendly when voxel size matches typical Δr per timestep. Used by some operational systems and most game-style trackers.
- **Sort-and-sweep (sweep-and-prune)** — AABBs sorted along an axis, swept to find overlaps. O(N log N) build, O(N + K) sweep, **incremental updates very cheap** when orderings persist between epochs. Faster than spatial hash for moderate N when catalog evolves smoothly.
- **BVH / k-d tree** — adaptive spatial partitioning, O(log N) per query. Refit per epoch; well-suited to one-snapshot use.
- **Trajectory-tube intersection** — temporally aware; treats each orbit's trajectory over the screening window as a 4D tube (3 space + 1 time) and finds tube intersections. Used by NASA CARA's COSINE and CelesTrak's SOCRATES. Doesn't sample-and-miss between epochs but significantly more complex.
- **Analytical orbit-distance bounds** — purely orbit-element-based; non-spatial. Already covered by Phase 3.2 (a separate pre-filter layer, not a broad-phase replacement).

The v0.5 REQUIREMENTS pinned one method (spatial hashing) implicitly — REQ-CAT-007 said *"The system SHALL apply spatial hashing or equivalent broad-phase filter."* The "or equivalent" was vague; the spec text mostly assumed spatial hash. The deepening surfaced that this is a real architectural choice with multiple defensible answers.

## Decision

The geometric broad-phase SHALL be implemented as a **strategy interface** (`BroadPhase`) rather than pinned to one algorithm. Apsis ships two implementations in v1:

- **`SpatialHashBroadPhase`** as the default — uniform 3D voxel grid; default voxel size 100 km, configurable per scenario.
- **`SortAndSweepBroadPhase`** as a required alternative — sweep-and-prune along the radial axis; cheap incremental updates between sample epochs.

Future extension implementations (notably **trajectory-tube intersection** for very-dense or very-long-duration screening) SHALL be addable without modifying the `BroadPhase` interface, the pipeline interface, or any consumer.

Selection between strategies is **per scenario** via the `ConjunctionScreeningPipeline::Config::broad_phase_strategy` configuration knob.

## Rationale

- **No single broad-phase wins universally.** Spatial hashing is simple and parallelisable; sort-and-sweep is faster for smoothly-evolving catalogs because its incremental update cost is near-zero between epochs; trajectory tubes catch pairs that sample-and-miss approaches miss. Each is the right answer in different operational regimes.
- **The choice is scenario-specific.** A long-duration interplanetary mission with a small interesting-object set wants different tradeoffs than a 50k-object LEO catalog screened daily. Pinning one method forces all use cases into one tradeoff.
- **Future-proofing.** Trajectory-tube methods are operationally significant (CARA, SOCRATES, ESA's CONJOINTLY) but expensive to implement. Deferring them while keeping the interface open lets v1 ship with the simpler methods and add tube-style screening later without surgery.
- **Testability.** A strategy interface with two implementations gives us **two adapters** at the seam — exactly the threshold for "real seam" in the architecture-skill discipline (one adapter is hypothetical; two means the seam is real).
- **Aligns with existing Apsis pattern.** The Encke propagator wraps any base integrator (ADR-002 indirectly establishes the strategy pattern); the `AttitudeEstimator` wraps any algorithm under a policy (ADR-004). Strategy-pluggable subsystems with a clean default are now Apsis's house style.
- **Sort-and-sweep is genuinely useful, not just a defensive option.** For the typical operational use case (daily 7-day screening of a slowly-changing catalog) sort-and-sweep's incremental updates are materially cheaper than per-epoch spatial-hash rebuilds. We're not committing to maintain a method we don't use; we're committing to two methods because each is the right answer for a real subset of use cases.

## Alternatives considered

**Pin spatial hash, document trajectory-tube as a future improvement.** Rejected because (a) sort-and-sweep is cheap to add and operationally useful — the mental cost of "we ship one method" doesn't pay for itself; (b) makes the future trajectory-tube addition a breaking change to all callers rather than a new strategy implementation; (c) doesn't surface the genuine choice that exists.

**Pin trajectory-tube as the default, the operationally-correct choice.** Rejected because trajectory-tube methods are significantly more complex (tube parameterisation for SGP4 orbits is non-trivial; the math is heavier), implementation effort doesn't fit a v1 timeline, and the simpler spatial-hash / sort-and-sweep methods cover the ε regimes Apsis actually targets at v1. We can add it later via the strategy interface without breaking anything.

**Two methods bundled with no abstraction (just an `if-else` in the pipeline).** Rejected because the abstraction is cheap (one virtual interface), the lack of abstraction makes adding a third method a pipeline-internal edit rather than a new file, and it precludes user-defined broad-phase strategies via REQ-EXT-* extension points.

**One method per spacecraft (per-active broad-phase choice).** Considered as a refinement; rejected for simplicity. If a real use case appears, the strategy interface makes it cheap to add (one strategy per active spacecraft is a configuration change, not an interface change).

## Consequences

- The `ConjunctionScreeningPipeline::Config` carries a `broad_phase_strategy` enum (spatial-hash / sort-and-sweep / future) plus per-strategy parameters (voxel size, sweep axis, etc.).
- Broad-phase implementations live in their own files; each is independently testable against synthetic catalog snapshots.
- The `ConjunctionScreeningSystem` (architecture §3 ECS Systems) holds a `std::unique_ptr<BroadPhase>` configured at scenario load time.
- **REQ-CAT-007 reworded** to specify "broad-phase strategy" rather than pinning spatial hashing.
- **REQ-CAT-017 added** to mandate the strategy interface and the two v1 implementations.
- **Test surface increases**: two broad-phase implementations need conformance tests (do they produce the same candidate-pair set, modulo ordering, on synthetic catalogs?). One conformance test parameterised over implementations covers both.
- **Performance characteristics differ by strategy** — performance budget (REQ-PERF-004) is per scenario, not per strategy. Documentation in [[concepts/conjunction-screening]] §"Performance budget" surfaces the per-strategy expectations.
- **Future trajectory-tube extension is unblocked** — adding it requires (a) a new `BroadPhase` implementation, (b) a new strategy enum value, (c) a strategy-specific config struct. No callers change.

## Open items if accepted

- Decide whether broad-phase strategies are exposed in the Python binding (REQ-SCN-001) as enum constants or as full strategy classes user-instantiates. Recommend enum constants for v1 simplicity; full strategy classes if a real custom-strategy use case appears.
- Defer until benchmarking: which strategy is actually faster for the canonical 50k-LEO-catalog 7-day-window scenario. Our claim that sort-and-sweep wins for smoothly-evolving catalogs is theoretical; an early benchmark validates or refutes it. The default may flip if benchmarks contradict expectation.
- Confirm that the `BroadPhase` interface signature handles **multi-active-spacecraft** scenarios correctly — i.e., that the candidate-pair output is structured to identify pairs as `(active_index, catalog_index)` not just `(index_a, index_b)`. Implementation detail surfaced in REQ-CAT-007 / REQ-CAT-017 reword.
