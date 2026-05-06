# Phase 1A Hardening Plan

> **Gate**: Phase 2 ECS + catalog + conjunction screening can build on a
> Phase 1 surface that delivers what the plan and ADR-009 claim. Specifically,
> the active-spacecraft side of narrow-phase conjunction screening (Φ-based
> covariance roll-forward) needs (a) realistic non-zonal Earth gravity with
> the body-fixed frame correctly applied and (b) analytical force-model
> partials so Φ accuracy at narrow-phase isn't FD-limited.

Phase 1A is a hardening pass that closes the deferred items from Phase 1
that materially block Phase 2 quality, plus the small reviewer-flagged
items that the Phase 1 PR carried as non-blocking observations.

## Reference

- Phase 1 plan: `docs/phase-1-plan.md` (the contract that shipped reduced)
- Issues addressed: **#5**, **#6**, **#7**, **#11** (Tier 1+2 from Phase 1 close-out triage), **#13**, **#14**, **#15** (skipped Phase 1 plan items)
- ADRs consumed: [[wiki/decisions/002-variational-equations-between-measurements]],
  [[wiki/decisions/003-tagged-time-scale-types]],
  [[wiki/decisions/009-hand-rolled-integrator-family]],
  [[wiki/decisions/010-phantom-typed-time-and-state]],
  [[wiki/decisions/012-eigen-with-apsis-math-aliases]]
- Concepts cited: [[wiki/concepts/long-arc-state-conditioning]],
  [[wiki/concepts/variational-equations]],
  [[wiki/concepts/spherical-harmonic-geopotential]] (load-bearing for §C),
  [[wiki/concepts/gauss-jackson-integration]] (load-bearing for §D2),
  [[wiki/concepts/f-and-g-series]] (D2 starter)

## Phase 1A scope

Four batches, **2–3 deliverables each**, per the
`qrspi_implement_procedure.md` T6 batching rule:

- **Batch A — vocabulary + frame guard** (2 deliverables; no dependencies)
- **Batch B — EOP threading** (1 deliverable; substantive API change)
- **Batch C — SphericalHarmonic quality** (2 deliverables; quality-blocking for Phase 2 narrow-phase conjunction screening)
- **Batch D — integrator family** (2 deliverables; quality-improving)

Each batch is one branch + one PR + one clear-eyes review per the new
batch model. Batches sequence A → B → C → D; each batch's branch is cut
off the latest `main` after the prior batch has merged.

**Note on Phase 7 vs Phase 1A scope**: this hardening pass pulls four
items currently *titled* "Phase 7" in the issue tracker (#5, #6, #7,
#11) forward into Phase 1A on the basis that they materially block
Phase 2 narrow-phase conjunction screening quality. The structure
outline (`docs/structure.md` Phase 1) lists GJ8 and the VE contract on
every adapter as Phase 1 acceptance items, so Phase 1A is *recovering*
Phase 1 scope from explicit Phase 1 deferrals, not creeping outside
it. Issue titles will be re-tagged "Phase 1A" once their respective
batches close (manual checklist below). Issues #8 (real ISS
reference), #9 (full DE440 fetch tooling), #10 (Yoshida4 co-Φ), #12
(Battin f(q) restoration) remain Phase 7.

---

## Batch A: vocabulary + frame guard

> **Goal**: close the small mechanical reviewer findings from PR #4 that
> have no dependencies and can land as one cohesive batch.

### A1. Rename `IForceModel::partials` → `partials_dadx` (issue #14)

> Closes: REQ-PHY-016 vocabulary alignment with ADR-009.

**Files**:
- `include/apsis/force/iforce_model.h` — interface declaration
- `include/apsis/force/{point_mass,spherical_harmonic,third_body}.h` — override declarations
- `src/force/{point_mass,spherical_harmonic,third_body}.cc` — override definitions
- `tests/conformance/force_model_ve_contract.cc` — test call sites

Mechanical rename. ~10 references. ADR-009's Consequences section
already says `partials_dadx`; this aligns the code to the ADR.

**Verify**:
- [x] `grep -rn "::partials\b" --include='*.cc' --include='*.h' include src tests` returns nothing.
- [x] Local CI parity (per `qrspi_implement_procedure.md` T2): all checks green.
- [x] `ctest --test-dir build` 49/49 pass (one more than the original 48 because A2 adds a beacon test).

### A2. `State<Frame>` compile-fail test (issue #13)

> Closes: Phase 1 plan §3 verification line "frame-mixing equivalent".

**Files**:
- `tests/unit/frames/frame_mixing_guard.cc` (new) — parallel to `tests/unit/time/scale_mixing_guard.cc`
- `tests/CMakeLists.txt` — wire the new compile-fail target via the same `try_compile`-with-FATAL_ERROR-on-success pattern the existing scale-mixing guard uses (CMake has no `EXPECT_FAIL` flag; the inversion is "configure FATAL_ERROR if compile succeeds")

**Body**: take a small snippet that mixes frame tags at the type level —
e.g. invoking `apsis::frames::transform<tags::ICRF, tags::ICRF>(state_in_itrs, t)`
where `state_in_itrs : State<tags::ITRS>` — and assert the snippet does
**not** compile. Mirror the existing scale-mixing guard's CMake structure.

**Verify**:
- [x] `ctest -R FrameMixingGuard` reports PASSED (compile-fail-as-expected).
- [x] Removing the type mismatch in the snippet produces a configure-time CMake FATAL_ERROR
      (i.e. the test catches the absence-of-mismatch, proving it isn't
      vacuously passing). Confirm during implementation; do not commit
      the inversion test, just verify it fires.

---

## Batch B: EOP threading

> **Goal**: eliminate the process-wide mutable EOP globals — required
> for Phase 5 MC determinism and worth doing now while the call surface
> is small.

### B1. `EopTable` API parameter (issue #15)

> Closes: REQ-TIME-* (UT1-UTC sourcing), Phase 1 plan §2 "loaded once at
> startup" promise that wasn't actually wired, design-overview "no
> global mutable state" + Phase 5 determinism requirement.

**Files**:
- `include/apsis/time/eop_table.h` (new) — `EopTable` class with `load_from_csv`, `query(Time<TT>) -> EopValues`, linear interpolation between rows.
- `src/time/eop_table.cc` (new) — implementation.
- `src/time/convert.cc` — `convert<UT1, UTC>` and inverse take `const EopTable&` parameter; remove `g_default_dut1` global + setter.
- `src/frames/icrs_itrs.cc` — `transform<ITRS, ICRF>` and inverse take `const EopTable&` parameter; remove `g_polar_motion_xp/yp` globals + setters.
- `include/apsis/time/convert.h` and `include/apsis/frames/transform.h` — public API parameter additions (signature break).
- `tests/unit/time/eop_table_test.cc` (new) — load `data/iers_eop_phase1.csv`, verify a known-row value to the interpolation tolerance.
- `tests/unit/{time,frames}/*_test.cc` — call sites updated; tests construct an `EopTable` explicitly (or via a shared test fixture) instead of calling `set_default_*`.

**Key signatures**:

```cpp
namespace apsis::time {

struct EopValues {
  double dut1{};       // UT1 - UTC, seconds
  double polar_xp{};   // polar motion x, radians
  double polar_yp{};   // polar motion y, radians
};

class EopTable {
 public:
  static EopTable load_from_csv(const std::filesystem::path& path);
  [[nodiscard]] EopValues query(Time<tags::TT> t) const;
  // ... iterators / row count for testing ...
 private:
  std::vector<EopRow> rows_;
};

}  // apsis::time
```

API change is observable: `convert<UT1, UTC>(t)` becomes
`convert<UT1, UTC>(t, eop)`. `transform<ITRS, ICRF>(state, t)` becomes
`transform<ITRS, ICRF>(state, t, eop)`. Conversions that don't touch
EOP (TAI ↔ TT, TAI ↔ UTC, TT ↔ TDB) are unchanged.

**LOC budget**: ~300–400 lines including the CSV parser. If the
implementation exceeds 3× this, STOP and report per
`qrspi_implement_procedure.md` T4.

**Verify**:
- [ ] `grep -rn "g_default_dut1\|g_polar_motion_xp\|g_polar_motion_yp" --include='*.cc' --include='*.h' src include` returns nothing.
- [ ] `EopTable::load_from_csv("data/iers_eop_phase1.csv")` succeeds; `query(known_epoch)` returns row values within 1 µas (polar) / 1 µs (DUT1).
- [ ] Local CI parity green; `ctest --test-dir build` 48/48 + new EOP unit test pass; sanitizer build green.

---

## Batch C: SphericalHarmonic quality

> **Goal**: make `SphericalHarmonic` pass the VE-contract conformance gate
> with analytical Pines/Holmes-Featherstone partials, and apply the
> body-fixed rotation correctly so non-zonal coefficients can ship
> without masking. Sequenced C1 → C2 because the rotation must be in
> place before non-zonal-touching tests are meaningful, and C2's
> analytical Jacobian transforms via `J_icrf = R^T J_bf R` (so C1's
> rotation must already be available at the API).

### C1. `SphericalHarmonic` ICRF→body-fixed rotation (issue #11)

> Closes: REQ-PHY-001 (real Earth gravity), Phase 1 plan §"Out of scope"
> entry for the SH frame rotation.

**Files**:
- `include/apsis/force/spherical_harmonic.h` — constructor accepts an `EopTable&` (consumed via `transform<ITRS, ICRF>` machinery from Batch B).
- `src/force/spherical_harmonic.cc` — `acceleration()` and `partials_dadx()` take the inertial position, transform to body-fixed via the EOP-aware `transform`, evaluate Cunningham V/W in body-fixed, then transform back.
- `tests/unit/force/spherical_harmonic_test.cc` — extended with a non-zonal test (e.g. tesseral C_{2,2} only) whose acceleration must be aligned with the body-fixed grid, not the inertial grid.
- `docs/wiki/decisions/009-hand-rolled-integrator-family.md` — append SH-rotation closure to the Phase 1 Implementation Note (parallel to the closures C2 / D1 / D2 will add).

**Algorithm**:
1. Compute `R_BF←ICRF` from EOP (uses `transform<ITRS, ICRF>` machinery internally).
2. `r_bf = R_BF←ICRF · r_icrf`.
3. Run Cunningham V/W recursion in body-fixed (existing code, unchanged).
4. `a_icrf = R_BF←ICRF^T · a_bf` (rotation is orthogonal so transpose = inverse).
5. For partials: `J_icrf = R_BF←ICRF^T · J_bf · R_BF←ICRF`.

**Dependency**: requires Batch B's `EopTable` API. Don't start before B merges.

**Verify**:
- [ ] Existing zonal-only tests still pass (R_BF←ICRF rotation around the polar axis is a no-op for zonal C_{n,0} coefficients to working precision).
- [ ] New tesseral C_{2,2}-only test produces an acceleration whose direction tracks the body-fixed equator-bulge, **not** the inertial Z-axis (rotation observable).
- [ ] Local CI parity green; sanitizer green.

### C2. Analytical SH gradient + re-include in VE-contract conformance gate (issue #7)

> Closes: REQ-PHY-004 (Pines or equivalent), REQ-PHY-016 (VE contract on every force model — SH side).

**Files**:
- `src/force/spherical_harmonic.cc` — `partials_dadx()` rewritten as the analytical gradient.
- `include/apsis/force/spherical_harmonic.h` — `kAnalyticalPartials = true`.
- `tests/conformance/force_model_ve_contract.cc` — `SphericalHarmonic` re-included in the parameterised test list now that `kAnalyticalPartials == true`. The existing tripwire `static_assert(!SphericalHarmonic::kAnalyticalPartials, ...)` flips to `static_assert(SphericalHarmonic::kAnalyticalPartials, ...)` (or is deleted with rationale).
- `docs/wiki/decisions/009-hand-rolled-integrator-family.md` — Phase 1 Note updated with C2 closure.
- `docs/wiki/sources/orekit-holmes-featherstone-impl.md` (new) — wiki source page citing Orekit's `HolmesFeatherstoneAttractionModel.java` (Apache-2.0) as the open-source reference implementation.

**Algorithm**: Pines (1973) and Holmes-Featherstone (2002) both yield
non-singular gradients via V/W-style polynomial recursion; the
implementer picks whichever cleanly extends the existing Cunningham V/W
skeleton in `spherical_harmonic.cc`. The Pines/Cunningham/Vallado/M-G
references are paywalled and not in the corpus. **The canonical
Apsis-side reference is Orekit's `HolmesFeatherstoneAttractionModel.java`
(Apache-2.0)** at <https://gitlab.orekit.org/orekit/orekit> (file
`src/main/java/org/orekit/forces/gravity/HolmesFeatherstoneAttractionModel.java`).
The implementer studies the Java code's gradient computation, ports the
algorithm with attribution in the C++ source comment + the wiki source
page citation. The license-compatible attribution path keeps the
project's "every factual claim traces to a source" convention intact.

**Tolerance framing**: empirical-tune during implementation. The
existing PointMass conformance tolerance in `force_model_ve_contract.cc`
is the sibling reference; SH's polynomial recursion adds round-off
proportional to recursion depth (degree 20 ≈ ~12-13 sig figs after
Cunningham). Set initial tolerance at 1e-6 relative (matching PointMass)
and tighten if empirical residual permits; loosen with documented
reason if not. **Do not re-introduce FD-vs-FD comparison**.

**No fallback**. If the gradient implementation proves fragile, **STOP
and report under T4**. The orchestrator decides between extending the
cycle, descoping to deg ≤ 4 (where a hand-derived gradient is
tractable), or deferring C2 again. The Phase 1A pass exists precisely
to close this, so an in-plan FD fallback would re-create the silent-
scope-cut pattern this hardening sprint is designed to prevent.

**Dependency**: requires C1 (uses the body-fixed rotation in the gradient transform).

**Verify**:
- [ ] VE-contract conformance test runs over `{PointMass, ThirdBody, SphericalHarmonic}` (3 adapters, was 2) and all pass.
- [ ] `kAnalyticalPartials == true` for `SphericalHarmonic` (verified by `grep` and by the conformance test no longer skipping the adapter).
- [ ] C1's tesseral test still passes (gradient consistent with the rotated acceleration).
- [ ] Local CI parity green; sanitizer green.

---

## Batch D: integrator family

> **Goal**: replace `Dp54` with the originally-specified `Dop853`
> (Hairer Vol I Table 5.2) and add the originally-specified
> `GaussJackson8` (Berry-Healy 2004 ordinate-form). Both upgrades
> restore the integrator family the structure outline and ADR-009
> promised. **Note**: the 10-yr JPL DE round-trip restoration is
> **not** in this batch — that requires also re-enabling the lunar
> third-body and the Encke-on/off pair (a composite deliverable beyond
> integrator scope), which stays a Phase 7 acceptance criterion as the
> Phase 1 plan documented.

### D1. True DOP853 — Hairer Vol I Table 5.2 coefficients (issue #5)

> Closes: REQ-INT-001 (DOP853 family), ADR-009 promise of Hairer Table 5.2 fidelity.

**Files**:
- `include/apsis/integrate/dop853.h` (new) — `Dop853` class behind `IIntegrator`.
- `src/integrate/dop853_coeffs.h` (new) — `constexpr std::array` Table 5.2 coefficients.
- `src/integrate/dop853.cc` (new) — adaptive RK8(5,3) step + blended-error PI step controller.
- `tests/conformance/integrator_kepler.cc`, `integrator_phi.cc` — `Dop853` added to the parameterised list (now `{Dp54, Dop853, Yoshida4}` — `Dp54` retained as a smaller-stencil reference adapter for cross-checking).
- `tests/regression/leo_kepler_24h.cc` — tolerance retuned per Phase 1 plan §10's anticipation ("Phase 7 DOP853 upgrade is expected to drop this by ~7 orders"). Target: 1e-7 m / 1e-10 m/s with 1× margin if empirically achievable; otherwise document the achieved residual.
- `docs/wiki/decisions/009-hand-rolled-integrator-family.md` — Phase 1 Note updated with D1 closure.

**Step controller**: Hairer Vol I §II.5 (specifically the DOP853
discussion) defines the **blended error norm** that combines the 5th-
and 3rd-order embedded estimates. This is materially different from
DP5(4)'s single-embedded controller; "just retune Dp54's controller"
is wrong framing. The implementer reads Hairer §II.5 + cross-checks
against the Hairer Fortran source `dop853.f` for the exact blend
weights.

**Coefficient verification (compile-time `static_assert`)**:
- Row sums of `A` equal `c`: `Σ_j a_ij == c_i` for every i (consistency).
- `Σ_i b_i == 1` (1st-order condition on the 8th-order weights).
- `Σ_i b̂_i == 1` (1st-order condition on the embedded 5th-order weights).
- `Σ_i b_i c_i == 1/2` (2nd-order condition on the 8th-order weights).
- (Higher-order conditions Σ_i b_i c_i^k = 1/(k+1) for k=2..7 are nice-to-have but heavier; the four above catch typos.)
- Reference: Hairer Vol I §II.1 "Order conditions" + Table 5.2 itself.

**Source**: textbook coefficients from Hairer Vol I Table 5.2 are
~16-digit; if the conformance gate proves tight enough that 16 digits
is insufficient, fetch higher-precision constants from the official
Hairer Fortran source (<http://www.unige.ch/~hairer/prog/nonstiff/dop853.f>)
and cite in `dop853_coeffs.h`. Hash the table at compile time so the
source-of-truth doesn't drift.

**Verify**:
- [ ] Coefficient `static_assert`s above all hold at compile time.
- [ ] Kepler conformance tightens vs `Dp54`: 1-period closure < 1e-7 m at rtol=1e-12 (originally targeted by Phase 1 plan §6).
- [ ] Φ conformance tightens vs `Dp54`: residual < 1e-3 m / 1e-6 m/s over 1 hour at rtol=1e-12.
- [ ] LEO Kepler 24h regression's tolerance retuned (target ~1e-7 m or document the achieved residual).
- [ ] All existing tests still pass with `Dop853` substituted at appropriate sites.
- [ ] Local CI parity green; sanitizer green.

### D2. Berry-Healy 2004 ordinate-form Gauss-Jackson 8 (issue #6)

> Closes: REQ-INT-002 (GJ8 multi-step), ADR-009 promise of Berry-Healy implementation.

**Files**:
- `include/apsis/integrate/gauss_jackson_8.h` (new — re-introduces the type that Phase 1 deleted).
- `src/integrate/gauss_jackson_8.cc` (new) — eighth-order multi-step second-sum integrator per [[wiki/sources/berry-healy-2004-gauss-jackson]].
- `src/integrate/gj8_starter.cc` (new) — startup procedure.
- `src/integrate/gj8_coeffs.h` (new) — predictor / corrector / second-sum coefficient tables.
- `tests/conformance/integrator_{kepler,phi}.cc` — `GaussJackson8` re-added to the parameterised list (now `{Dp54, Dop853, GaussJackson8, Yoshida4}`).
- `docs/wiki/decisions/009-hand-rolled-integrator-family.md` — Phase 1 Note updated with D2 closure.

**Starter (canonical)**: f-and-g series per Berry-Healy 2004 §5.1, using
the existing helper in `src/math/f_and_g_series.{h,cc}` (Phase 1 §7).
Berry-Healy's canonical procedure propagates the eight back-points
symmetrically (j = −4..+4) using f-and-g; this is what the GJ8
literature treats as "the starter." **Alternative**: use `Dop853` from
D1 to bootstrap the back-points instead — a deviation from the
canonical paper but acceptable for non-singular orbits where f-and-g
converges easily anyway. **Default to canonical f-and-g**; flip to
DOP853 starter only if f-and-g startup proves numerically unstable in
the conformance test grid (near-circular eccentric or near-parabolic
geometries).

**Coefficient verification**: `gj8_coeffs.h` table compared at
compile-time `static_assert` against a packed checksum derived from
the UMD generator output cited in
[[wiki/sources/berry-healy-2004-gauss-jackson]] §"References" (UMD
handle `http://hdl.handle.net/1903/2202`). The implementer either
captures the generator output in a comment table for the hash, or
cites a known-good prior implementation (e.g. NASA GMAT `GaussJackson`
class — NOSA license, study-permitted).

**Dependency**: D2's f-and-g starter depends on the Phase 1 helper (already merged); does not depend on D1. The DOP853 starter alternative is opt-in and does not block D2 on D1.

**Verify**:
- [ ] Coefficient checksum matches the UMD-generator reference (compile-time `static_assert`).
- [ ] Kepler conformance: GJ8 at fixed Δt=60 s closure < 1e-5 m over 1 period.
- [ ] Φ conformance: GJ8 Φ via central-difference perturbation matches at the same tolerance as Kepler.
- [ ] Predictor-only (PEC) and predictor-corrector (PECE) modes both pass conformance; production default is PECE.
- [ ] All existing tests + D1 tests still pass.
- [ ] Local CI parity green; sanitizer green.

---

## Verification (per batch)

Per `qrspi_implement_procedure.md` §2.3, every batch's implementer runs
the exact CI commands locally before pushing. Per-batch automated
gates list above. Cross-batch:

### Manual (across all batches; the user's signoff)
- [ ] PR bodies for all four batches include the `## Plan deviations` section per `pr_creation_procedure.md`.
- [ ] Each batch's clear-eyes review (`qrspi_review_procedure.md`) has reported and findings resolved.
- [ ] After Batch D merges: re-read `docs/phase-1-plan.md` and confirm every checkbox is `[x]` or has a documented deferral; update ADR-009's Phase 1 Implementation Note to a "Phase 1 + 1A Implementation Note" that records what landed when.
- [ ] **Re-tag GitHub issue titles for closed items** from "Phase 7" to "Phase 1A": #5, #6, #7, #11. Issues #13, #14, #15 were filed as Phase 1 hardening; close on PR-merge per the per-batch boundary.
- [ ] **Phase 7 issues updated** to reflect what 1A closed: the JPL DE 10-year horizon restoration becomes a composite Phase 7 issue tied to #5 (DOP853), the lunar third-body re-enable, and the Encke-on/off pair.

## Out of scope

These items remain deferred; they're not in this hardening pass:

- **JPL DE round-trip 10-year restoration.** The 10-yr / 100 km / 1 mm/s
  case from the original Phase 1 plan §9 requires (a) DOP853
  (closed by D1), (b) lunar third-body re-enabled in the test, (c)
  Encke-on/off variant pair. (a) lands in 1A; (b) and (c) stay Phase
  7. After 1A merges, the 1-yr horizon may become achievable and
  should be tried as a sanity check; the 10-yr case is a composite
  Phase 7 acceptance criterion.
- **Issues #2 (Windows CI), #8 (real ISS reference), #9 (full DE440 fetch tooling), #10 (Yoshida4 co-integrated Φ), #12 (Battin f(q) restoration)** — Tier 3 from the close-out triage. Independent improvements; not Phase 2 prerequisites.
- **gcov coverage gate enforcement at ≥80%** — currently the gate is wired but soft; flipping to hard is a Phase 2 prerequisite (or a Phase 1B cycle), not Phase 1A.
- **REQ-traceability lint flip from soft to hard** — same. Phase 2 introduces enough new test surface that the soft → hard transition has a meaningful denominator.

## Codegen / fallback notes

- **C2 (analytical SH gradient)**: **no in-plan fallback**. STOP-and-report under T4 if the implementation proves harder than expected. Hiding "fragile, ship FD" as an in-plan fallback re-introduces the silent-scope-cut pattern QRSPI was designed to prevent.
- **D1 (DOP853 coefficients)**: if textbook 16-digit coefficients are insufficient for the conformance gate, fetch higher-precision constants from Hairer's official Fortran source. Hash the table at compile time so the source-of-truth doesn't drift.
- **D2 (GJ8 starter)**: if f-and-g startup proves numerically unstable in the conformance test grid, flip to the DOP853 starter (D1 is in scope of the same batch / immediately preceding it). Document the choice in `gj8_starter.cc` with the empirical residual that motivated the flip.
- **B1 (EOP CSV format)**: the bundled `data/iers_eop_phase1.csv` uses the IERS finals2000A column convention. If `EopTable::load_from_csv` parsing turns out to fight the format, the fallback is a simpler CSV in `data/iers_eop_phase1a.csv` with documented columns. **Note**: this throws away parser code at the Phase 7 real-IERS-finals integration; preferred is to handle the real format correctly in 1A and avoid the rework.

## Cycle plan

4 cycles, one per batch. Per `qrspi_implement_procedure.md`:

1. **Cycle 1 — Batch A**: cut `phase-1a-batch-a-vocab` worktree → implement subagent → clear-eyes review subagent → PR → merge.
2. **Cycle 2 — Batch B**: cut `phase-1a-batch-b-eop` off updated main → implement → review → PR → merge.
3. **Cycle 3 — Batch C**: cut `phase-1a-batch-c-sh` off updated main → implement → review → PR → merge. Depends on Batch B.
4. **Cycle 4 — Batch D**: cut `phase-1a-batch-d-integrators` off updated main → implement → review → PR → merge.

After Cycle 4 merges, Phase 1 + 1A is solid; Phase 2 brainstorming opens.
