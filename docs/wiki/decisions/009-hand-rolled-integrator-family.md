---
type: decision
title: "Hand-rolled DOP853, Yoshida-4, and Gauss-Jackson 8 integrator family behind one IIntegrator seam"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: [berry-healy-2004-gauss-jackson]
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Phase 1 architectural decision.

## Context

The propagator core needs an `IIntegrator` seam that supports the
state-plus-Φ step contract from `docs/structure.md` Phase 1:

```
IIntegrator::step(state, Φ, dt) -> (state', Φ')
```

Three regimes must be covered, per the design overview's emphasis on
[[concepts/long-arc-state-conditioning|long-arc conditioning]]:

- **Adaptive RK** for general non-stiff orbital ODEs and
  [[concepts/variational-equations|variational-equations]] propagation
  between measurements ([[decisions/002-variational-equations-between-measurements]]).
- **Symplectic** for gravity-only long arcs where energy preservation
  matters more than per-step accuracy.
- **Multi-step** ([[concepts/gauss-jackson-integration|Gauss-Jackson 8]])
  for second-order orbital ODEs in the operationally common case where
  step size is fixed and the force evaluation is the cost driver.

The design overview commits to "structure-preserving integrators" as
part of the long-arc precision strategy; the seam must accommodate all
three families uniformly.

## Decision

Implement three integrators **hand-rolled in `src/integrate/`**, all
behind a common `IIntegrator` interface and covered by a single
parameterised conformance test:

- **`Dop853`** — Dormand-Prince 8(5,3) adaptive Runge-Kutta. Coefficients
  from Hairer-Nørsett-Wanner Vol. I Table 5.2. Adaptive stepsize via the
  embedded 5th-order solution. ~600 LOC.
- **`Yoshida4`** — fourth-order symplectic composition of the velocity-Verlet
  flow (Yoshida 1990). Used for gravity-only long-arc cases where energy
  drift matters. ~150 LOC.
- **`GaussJackson8`** — eighth-order multi-step second-sum integrator per
  [[sources/berry-healy-2004-gauss-jackson]]. Uses Dop853 for the
  starter. ~400 LOC including starter wiring.

All three step `(state, Φ, dt) -> (state', Φ')` by augmenting the state
vector with the columns of Φ and integrating in lockstep with the
underlying physical state. The Φ derivative is `∂a/∂x · Φ`, requiring
the `IForceModel::partials_dadx` from the VE contract.

## Rationale

- The augmented `(state, Φ)` step contract is the natural form for the
  VE roll-forward mandated by ADR-002. Boost.Odeint exposes its
  observer-pattern step interface around a single state vector, which
  forces us to either (a) flatten Φ into the state every step (loses
  type clarity, awkward sizes), or (b) write a custom Odeint stepper
  that replicates DOP853 anyway. The latter is no smaller than a
  hand-rolled DOP853.
- DOP853 coefficients are textbook and stable for decades. Implementing
  them is a transcription task with property-based tests; not a
  research project.
- GJ8 is not provided by Boost.Odeint or SUNDIALS in a form we can
  directly reuse. Berry-Healy 2004 gives a complete algorithmic
  description suitable for direct implementation.
- A hand-rolled family keeps the codebase under our control — debugging
  numerical behaviour for a 50-year arc means owning the integrator
  internals, not chasing them across a third-party API.
- Compile times stay low (no Boost transitive include explosion).

## Alternatives considered

- **Boost.Odeint adapter family.** Rejected per the contract-fit
  argument above. Also drags Boost into the dependency tree at Phase 1,
  which is otherwise avoidable.
- **SUNDIALS CVODE.** Excellent for stiff problems; overkill for
  non-stiff orbital ODEs; doesn't help with VE contract. Heavy build
  weight.
- **Pin one integrator (e.g. just DOP853).** Rejected because the design
  overview commits to symplectic for gravity-only long arcs and to
  GJ8-class multi-step for the operational fixed-step regime; one
  integrator can't cover all three regimes well.
- **Use an autograd / templated integrator that derives ∂a/∂x via
  forward-mode AD on the force model.** Considered; would let us drop
  the explicit `partials_dadx` requirement on `IForceModel`. Rejected
  because (a) AD across SPICE-backed third-body terms is intractable
  without rewriting the SPICE seam, (b) analytical partials are well-known
  for our force models and faster than AD.

## Consequences

- `IForceModel` must expose `partials_dadx` as a hard contract member,
  not optional. Adapters that lack analytical partials (none currently
  planned) would have to provide a finite-difference fallback — flagged
  but not implemented in Phase 1.
- The conformance test parameterised over `{Dop853, Yoshida4,
  GaussJackson8}` runs on a Kepler problem and checks: (a) solution
  matches f-and-g-series Keplerian closed-form to integrator tolerance;
  (b) Φ propagated by the integrator agrees with `∂x(t)/∂x(0)` derived
  via central-difference perturbation of initial conditions to the same
  tolerance. This is the *only* mechanism by which a new integrator
  adapter is admitted to the seam.
- Coefficient tables are committed as `constexpr std::array` in headers.
  (An earlier draft of this ADR specified hashing them in CI to prevent
  silent corruption; that mechanism has not been wired and the claim is
  withdrawn here. Phase 7 may add it once the full DOP853 / GJ8 tables
  land.)
- Encke-wrapper composition ([[concepts/long-arc-state-conditioning]]) is
  a separate module that takes any `IIntegrator` and reframes it as a
  deviation propagator.

## Phase 1 Implementation Note (2026-05-05)

Phase 1 lands the IIntegrator seam, the Φ augmentation, the Encke
wrapper, and the conformance tests as committed above, but ships a
narrower adapter set than the steady-state vision:

- The adaptive RK adapter that ships in Phase 1 is **`Dp54`** —
  Dormand-Prince 5(4), Hairer Vol I Table 5.1. The full DOP853
  (Hairer Vol I Table 5.2) is deferred to Phase 7. The seam, the
  PI step controller, the Φ augmentation, and the conformance gate
  are unchanged at the upgrade; only the coefficient table and the
  per-step error weights move. The Phase 1 conformance tolerances
  are widened accordingly (Kepler closure < 1 m / 1 period at rtol
  1e-12 instead of the < 1e-7 m the plan originally targeted; the
  upgrade rebuilds those by ~7 orders of magnitude).
  Naming the class `Dp54` (rather than re-using `Dop853`) is
  intentional: the previous `Dop853` alias over a DP5(4) coefficient
  table was a load-bearing lie — a future maintainer would have read
  "Dop853" and assumed Hairer Table 5.2 fidelity.

- The Berry-Healy 2004 ordinate-form Gauss-Jackson 8 implementation
  is **deferred to Phase 7**. The `GaussJackson8` adapter and its
  conformance test were removed in Phase 1 because the seam-only
  stand-in (a single Dp54 step under the GJ8 name) carried zero
  distinct behaviour and the conformance test under both names was
  the same integrator twice. Phase 7 reintroduces the type behind
  `IIntegrator` with the second-sum starter and the ordinate-form
  predictor-corrector, at which point GJ8 rejoins the parameterised
  conformance gate.

- Analytical partials for `SphericalHarmonic` (the Pines gradient)
  are deferred to Phase 7. The Phase 1 SH adapter ships with a
  central-difference `partials()` and is **excluded** from the VE-
  contract conformance test parameterisation (the test runs on
  `{PointMass, ThirdBody}` only). `PointMass` and `ThirdBody`
  partials are analytical in Phase 1 as ADR-009 requires. Each
  adapter carries a `static constexpr bool kAnalyticalPartials`;
  the conformance test loops only over those that claim `true`,
  and a `static_assert` on `SphericalHarmonic::kAnalyticalPartials
  == false` in the conformance source guards the disclosure.

- The Phase 1 `ThirdBody` adapter's `acceleration()` was changed in
  service of fix-set landing this note: it now returns the
  conventional Vallado §8.7.2 / Montenbruck-Gill §3.3.2 form
  `mu_3 * ((r_3 - r) / |r_3 - r|^3 - r_3 / |r_3|^3)` rather than the
  Battin / f(q) stable substitution that was previously wired. The
  switch was made for clarity and to keep the analytical Jacobian
  directly verifiable against the literature derivation; an earlier
  draft of this note characterised the prior Battin form as having a
  numerical sign discrepancy in the LEO-vs-Sun regime, but that claim
  was not substantiated against an independent oracle and has been
  withdrawn — both expressions describe the same vector field
  algebraically, and absent a smoking-gun unit test the substitution
  is best framed as a clarity/maintainability change rather than a
  bug fix. REQ-PHY-005's "must be numerically stable at small
  spacecraft-central-body distances" is unaffected at LEO (the
  conventional-form cancellation costs ~5 of ~16 sig figs, well above
  the 1e-6 conformance tolerance); the Battin substitution returns as
  a Phase 7 hardening item (tracked separately) if conjunction-
  screening close approaches require it.

The IIntegrator and IForceModel seams as documented above are
unchanged at the Phase 7 upgrades; these notes describe the fidelity
of what is wired behind the seams, not changes to the seams
themselves.

## Phase 1A Implementation Note — Batch C closures (2026-05-05)

Phase 1A Batch C closes the two `SphericalHarmonic` Phase-1 deferrals
that the Phase 1 note above documented as Phase 7 follow-ups:

- **C1 — ICRF→body-fixed rotation (issue #11)**. The Phase-1 SH adapter
  treated body-fixed and ICRF as aligned (a Phase-7-deferred shortcut
  documented in the file header). It now takes a `const EopTable&` at
  construction, queries the rotation matrix via the new
  `apsis::frames::icrf_to_itrs_rotation(tt, eop)` helper (a public
  factoring of the per-pair builder used by `transform<ITRS, ICRF>`),
  rotates the input ICRF position to body-fixed for the Cunningham V/W
  evaluation, and rotates the resulting acceleration back via the same
  matrix's transpose. The position-Jacobian is conjugated through the
  same matrix: `J_icrf = R^T J_bf R`. A new tesseral C_{2,2}-only unit
  test (`SphericalHarmonic.C22RotationObservable`) verifies the rotation
  is observable: at a fixed ICRF position, the acceleration vector
  differs by ≥ 1e-5 m/s² between two TT epochs separated by 6 sidereal
  hours, because the C_{2,2} body-fixed bulge has rotated underneath.
  The Phase 1 implementation would have produced the same vector at
  both epochs.

- **C2 — analytical SH gradient (issue #7)**. The Phase-1 SH adapter
  shipped with `kAnalyticalPartials = false` and a finite-difference
  `partials_dadx()`, excluded from the VE-contract conformance gate
  because both sides would have been FD-of-the-same-gradient (a
  tautology). The Cunningham gradient (Hessian of the geopotential via
  the same V/W functions extended by one row) is now in place: the V/W
  table is computed up to row N+2 (one further row than the
  acceleration assembly needs), and the Jacobian assembly applies the
  V/W partial-derivative identities (Montenbruck & Gill 2000 §3.2.5,
  eq 3.33) to each per-(n, m) acceleration term by linearity. The
  flag flipped to `kAnalyticalPartials = true` and SphericalHarmonic
  re-joined the conformance gate (`tests/conformance/force_model_ve_contract.cc`).
  Empirical residual on the J2-only conformance grid is ~1e-10
  (deg=8, full-order-tesseral synthetic case is ~2.4e-10), four to
  five orders below the 1e-6 PointMass-sibling tolerance applied; the
  residual is dominated by the FD oracle's truncation `(h²/6) |∂³a/∂r³|`
  rather than the analytical V/W recursion, which at deg ≤ 20 retains
  13–14 sig figs.

  **Algorithm choice**: Cunningham V/W (M-G §3.2.5) over Holmes-
  Featherstone or Pines. The plan permitted "whichever cleanly extends
  the existing Cunningham V/W skeleton"; the Cunningham gradient *is*
  the existing skeleton with one extra row of V/W and an isomorphic
  per-(n, m) sum, so the LOC and conceptual delta were minimal.
  Holmes-Featherstone uses a different basis (`P̄_{nm}/u^m`) that would
  have required a from-scratch port of Orekit's
  `HolmesFeatherstoneAttractionModel.java`; that file is cited in the
  source comment as the open-source sibling that was studied during
  implementation, with a wiki source page at
  [[sources/orekit-holmes-featherstone-impl]] for the audit-trail
  attribution. Both formulations are mathematically equivalent and
  produce identical accelerations and Jacobians to round-off at
  Apsis's working ceiling (deg 70 LEO / deg 165 lunar — well within
  Cunningham's stable range, which only loses precision around
  deg ≥ 1500 where Holmes-Featherstone becomes mandatory).

The Phase 1 note above is left in place verbatim for the historical
record; this Phase 1A note supersedes its Phase 7-deferral entries
for `SphericalHarmonic`. The remaining Phase 7 items (full DOP853 +
GJ8) are tracked separately under Batch D of the Phase 1A hardening
plan.

## Phase 1A Implementation Note — Batch D1 closure (2026-05-06)

Phase 1A Batch D1 closes the Phase-1 DOP853 deferral that the original
Implementation Note above documented as a Phase 7 follow-up:

- **D1 — true DOP853 (issue #5)**. The Phase-1 adaptive-RK adapter shipped
  as `Dp54` (Hairer Vol I Table 5.1) with the `Dop853` name retired to
  remove the load-bearing-lie; the full DP8(5,3) coefficient table (Hairer
  Vol I Table 5.2) and blended-error PI step controller (§II.5) are now
  in place behind a sibling `Dop853` adapter. The adapter sits behind the
  same `IIntegrator` seam as `Dp54`, `Yoshida4`, and (D2) `GaussJackson8`;
  `Dp54` is retained as a smaller-stencil reference for cross-checking.

  **Coefficient source**: transcribed directly from Hairer's official
  Fortran source `dop853.f`
  (<http://www.unige.ch/~hairer/prog/nonstiff/dop853.f>; public domain via
  INRIA / Universite de Geneve), which is the canonical machine-readable
  form of Table 5.2 to ~30 decimal digits. The transcription is in
  `src/integrate/dop853_coeffs.h` with file-level attribution. Compile-time
  `static_assert`s verify (a) row sums of A equal c, (b) sum(b) == 1,
  (c) sum(bhh) == 1, (d) sum(b * c) == 1/2 — the four cheap order
  conditions that catch typos. A defence-in-depth FNV-1a hash of the full
  table is also computed at compile time so a silent drift to the constants
  produces a conspicuous diff.

  **Step controller**: blended-error PI per Hairer §II.5. Uses both the
  5th-order embedded estimate (via `er` weights) and the 3rd-order embedded
  estimate (via `bhh1, bhh2, bhh3` applied to stages 1, 9, 12); the blended
  norm is `|h| * sqrt(sum_err / (N * (sum_err + 0.01 * sum_err2)))`, then
  Lund-stabilised step factor `(err)^(1/8 - 0.2*beta) / facold^beta` per
  dop853.f line ~677. Default `beta = 0.0` reduces to a plain I-controller
  matching Hairer's recommended default.

  **Empirical residuals (development host)**:
  - Kepler 1-period at rtol=1e-12 / atol=1e-9 / dt_max=600 s: **~4e-6 m**
    (asserted bound 5e-5 m; ~10x margin). The 1-period closure is bounded
    by `rtol * orbit_radius` ~ 1e-12 * 7e6 = 7e-6 m at this rtol, so the
    method is at the floor; tightening rtol below 1e-12 hits double-
    precision noise. ~5 orders below Dp54's 1 m bound on the same problem.
  - Phi 1-hour at default Options: **~1e-3 m / 1e-6 m/s**, dominated by
    the central-difference perturbation order (h^2 truncation in the FD
    oracle), not the DOP853 step error (which is well below 1 nm).
  - LEO Kepler 24-h regression at rtol=1e-12 / atol=1e-9 / dt_max=60 s:
    **~1.9e-5 m / ~2.1e-8 m/s** (asserted bound 2e-4 m / 2e-7 m/s; ~10x
    margin). About 3 orders below Dp54's prior ~1.5e-2 m residual on the
    same problem.

  **Plan-anticipated vs achieved**: the Phase 1 plan §10 anticipated
  "~7 orders of magnitude" tightening for the DOP853 upgrade. The achieved
  tightening is ~3 orders on the LEO 24-h regression and ~5 orders on the
  1-period Kepler conformance. The shortfall is explained by the rtol
  floor noted above: at rtol=1e-12 with a 7000 km orbit radius, per-step
  error is bounded by `rtol * r` regardless of the method's order, so
  DOP853's per-step truncation is no longer the dominant residual term.
  The §10 anticipation implicitly assumed an idealised constant-relative-
  error regime that doesn't apply once rtol approaches the working
  precision floor. The tolerance retune in `leo_kepler_24h.cc` reflects
  this empirical reality and is documented in the test file header.

- **D2 — Berry-Healy 2004 ordinate-form Gauss-Jackson 8 (issue #6)**:
  **DEFERRED to a follow-on cycle.** Per the implement procedure's T4
  STOP-and-report trigger, both coefficient-source bridges flagged in
  the Phase 1A hardening plan §"Codegen / fallback notes" failed inside
  the available cycle context budget:

  1. Direct transcription of Berry-Healy's printed Tables 5 (90 entries)
     and 6 (90 entries) from the corpus PDF was attempted via
     `pdftotext -layout` and visual readout; both lost the printed table's
     subtle minus signs in some rows, and a row-sum-zero consistency
     check fired on transcription (rows j = -1 and j = +1 of Table 5
     summed to ±0.0698 instead of 0; Table 6 row sums of several rows
     were further off the expected 1/12 alternate-formulation residual).
     A single-entry typo could be debugged but the audit-trail to the
     paper printed values would require more rounds than the cycle
     budget supported.
  2. Compute-from-first-principles via the Adams-Moulton (Eq 26),
     Adams-Bashforth (Eq 31), Stormer-Cowell (Eq 43), and Stormer-predictor
     (Eq 48) recurrences in exact rational arithmetic, plus the
     mid-corrector backward-difference recurrence Eq 59 and the
     ordinate-form transformation Eq 67. The recurrences match Berry-Healy's
     Tables 3 and 4 exactly (verified by `static_assert` on c_1, c_2, c_4,
     q_1..q_4) but the conversion to alternate-formulation ordinate Tables
     5 and 6 has an unresolved shift convention that disagrees with the
     printed anchor values `b_{4,4} = -19087/89600` and `a_{4,4} =
     3250433/53222400` by approximately the centre-term shift factor.

  Resolution path forward (orchestrator decides):
  - Extend the cycle and complete a hand-checked transcription against a
    higher-resolution Berry-Healy 2004 reprint or against Berry's UMD
    generator output (`http://hdl.handle.net/1903/2202` — the page is
    bot-blocked for unauthenticated access from CI hosts but a one-time
    human-mediated download lands the canonical reference).
  - Defer D2 to a follow-on hardening cycle (`phase-1a-batch-d-prime` or
    similar) with the same scope; D1's DOP853 is independently useful
    in this batch and unblocks Phase 2 active-spacecraft narrow-phase
    work that does not require GJ8.
  - Descope to predictor-only (PE) at lower order; rejected because PE
    inherits the same coefficient-table dependency as PECE.

  The IIntegrator seam is unchanged at the eventual D2 upgrade; the
  GaussJackson8 type slot is reserved per ADR-009 above. Phase 2 work
  may proceed on the active-spacecraft side using DOP853; the
  catalog-scale propagation case (50k-object SGP4-style) D2 was
  originally targeting is a Phase 5 / Phase 6 concern, not blocked by
  the D2 slip.

## Phase 1A Implementation Note — Batch D' cleanup (2026-05-05)

Phase 1A Batch D' is a follow-on cleanup batch addressing five
non-blocking findings from the Batch D clear-eyes review. Each is
tracked separately in this ADR rather than as new architectural
decisions:

- **D'1 (regression-tolerance tightening)**. Asserted bounds on
  `LeoKepler24h.Dop853MatchesFAndGOracle` and `IntegratorKepler.Dop853`
  retuned from ~10× to ~5× the empirical residual. Empirical values on
  the dev host (re-measured by both implementer and clear-eyes reviewer):
  Kepler 1-period 4.150e-6 m; LEO 24h 1.865e-5 m / 2.101e-8 m/s. New
  asserted bounds: 2e-5 m (Kepler) and 1e-4 m / 1e-7 m/s (LEO 24h).
  The Phi-conformance gate is intentionally left at the looser
  ~10× margin because it is FD-oracle-bounded and tightening would
  fight round-off rather than catch real regressions.

- **D'2 (PI controller `facold` persistence)**. Hairer Vol I §II.5's
  PI step controller requires `facold` (the previous accepted step's
  normalised error) to persist across `step()` calls so the I-term has
  memory across the integration. Phase 1A Batch D shipped `facold` as
  a function-local variable re-initialised to `1e-4` at the top of
  every `step()`, defeating the persistence and degrading the PI
  controller to plain-I when `beta != 0`. Fix: `facold_` is now an
  instance member of `Dop853` (initialised to `1e-4` in the
  constructor); `step()` reads on rejects and writes on accepts. The
  default `beta = 0.0` codepath is unaffected (`pow(facold, 0) == 1`),
  so no behaviour-change for the existing tests.

  A `facold_for_test()` accessor on `Dop853` exposes the persisted
  value so the conformance test can assert it. The
  `IntegratorPhi.Dop853WithBeta` test exercises both `beta = 0.0` and
  `beta = 0.04` (Hairer's recommendation), runs a 1-hour Kepler
  integration, and asserts (a) step counts in a coarse band, (b)
  PI/I step-count ratio within 2×, (c) final-state agreement to 1 m,
  (d) `facold_` remains in `[1e-4, 1.0]`, and (e) **inversion-detecting
  assertion**: `facold_ > 1e-3` after the integration. (e) is the
  load-bearing check — under the original bug `facold_` would stay at
  the constructor-seed `1e-4`, so this assertion fires under the bug
  and passes under the fix. Verified by inversion: re-introducing the
  bug (writing the accept-path update to a function-local) produces a
  test failure.

- **D'3 (FNV-1a coefficient-table hash baseline)**. Batch D shipped a
  `kCoefficientHash` constexpr but with only a `static_assert(kCoefficientHash != 0)`
  check, which is not load-bearing for typo detection (any single-bit
  flip changes the hash but no compile-time assert fires on the change).
  Fix: pinned the baseline literal `0xB3EDD5CF93EBB0EEULL` via
  `static_assert`. The misleading comment block at `dop853_coeffs.h`
  rewritten to honestly describe the magnitude-encoding (FNV-1a over
  `int64(|x| * 2^53)` with bitwise-not for negatives), explicitly
  noting the limitation ("catches IEEE-754-distinguishable edits, not
  sub-ULP edits that round to the same double") and disclaiming
  cryptographic use. Verified: flipping a digit in any coefficient
  fires the assert with the rebaseline-script reminder.

- **D'4 (vestigial `Options::dt_initial`)**. The `Dop853::Options::dt_initial`
  field was declared but never read in the implementation; deleted.
  (The matching `Dp54::Options::dt_initial` is also vestigial but is
  out of scope for this cleanup batch — it lands separately if/when
  Dp54 is touched.)

- **D'5 (`recompute_dop853_hash.sh`)**. New helper script at
  `tools/dev/recompute_dop853_hash.sh` builds the coefficient header,
  links it into a tiny printer harness, and emits the `kCoefficientHash`
  literal in the `static_assert` form ready to paste at the pinned
  baseline. Run this script after any *intentional* coefficient edit
  to rebaseline. Verified by both implementer and clear-eyes reviewer
  on the dev host: emits `0xB3EDD5CF93EBB0EEULL` matching the current
  baseline.

The `IIntegrator` seam, the coefficient table contents, and the step
controller's mathematical specification are all unchanged at this
cleanup batch; only the `facold` data flow, the asserted-tolerance
literals, the pinned hash baseline, and one Options field move.
