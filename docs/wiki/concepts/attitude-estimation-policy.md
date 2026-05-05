---
type: concept
canonical_name: Attitude Estimation Policy
aliases: [attitude estimator family, attitude estimation under uncertainty, attitude estimator mode logic]
created: 2026-05-05
requirements: [REQ-GNC-003, REQ-GNC-014, REQ-GNC-016]
sources: [lefferts-1982-mekf, markley-2003-attitude-error-representations, crassidis-2003-ukf-attitude]
decisions: [004-hybrid-attitude-estimation-mode-logic]
---

# Attitude Estimation Policy

The umbrella concept for Apsis's attitude estimation: which estimator is active when, what triggers switches, how covariance hands off, and what guarantees survive transitions. The two algorithms ([[concepts/mekf|MEKF]] and [[concepts/usque|USQUE]]) live as peer concepts; this page is the operational policy that ties them together.

For algorithmic detail see the per-algorithm pages. For the architectural decision *why* Apsis runs both estimators (rather than pinning to one), see [[decisions/004-hybrid-attitude-estimation-mode-logic|ADR-004]].

## The two algorithms in one paragraph

[[concepts/mekf|MEKF]] linearises the attitude-error dynamics around the current estimate (small-angle assumption); [[concepts/usque|USQUE]] propagates `2n+1` sigma points through the full nonlinear attitude-and-bias dynamics. MEKF is half the compute and has longer flight heritage; USQUE handles large initial errors where MEKF can fail to converge ([[sources/crassidis-2003-ukf-attitude|Crassidis & Markley 2003]] demonstrated this on a single-magnetometer TRMM simulation: 8 h non-convergence vs 30 min convergence). Both share the global-quaternion / 3-vector-MRP-error parameterisation, so their covariance is structurally identical and direct hand-off is lossless (under the Gaussian assumption both filters share).

## Hybrid mode logic

Apsis's canonical default is **boot-USQUE plus NIS-monitored MEKF**:

### Acquisition phase

The estimator boots in **USQUE** mode. This handles the typical scenario where initial attitude is poorly known (e.g., post-separation tumbling, post-anomaly recovery) and MEKF's small-angle assumption would be violated.

### USQUE → MEKF (convergence trigger)

When all of:

- `||MRP_estimate|| < 10°` (attitude-error magnitude small enough that MEKF's small-angle assumption is safe), AND
- `trace(P_attitude) < threshold_acquired` (covariance is well-bounded — defaults to a value corresponding to ~1° standard deviation),

the policy hands off to MEKF. **Direct covariance copy** (Q4 design): the manager moves `(q_global, P_attitude, b_gyro, P_bias)` to MEKF unchanged, with the MRP-flavoured GRP parameterisation matching exactly between the two filters.

### Nominal phase

**MEKF** runs as the default; the manager continuously monitors innovation consistency via a **Normalized Innovation Squared (NIS) test**:

```
NIS_k = z̃_k^T S_k^{-1} z̃_k
```

with `z̃_k` the innovation and `S_k` the predicted innovation covariance. Under MEKF's Gaussian assumption, `NIS_k` is chi-squared distributed with degrees-of-freedom equal to the measurement dimension. Apsis flags inconsistency when `NIS_k > χ²_{p=0.99, dof=meas_dim}` for `N=5` consecutive samples (configurable).

### MEKF → USQUE (inconsistency trigger)

On sustained inconsistency, the manager **warm-restarts USQUE** with MEKF's current estimate as the initial condition. Direct covariance copy again — USQUE accepts any positive-definite covariance via the Cholesky-based sigma-point generation.

After the warm restart, the policy returns to the acquisition→nominal sequence: USQUE runs until the convergence trigger fires, then MEKF resumes.

### Mission-FSM override

The broader GNC mode FSM (REQ-GNC-009 — detumble / sun-pointing / science / safe / etc.) can **pin** the estimator to a specific algorithm via `AttitudeEstimator::pin_mode(EstimatorMode)`. Use cases:

- **Permanent USQUE** for missions with permanently uncertain attitude (long-baseline interferometry, cluster spacecraft, certain debris-tracking platforms).
- **Permanent MEKF** for missions where USQUE compute cost is unacceptable (very high attitude rates, severely constrained CPU).
- **Phase-pinned**: detumble pins USQUE; science pins MEKF; transitions are FSM-driven rather than policy-driven.

When pinned, the autonomous policy is bypassed; `unpin()` restores autonomous operation.

## Architecture

```cpp
class AttitudeEstimator : public Estimator {  // existing Estimator interface
public:
    AttitudeEstimator(AttitudeEstimationConfig);
    void predict(const Time& t) override;
    void update(const Measurement& m) override;
    EstimatedState current_estimate() const override;

    EstimatorMode current_mode() const;          // diagnostic
    void pin_mode(EstimatorMode);                // FSM override hook
    void unpin();
private:
    Mekf mekf_;
    Usque usque_;
    AttitudeEstimationPolicy policy_;            // pure logic
    EstimatorMode active_mode_;
    std::optional<EstimatorMode> pinned_;
};
```

Three internal seams:

- **MEKF** and **USQUE** are individual `Estimator`-conforming algorithm classes. Independently testable and individually swappable for variants (e.g., second-order MEKF vs first-order; CDKF vs UKF).
- **AttitudeEstimationPolicy** is a pure-logic class — takes covariance traces and innovation NIS history as inputs; returns mode decisions. No dependencies on the algorithm classes; no external I/O. Trivially unit-testable against synthetic histories.

The manager itself is the deep module: external callers see one `Estimator` satisfying the existing interface; the algorithm + policy structure is hidden behind it.

## Hand-off mechanism

Both algorithms maintain `(q_global ∈ S³, P_MRP ∈ R^{3×3}, b_gyro ∈ R³, P_bias ∈ R^{3×3})`. The mandate that USQUE uses the `(a=1, f=1)` MRP-flavoured GRP from [[concepts/generalized-rodrigues-parameters|Crassidis-Markley]] makes the parameterisations algebraically identical, so hand-off is a direct copy of the four state blocks.

No covariance inflation, no sample-and-resample ceremony. The Q3 convergence trigger guards against handing MEKF a covariance it can't handle (USQUE→MEKF only when error is small); the inverse direction (MEKF→USQUE) is unconditionally safe (USQUE accepts any positive-definite covariance).

## Validation invariants

The policy class has clean test surfaces because it's pure logic:

- **Acquisition triggers**: synthetic covariance histories crossing the threshold should produce mode transitions exactly at the crossing.
- **NIS detector**: synthetic innovation streams with known chi-squared characteristics should produce false-alarm rates matching the configured `p` threshold.
- **Persistence**: `N=5` consecutive samples — single excursions should not trigger; sustained excursions should.
- **Hand-off lossless within Gaussian**: mean and covariance preserved exactly across direct-copy hand-off (modulo `f64` LSB).
- **End-to-end Monte Carlo**: a large-initial-error scenario where MEKF-alone fails should reliably converge under the hybrid policy. This is a CI regression gate (REQ-OBS-005, REQ-GNC-014).

## Tuning knobs (with defaults)

| Knob | Default | Purpose |
|---|---|---|
| `acquisition_mrp_threshold` | 10° (`tan(10°/4)` MRP magnitude) | USQUE → MEKF: MRP magnitude criterion |
| `acquisition_trace_threshold` | 1° σ-equivalent | USQUE → MEKF: covariance trace criterion |
| `nis_chi2_p` | 0.99 | MEKF NIS test: false-alarm probability |
| `nis_consecutive_n` | 5 | MEKF NIS test: persistence requirement |
| `usque_alpha` | 1e-3 | USQUE scaled UT spread |
| `usque_beta` | 2 | USQUE scaled UT prior moment (optimal for Gaussian) |

All are scenario-configurable.

## See also

- [[concepts/mekf]] — small-angle algorithm.
- [[concepts/usque]] — sigma-point algorithm.
- [[concepts/generalized-rodrigues-parameters]] — MRP-flavoured GRP shared by both filters.
- [[concepts/farrenkopf-gyro-model]] — gyro model both filters consume.
- [[sources/lefferts-1982-mekf]] — MEKF foundational paper.
- [[sources/markley-2003-attitude-error-representations]] — error-representation taxonomy.
- [[sources/crassidis-2003-ukf-attitude]] — USQUE foundational paper; documents the MEKF-failure scenarios that motivate the hybrid policy.
- [[decisions/004-hybrid-attitude-estimation-mode-logic]] — ADR documenting the hybrid-policy choice.
