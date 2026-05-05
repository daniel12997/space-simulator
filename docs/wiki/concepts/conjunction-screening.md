---
type: concept
canonical_name: Conjunction Screening
aliases: [conjunction screening, conjunction screening pipeline, conjunction assessment, CA pipeline]
created: 2026-05-05
requirements: [REQ-CAT-001, REQ-CAT-002, REQ-CAT-003, REQ-CAT-004, REQ-CAT-006, REQ-CAT-007, REQ-CAT-008, REQ-CAT-009, REQ-CAT-010, REQ-CAT-013, REQ-CAT-014, REQ-CAT-015, REQ-CAT-016, REQ-CAT-017]
sources: [foster-estes-1992-jsc-25898-pc, bombardelli-2015-collision-avoidance, newman-2022-cara-best-practices, ccsds-508-0-b-1-cdm, hoots-roehrich-1980-spacetrack-report-3, vallado-2006-revisiting-spacetrack-3]
decisions: [005-broad-phase-strategy-pluggable]
---

# Conjunction Screening

The streaming pipeline that turns **catalog state plus active spacecraft state** into **conjunction events** with TCA, miss distance, joint covariance, Pc, and HBR. This is the canonical contract: every requirement REQ-CAT-001..017 either feeds, lives in, or consumes the screening pipeline. Collision-avoidance maneuver planning (CAM) is a *downstream* concept — a future deepening will give it its own page consuming events from this pipeline.

## Pipeline shape

The pipeline is a single deep module — `ConjunctionScreeningPipeline` — composing named inner stages. External callers see one entry point:

```cpp
std::vector<ConjunctionEvent> screen(
    const Catalog&                       catalog,
    std::span<const ActiveSpacecraft>    active,
    Time t0, Time t1
);
```

Internally there are four phases:

```
Phase 1: Catalog state maintenance        (CatalogStore, persistent across screens)
                  │
                  ▼
Phase 2: Propagation over screening window
   ├── CatalogPropagator  (SGP4 bulk)
   └── ActivePropagator   (high-fidelity)
                  │
                  ▼
Phase 3: Pair reduction
   ├── OrbitElementPreFilter      (apogee/perigee + inclination)
   ├── AnalyticalDistancePreFilter (Hoots-Crawford / Healy bounds)
   └── BroadPhase                  (strategy: SpatialHash | SortAndSweep | …)
                  │
                  ▼
Phase 4: Per-pair assessment
   ├── TcaRefiner                  (sample-and-bracket → Brent localisation)
   ├── CovariancePropagator        (Φ from Variational Equations integrator)
   ├── PcCalculator                (method registry: Foster | MC | Patera | …)
   ├── HbrResolver                 (combine per-spacecraft HBRs)
   └── EventEmitter                (threshold + payload assembly)
                  │
                  ▼
        std::vector<ConjunctionEvent>
```

Each inner module is independently testable. `CatalogStore` lives outside the pipeline as a persistent service queried via the `Catalog&` parameter — its lifetime scope is per-process, not per-screen.

## Phase 1 — Catalog state maintenance

The `CatalogStore` ingests two stream types:

- **TLE feed** (REQ-CAT-001) from Space-Track / CelesTrak; classic 80-column 2-line format with modulo-10 checksum. Bad-checksum lines are logged but not fatal by default (configurable; some operational catalogs occasionally contain them and operators expect flag-and-pass-through behaviour). Per [[sources/kelso-celestrak-tle-format|CelesTrak TLE format]] and [[sources/hoots-roehrich-1980-spacetrack-report-3|STR#3]].
- **CCSDS 508.0-B-1 CDM** (REQ-CAT-004, REQ-CAT-014) ingest in both KVN and XML serialisations. Each CDM provides per-object state at its emission TCA, 6×6 covariance in RTN frame, drag/SRP coefficients, maneuverability flag, and originator-computed Pc (retained for cross-reference but not authoritative).

Both ingest paths land in the same `CatalogObject` shape: identifier, latest TLE (with epoch), latest covariance (when available), drag/SRP coefficients, optional maneuverability flag. Daily refresh from external sources (REQ-CAT-003) reuses the same ingest paths.

**HBR (Hard-Body Radius)** does not appear in CDMs. Apsis maintains a separate per-spacecraft HBR config (REQ-CAT-013); the catalog stores the per-object HBR alongside its other metadata and the `HbrResolver` (Phase 4) combines as `HBR_combined = HBR_primary + HBR_secondary` for Pc evaluation.

## Phase 2 — Propagation over screening window

For a screening window `[t₀, t₁]` (default 7 days forward):

- **Catalog propagation**: SGP4 bulk-propagation per [[concepts/sgp4]] using WGS-72 constants (REQ-INT-005). Per-object cost ~10 µs; 50k-object catalog propagates in ~500 ms (REQ-PERF-003: <1 sec per epoch). The propagator emits a sample-epoch series — SoA layout, hot loop, optional SIMD batching.
- **Active spacecraft propagation**: high-fidelity (full force model) propagation of the user's active spacecraft over the same window, dense-output enabled (REQ-INT-008) so Phase 4 can interpolate freely.

Both feed Phase 3 with sample-epoch geometry; the active spacecraft also retains its dense output for sub-millisecond TCA evaluation in Phase 4.

## Phase 3 — Pair reduction

The naive screening cost is `O(N × M)` for N catalog objects and M active spacecraft, evaluated per sample epoch. For N=50k that's ~50k checks per active spacecraft per epoch — manageable with cheap per-pair tests but expensive if every pair reaches the geometric stage. Phase 3 reduces the candidate pair count in three layers, each cheaper than the next.

### 3.1 Orbit-element pre-filters (REQ-CAT-006)

Cheap arithmetic on orbit elements:

- **Apogee/perigee filter**: if `apogee(A) < perigee(B)` or vice versa, the orbits cannot intersect. Skip.
- **Inclination filter**: for shared central body, orbits with very different inclinations have a small mutual-altitude window — eliminable via more elaborate variants of this test, primarily useful at GEO.

These eliminate **99%+ of pairs** before any geometric evaluation. After this layer, the residual pair count is typically ~10⁵-10⁶ from the original ~10⁹+.

### 3.2 Analytical orbit-distance bounds (REQ-CAT-006)

A second per-pair pre-filter using **Hoots-Crawford / Healy analytical orbit-distance bounds**: for two elliptical orbits with similar periods, compute a **lower bound on minimum mutual distance** purely from orbital elements, without per-epoch geometric evaluation. If the lower bound exceeds the screening threshold (HBR plus margin), the pair cannot conjunct in this window — skip.

Eliminates an additional 50-90% of the residual pairs at negligible cost (per-pair arithmetic). After this layer the residual is typically ~10³-10⁴ pairs requiring geometric broad-phase.

### 3.3 Geometric broad-phase (REQ-CAT-007 + REQ-CAT-017)

The **broad-phase is a strategy interface** ([[decisions/005-broad-phase-strategy-pluggable|ADR-005]]) — multiple implementations satisfy it; the pipeline picks one by config:

```cpp
class BroadPhase {
public:
    virtual std::vector<CandidatePair> filter(
        const PropagatedCatalog& catalog,
        std::span<const PropagatedActive> active,
        Time t0, Time t1
    ) = 0;
};
```

Apsis ships two implementations in v1:

- **SpatialHashBroadPhase (default)** — uniform 3D voxel grid (default voxel size 100 km, configurable). At each sample epoch, hash positions into voxels; only same-voxel and neighbour-voxel pairs survive. O(N + K) per epoch where K is the close-pair count.
- **SortAndSweepBroadPhase** — sweep-and-prune along the radial axis. AABBs sorted; sweep to find overlaps. Incremental updates between sample epochs are very cheap (orderings change minimally), making it competitive for smoothly-evolving catalogs.

Future extension point: **trajectory-tube intersection** (CARA's COSINE / SOCRATES-style) for very-dense or very-long-duration screening. Adding it requires only a new `BroadPhase` implementation; no pipeline-interface change. See [[decisions/005-broad-phase-strategy-pluggable|ADR-005]] for rationale.

After the broad-phase, the residual is typically ~10²-10³ candidate pairs that survive to Phase 4 per active spacecraft, with epoch hints localising likely-conjunction intervals.

## Phase 4 — Per-pair assessment

For each candidate pair surviving Phase 3:

### 4.1 TCA bracketing and localisation

**Bracketing**: evaluate `|r_A(t) - r_B(t)|` at sample epochs across the candidate interval; identify all sign changes of `d/dt |r_A - r_B|²`, each marking a local minimum. **A pair can have multiple close approaches in a 7-day window** if the orbits have similar periods — every local minimum is a separate conjunction event candidate.

**Localisation**: per local minimum, Brent's method on `d/dt |r_A - r_B|² = 0` over the bracketed interval (REQ-CAT-008). Sub-millisecond TCA precision (which corresponds to <10 m miss-distance precision at typical 7-15 km/s relative velocities).

### 4.2 Miss distance and relative velocity

Evaluated at TCA from each object's propagated state:
- `miss_distance = |r_A(TCA) - r_B(TCA)|`
- `v_rel = v_A(TCA) - v_B(TCA)`

### 4.3 Joint covariance roll-forward

Per-object 6×6 covariance is propagated from the CDM epoch (or last known covariance epoch) to TCA via the [[concepts/variational-equations|Variational Equations integrator]] — `P(TCA) = Φ · P(t_epoch) · Φᵀ` for each object. Joint covariance at TCA is the sum:

```
C_joint(TCA) = Φ_A · C_A(t_epoch) · Φ_Aᵀ  +  Φ_B · C_B(t_epoch) · Φ_Bᵀ
```

(per object independence assumption — standard in operational CA per [[sources/foster-estes-1992-jsc-25898-pc|Foster & Estes 1992]] App. C).

### 4.4 Pc computation

Apsis uses a **Pc method registry**. Each registered method declares a validity predicate (typically a function of `ε = t_c / T_orbit` where `t_c ≈ 2σ_η / |v_rel|` is the encounter duration); the pipeline picks the **first valid method in registry order** for the current pair. Default registry: `[foster_2d, monte_carlo]`.

| Method | Validity | Cost | Notes |
|---|---|---|---|
| **Foster 2D analytic** (REQ-CAT-009) | `ε < 1e-3` | ~µs per pair | Primary for short-term encounters (LEO close approaches); short-term-encounter assumption per [[sources/foster-estes-1992-jsc-25898-pc|Foster & Estes 1992]]. Apsis uses Chan series ([[sources/bombardelli-2015-collision-avoidance|Bombardelli 2015]] Eq. 4) as the numerically convenient evaluator |
| **Patera 2D extended-duration** (REQ-CAT-015, S) | `ε < 1e-1` | ~ms per pair | Optional alternative covering moderate-ε regime where Foster fails but MC cost is undesirable. Registered by configuration, not default |
| **Monte Carlo** | always valid | ~ms per pair (default N=10,000 samples) | Default fallback for `ε > 1e-3`. Sample joint state from `C_joint(TCA)`, count fraction within `HBR_combined`, divide by N. Per-pair seeded RNG (REQ-MC-003); the seed is part of the conjunction event for reproducibility |

The chosen method is recorded in the conjunction event's `pc_method` field for diagnostic / audit use. Future methods (Akella-Alfriend, Alfano, etc.) are added as additional registry entries with their own validity predicates.

### 4.5 HBR resolution (REQ-CAT-013)

`HBR_combined = HBR_primary + HBR_secondary`. Both per-spacecraft HBRs are loaded from the catalog (Phase 1) — Apsis-side configuration, not CDM-derived (CCSDS 508 doesn't carry HBR). The combined hard-body radius is what the Pc method consumes for its disc/cylinder integration.

### 4.6 Event emission (REQ-CAT-010)

When `Pc > pc_threshold` (default 1e-5, configurable per scenario), emit a `ConjunctionEvent` with:

- `time_of_closest_approach` — TCA in TT
- `miss_distance` (m), `relative_velocity` (m/s) at TCA
- `pc` and `pc_method` (string tag from the registry)
- `hbr_combined` (m)
- per-object `state_at_tca` (position + velocity in inertial frame)
- per-object `covariance_at_tca` (6×6 in RTN, post roll-forward)
- per-object identifiers (catalog ID, name)
- `screening_diagnostics`: which broad-phase strategy was used, how many sample epochs were evaluated, MC seed (if applicable), originator-Pc from CDM (if any) for cross-reference

The event is the **handoff to downstream consumers**: a future CAM-planner deepening will consume these events for Bombardelli-style optimal Δv computation (audit Cluster G); operations dashboards consume them for visualisation; mission scheduler consumes them for maneuver-window planning.

## Performance budget

REQ-PERF-004 specifies **< 60 sec for a 7-day window over a 50k-object catalog with one active spacecraft on a single core**. Allocation across stages:

| Stage | Allocation | Notes |
|---|---|---|
| Catalog SGP4 bulk propagation | ~5 sec | ~100 sample epochs × 500 ms per epoch |
| Active spacecraft high-fidelity | ~1-3 sec | depends on force-model fidelity |
| Orbit-element pre-filter (3.1) | <1 sec | 50k pairs × cheap arithmetic |
| Analytical distance bounds (3.2) | <1 sec | residual ~10⁵ pairs, cheap |
| Geometric broad-phase (3.3) | ~10-20 sec | residual ~10³ pairs after pre-filters; varies with strategy |
| Per-pair assessment (Phase 4) | ~20-30 sec | ~10²-10³ candidate pairs × per-pair cost (TCA + covariance + Pc) |
| Event emission | <1 sec | trivial |

The MC fallback path adds cost only for the rare (typically <1%) pairs where Foster's validity fails. Patera at S priority would replace MC for some of those pairs, dropping cost in the moderate-ε regime.

## Tuning knobs (with defaults)

| Knob | Default | Purpose |
|---|---|---|
| `screening_window` | 7 days forward | Default temporal window per REQ-CAT-005 |
| `voxel_size` | 100 km | SpatialHashBroadPhase voxel size |
| `tca_sample_interval` | 60 sec | TCA bracketing sample spacing |
| `tca_localisation_tolerance` | 1 ms | Brent convergence target |
| `pc_threshold` | 1e-5 | Event emission threshold |
| `pc_method_registry` | `[foster_2d, monte_carlo]` | Method order; user configures e.g. `[foster_2d, patera_2d, monte_carlo]` |
| `pc_method_validity_eps` | 1e-3 | Foster→fallback switch criterion |
| `mc_samples` | 10,000 | Monte Carlo Pc sample count |
| `tle_bad_checksum_policy` | log_and_pass | Operational catalog tolerance |

All scenario-configurable.

## See also

- [[decisions/005-broad-phase-strategy-pluggable]] — broad-phase strategy interface ADR.
- [[concepts/sgp4]] — catalog propagator algorithm.
- [[concepts/variational-equations]] — Φ source for covariance roll-forward.
- [[concepts/long-arc-state-conditioning]] — substrate for 7-day window precision.
- [[sources/foster-estes-1992-jsc-25898-pc]] — Pc primary method.
- [[sources/bombardelli-2015-collision-avoidance]] — Chan series evaluator + Pc validity criterion.
- [[sources/newman-2022-cara-best-practices]] — operational practices that consume conjunction events.
- [[sources/ccsds-508-0-b-1-cdm]] — CDM ingest format.
- [[sources/hoots-roehrich-1980-spacetrack-report-3]] / [[sources/vallado-2006-revisiting-spacetrack-3]] — TLE and SGP4 references.
- [[sources/kelso-celestrak-tle-format]] — TLE format and operational tolerance.
- (Future) collision-avoidance maneuver concept page — downstream consumer of conjunction events.
