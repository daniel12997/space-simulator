---
type: concept
canonical_name: Long-arc state conditioning
aliases: [conditioning over precision, two-component time, Encke perturbation, compensated summation, Kahan-Neumaier summation, long-arc precision]
created: 2026-05-05
requirements: [REQ-TIME-002, REQ-TIME-009, REQ-TIME-013, REQ-INT-006, REQ-INT-007]
sources: [sofa-2023-time-scale-cookbook, berry-healy-2004-gauss-jackson]
decisions: [003-tagged-time-scale-types]
---

# Long-arc state conditioning

The architectural principle that lets Apsis preserve `f64` precision over multi-decade orbital arcs and 7-day Pc covariance roll-forwards, without resorting to wider precision (`f128`, multi-precision, BigFloat). Apsis's three pillars of conditioning each address a different way `f64` precision degrades over long arcs:

| Pillar | What degrades without it | Mechanism |
|---|---|---|
| **Two-component time** | Time-of-evaluation precision | Split `(epoch_jd, offset_seconds)`; rectify when `offset` grows |
| **Encke perturbation propagation** | Cancellation between large central-body acceleration and small perturbation | Integrate deviation `δr` from a Keplerian reference, not absolute `r` |
| **Compensated summation** | Accumulated round-off in integrator state-update loop | Kahan-Neumaier compensation per accumulated quantity |

This page is the canonical contract for all three. The architecture doc's design principle *"Conditioning over precision"* (§2) is the operational realisation here.

## Why f64 alone isn't enough

f64 has 52 bits of mantissa — relative precision ~2.2×10⁻¹⁶. At first glance that's plenty for orbital work; in practice three independent precision-loss mechanisms compound over long arcs:

1. **Time precision loss.** A single Julian Date stored as one `f64` near J2000 (≈ 2.45×10⁶ in JD-day units) has ULP ≈ 5×10⁻¹⁰ days ≈ 50 µs. Over a 30-year mission, that drift becomes seconds — meaningless for any frame transformation depending on UT1 / ERA. ([[sources/sofa-2023-time-scale-cookbook|SOFA cookbook §2.3]] is explicit about this; their two-part `(jd1, jd2)` API exists for exactly this reason.)

2. **Cancellation in absolute-state propagation.** A LEO satellite has `|r| ≈ 7×10⁶ m` and `|a_central| ≈ 9 m/s²`; perturbations like J₂ are ~10⁻² m/s², drag at ~10⁻⁷ m/s². Each integrator step computes `r_new = r + v·dt + ½·a·dt²` where `a = a_central + a_perturb`. The dominant term `½·a_central·dt²` (~10⁵ m at 60 s steps) loses its low-order bits to `f64` exponent alignment; the perturbation contribution drops below ULP at modest dt. Direct propagation of absolute `r` discards perturbation effects systematically.

3. **Accumulated round-off in state updates.** Even with conservative steps, the state-update loop `s ← s + Δs` accumulates one ULP per step in a worst-case error model. At 1 Hz integration over 10 years (~3×10⁸ steps), that's ~3×10⁸ × 5×10⁻¹⁰ ≈ 0.15 m of pure FP-arithmetic error in position — independent of any physical model error.

Each pillar below addresses one of these mechanisms. **Using only one pillar leaves the other two as bottlenecks** — they compose multiplicatively, not additively, in the precision budget.

## Pillar 1 — Two-component time (representation conditioning)

Time is stored as `(epoch_jd: f64, offset_seconds: f64)`:

- `epoch_jd` is a reference Julian Date (typically J2000 = 2451545.0 in the relevant scale).
- `offset_seconds` accumulates seconds-since-epoch.
- Together they preserve nanosecond precision for `|offset| ≤ 10⁷ s` (~4 months).
- When `|offset|` exceeds the rectification threshold, the time is **rectified**: `epoch_jd ← epoch_jd + (offset_seconds / 86400.0)` and `offset_seconds ← 0`. Rectification preserves the represented instant exactly (within `f64` of the JD-day arithmetic, which has ULP ~50 ns at the J2000 epoch).

### Tagged at the type level

The `Time` type is templated on its time scale, so scale-mixing bugs become compile errors rather than silent ms-level drifts:

```cpp
template<TimeScale S>
struct Time {
    f64 epoch_jd;
    f64 offset_seconds;
};
using TaiTime = Time<TimeScale::TAI>;
using TtTime  = Time<TimeScale::TT>;
using UtcTime = Time<TimeScale::UTC>;
using Ut1Time = Time<TimeScale::UT1>;
using TdbTime = Time<TimeScale::TDB>;
// optional, for relativistic-strict use:
using TcgTime = Time<TimeScale::TCG>;
using TcbTime = Time<TimeScale::TCB>;

TtTime  tt_from_tai(TaiTime);
TdbTime tdb_from_tt(TtTime);
// ... full conversion graph mediated by SOFA wrappers (iauTaitt, iauDtdb, ...).
```

See [[decisions/003-tagged-time-scale-types]] for the rationale (vs untagged) and [[concepts/time-scales]] for the conversion graph between scales.

### Auto-rectification

Arithmetic operations (`Time + Duration`, `Time - Time`) check the rectification threshold after the operation and rectify lazily if exceeded. The user never has to remember to rectify — the abstraction does its own bookkeeping. The performance cost (one comparison per operation) is negligible vs the precision-loss cost of forgetting (silent ms-level drift).

## Pillar 2 — Encke perturbation propagation (formulation conditioning)

The Encke formulation integrates the **deviation** from an analytical Keplerian reference, not the absolute state. For a perturbed orbit:

```
r_actual(t) = r_reference(t) + δr(t)
```

where `r_reference(t)` is propagated **analytically** (Keplerian conic from the latest rectification epoch) and `δr(t)` is integrated numerically via Battin's form:

```
δr̈ = a_perturb(r_actual, t) - (μ/|r_reference|³) · (r_actual - r_reference)
```

The right-hand side is **small** by construction (the Keplerian-correction term cancels most of the central-body contribution that would otherwise dominate `f64` precision). State magnitudes for `δr` are kilometres (or less) rather than millions of metres; perturbation effects are no longer below ULP.

### Wrapper around any base integrator

Apsis implements Encke as a **wrapper around the existing `Integrator` interface**, not as a standalone integrator class. `EnckePropagator` holds a reference orbit, delegates `δr` integration to the wrapped integrator (DOPRI 8(7), GJ8, Yoshida 8, etc.), and composes `r_actual = r_reference + δr` for output:

```cpp
class EnckePropagator : public Integrator {
public:
    EnckePropagator(std::unique_ptr<Integrator> base, double rectify_threshold = 0.01);
    StepResult step(State& s, double dt_max, const ForceModel& f, EventDetector& ev) override;
    State interpolate(double t_query) const override;  // returns absolute state
    void rectify_reference() override;                  // explicit + auto on threshold
private:
    std::unique_ptr<Integrator> base_;
    KeplerianReference ref_;
    double rectify_threshold_;
};
```

This pattern is **orthogonal to integrator choice** — one Encke implementation, N base integrators. The wrapper exposes absolute-coordinate dense output through `interpolate()`, so downstream consumers (including the [[concepts/variational-equations|Variational Equations integrator]] when querying `(r, v)` for partials evaluation) never need to know Encke is in play.

### Rectification

When `|δr| / |r_reference| > rectify_threshold` (default 0.01 — the textbook 1%), the wrapper rectifies: sets the reference orbit to the current absolute state, resets `δr` and `δv` to zero. Rectification is essentially free (it's just resetting the analytical reference's epoch and elements) and preserves the physical state exactly.

### Engagement policy

Encke is **per-spacecraft opt-in via scenario configuration**. Default off — short-arc / high-perturbation scenarios (LEO with strong drag, capture phases) don't benefit. Long-arc cruise, GEO stationkeeping, and Pc covariance roll-forward over 7-day windows are the typical use cases.

## Pillar 3 — Compensated summation (accumulation conditioning)

Apsis uses **Kahan-Neumaier compensated summation** ([REQ-INT-006](../../../REQUIREMENTS.md)) for every per-step state accumulation. Without it, naive accumulation `s ← s + Δs` accumulates ~1 ULP per step. With it, the error per accumulation drops to a small constant (typically `O(ε)` for the entire summation, regardless of step count).

Two primitives:

```cpp
template<typename T>
class CompensatedSum {              // for individual scalars (Time::offset_seconds, time accumulator, etc.)
public:
    void add(T value);              // Neumaier-corrected
    T value() const;
};

template<typename V>                // for vector / matrix accumulation
class CompensatedAccumulator {      // (V = Eigen::Matrix<scalar, N, M>)
public:
    void add(const V& delta);       // per-coefficient Neumaier via Eigen Array ops
    V value() const;
};
```

Both are **integrator-internal** — nothing exposed at the public `Integrator::step()` interface. Each integrator picks the right tool per accumulator (state vector → `CompensatedAccumulator<Vector6d>`; time → `CompensatedSum<f64>`).

The [[concepts/variational-equations|Variational Equations integrator]] (§3.6) uses `CompensatedAccumulator<Matrix>` for Φ propagation by default. Pc roll-forward windows of 7 days at typical RK4 step sizes accumulate ~10⁵ matrix updates; FP-accumulation error compounds enough that compensation is justified. The one extra Neumaier step per accumulation is negligible vs the matrix-vector products in `A·Φ`.

## How the three pillars compose

The three pillars address **independent, multiplicative** precision-loss mechanisms:

```
total_precision_loss ≈
    time_precision_loss     × { ≈ 1 if pillar 1 used; large otherwise }
  × formulation_precision_loss × { ≈ 1 if pillar 2 used; large otherwise }
  × accumulation_precision_loss × { ≈ 1 if pillar 3 used; large otherwise }
```

Using only one pillar leaves the other two unaddressed. **REQ-TIME-009** (mm-level position precision in CCI frames out to 50 AU) is achievable only via the trifecta. The cross-references in v0.4 of REQUIREMENTS make this explicit.

## Test surfaces

Long-arc precision is verified via a regression test suite that propagates published reference cases over their full reference arc and compares against the published state:

- **Two-body Keplerian over 100 years** — energy and angular momentum should be conserved to within compensated-summation residual (~ULP × √N steps).
- **Earth-orbiting satellite over 1 year** with EGM2008 + sun/moon — compare against a JPL DE-derived reference ephemeris to within published mission tolerances.
- **JPL DE planetary ephemeris over 10 years** — round-trip Apsis force-model integration vs DE-tabulated state; closure error sets the practical conditioning floor.
- **Pc covariance roll-forward over 7 days** — propagate covariance via the Variational Equations integrator with and without each pillar engaged; precision-loss contributions of each pillar quantified.

These tests are CI gates ([REQ-OBS-005](../../../REQUIREMENTS.md), REQ-OBS-006).

## See also

- [[decisions/003-tagged-time-scale-types]] — why `Time` is tagged at the type level.
- [[decisions/009-hand-rolled-integrator-family]] — the integrator family (DOP853, Yoshida-4, Gauss-Jackson 8) that all three pillars feed; the canonical Apsis implementation home for this concept.
- [[decisions/010-phantom-typed-time-and-state]] — how the tagged `Time` and `State` are implemented (phantom tag types) so the two-component-time pillar lands as a type, not a convention.
- [[concepts/time-scales]] — the seven astronomical time scales and the conversion graph between them. Distinct concern from the time *representation* covered here.
- [[concepts/variational-equations]] — the Variational Equations integrator consumes all three pillars (tagged Time, optionally Encke for long Pc roll-forwards, vectorised compensated accumulator for Φ).
- [[concepts/gauss-jackson-integration]] — one of several base integrators that benefits from being wrapped by Encke for long-arc work.
- [[sources/sofa-2023-time-scale-cookbook]] — `(jd1, jd2)` two-part time API; the upstream pattern Apsis's tagged `Time` realises.
- [[sources/berry-healy-2004-gauss-jackson]] — discusses long-arc precision for satellite propagation; one driver for compensated summation.
