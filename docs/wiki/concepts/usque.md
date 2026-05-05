---
type: concept
canonical_name: Unscented Quaternion Estimator
aliases: [USQUE, UKF-attitude, unscented quaternion estimator]
created: 2026-05-05
requirements: [REQ-GNC-014]
sources: [crassidis-2003-ukf-attitude, julier-uhlmann-1997-ukf, wan-vandermerwe-2000-ukf, markley-2003-attitude-error-representations]
decisions: [004-hybrid-attitude-estimation-mode-logic]
---

# Unscented Quaternion Estimator (USQUE)

The **sigma-point-based attitude filter** Apsis uses as the acquisition-mode complement to the [[concepts/mekf|MEKF]]. Where MEKF linearises the attitude error around the current estimate (small-angle assumption), USQUE propagates a deterministic set of sigma points through the **full nonlinear** attitude-and-bias dynamics, recovering posterior moments by weighted statistics. The result: MEKF is faster and equally accurate when the attitude error is small; USQUE converges from large initial errors where MEKF can fail outright ([[sources/crassidis-2003-ukf-attitude|Crassidis & Markley 2003]] Figure 3 — 8h non-convergence vs 30 min convergence on a single-magnetometer TRMM simulation).

This page describes the algorithm. For *when* USQUE is active vs MEKF and how transitions happen, see [[concepts/attitude-estimation-policy]].

## State and parameterisation

USQUE shares the global-attitude / 3-vector-error split with MEKF:

- **Global attitude state**: 4-component unit quaternion `q ∈ S³`.
- **Error parameterisation**: 3-vector **MRP-flavoured generalised Rodrigues parameters** (the `(a=1, f=1)` instance of the [[concepts/generalized-rodrigues-parameters|Crassidis-Markley GRP]] family — algebraically identical to Markley 2003 MRP).
- **Bias state**: 3-vector gyro bias `b`, additive.

Apsis mandates **MRP-flavoured GRP for USQUE** so the covariance is structurally identical to MEKF's MRP covariance — direct hand-off (no parameterisation conversion) when the [[concepts/attitude-estimation-policy|policy]] switches modes. See [[decisions/004-hybrid-attitude-estimation-mode-logic|ADR-004]] for why this matters.

## Sigma points

For state dimension `n = 6` (3-vector MRP error + 3-vector bias) and tuning parameter `λ` (typically `λ = 3 - n` per Wan-van der Merwe 2000 scaled-UT), the `2n+1 = 13` sigma points are:

```
χ₀     = x̂                                                   (mean)
χᵢ     = x̂ + (√((n+λ) P))ᵢ          i = 1..n                 (positive)
χᵢ₊ₙ   = x̂ - (√((n+λ) P))ᵢ          i = 1..n                 (negative)
```

`(√((n+λ) P))ᵢ` is the i-th column of the matrix square root (Cholesky in practice).

Weights:
```
W₀^(m) = λ/(n+λ)
W₀^(c) = λ/(n+λ) + (1 - α² + β)
Wᵢ^(m) = Wᵢ^(c) = 1/(2(n+λ))    i = 1..2n
```

with `α = 10⁻³`, `β = 2` (optimal for Gaussian) per the modern scaled UT.

Each sigma-point's MRP component is converted to a *delta quaternion* `δq` via the standard MRP-to-quaternion map; the full attitude per sigma point is `q_global ⊗ δq`. This way the sigma points span attitude-space directly, not a flat tangent space.

## Predict step

At each gyro tick `dt`:

1. For each sigma point `χᵢ = (δp_i, δb_i)`:
   - Apply MRP→quaternion to recover sigma quaternion `q_i = q_global ⊗ δq(δp_i)`.
   - Subtract bias from gyro reading: `ω̃_i = ω_meas - (b_global + δb_i)`.
   - Propagate the sigma quaternion via the standard quaternion kinematic equation `q̇ = ½ Ω(ω̃) q` over `dt`.
   - Convert back to MRP relative to a chosen reference (typically the centre sigma point's propagated quaternion).
2. Compute weighted mean and covariance from the propagated sigma points to recover `(x̂_pred, P_pred)`.
3. Add discrete process-noise covariance `Q_d` for the bias drift.

The discrete-time process-noise covariance for the Farrenkopf gyro model (per [[sources/crassidis-2003-ukf-attitude|Crassidis & Markley 2003]] Eq. 42) has a closed form:

```
Q_d = [ (σ_v² dt + ⅓ σ_u² dt³) I₃    ½ σ_u² dt² I₃ ]
      [        ½ σ_u² dt² I₃              σ_u² dt I₃ ]
```

where `σ_v` is the angle random walk (ARW) and `σ_u` the rate random walk (RRW). Apsis uses this closed form rather than discretising via numerical integration.

## Update step

For each measurement (star tracker quaternion, sun-vector body unit vector, magnetometer body field):

1. Pass each sigma point through the measurement model `h(χᵢ)`.
2. Compute the weighted mean predicted measurement and innovation covariance.
3. Compute the cross-covariance `P_xz` between state sigma points and measurement sigma points.
4. Kalman gain `K = P_xz · P_zz⁻¹`.
5. State and covariance update.

After the update, the global quaternion is **rotated by the resulting MRP error** (`q_global ← q_global ⊗ δq(δp)`), and the MRP error is reset to zero — the same multiplicative-update-and-reset pattern MEKF uses.

## Computational cost

Per measurement step:

- MEKF: one `f(x)` and one `h(x)` evaluation; one Jacobian or finite-difference Jacobian per measurement; standard EKF update.
- USQUE: 13 (= 2n+1 for n=6) `f(x)` evaluations; 13 `h(x)` evaluations; sample mean/covariance computation; standard UKF update.

USQUE is roughly **2× MEKF in compute** for typical dimensions. At Apsis's typical 10-100 Hz attitude estimation rate this is well within budget — USQUE is the right default for acquisition phases and for any scenario where attitude uncertainty is large enough that MEKF's small-angle assumption is in doubt.

## When USQUE wins

USQUE converges from initial errors where MEKF either fails or takes hours to settle:

- **Large initial attitude error** (>60° from truth) — MEKF's linearisation around the current estimate captures none of the curvature; USQUE's sigma points sample the actual attitude distribution.
- **Sparse measurements** — single-magnetometer scenarios, where the geometric observability is poor; USQUE's higher-moment treatment of the prior helps the filter avoid getting stuck on the wrong attitude branch.
- **Highly nonlinear measurement geometry** — when measurement Jacobians vary rapidly across the prior covariance, MEKF's local linearisation produces consistent under-confidence; USQUE samples the full geometry.
- **Recovery from anomalies** — a manoeuvre, sensor outage, or unmodelled torque that drives MEKF inconsistent. USQUE's larger basin of convergence allows recovery without manual intervention.

## When MEKF is preferred

For small-angle, well-observed nominal operation (which is most science-mode time): MEKF is half the compute, gives equivalent accuracy, and has a longer flight heritage. The estimation policy in [[concepts/attitude-estimation-policy]] formalises the switching criterion.

## See also

- [[concepts/attitude-estimation-policy]] — operational policy: when each estimator is active, switching triggers, hand-off mechanism.
- [[concepts/mekf]] — the small-angle sibling.
- [[concepts/kalman-filter]] / [[concepts/unscented-kalman-filter]] — broader filter family.
- [[concepts/generalized-rodrigues-parameters]] — the MRP-flavoured GRP parameterisation USQUE shares with MEKF.
- [[concepts/farrenkopf-gyro-model]] — gyro noise model both estimators consume.
- [[sources/crassidis-2003-ukf-attitude]] — canonical USQUE reference.
- [[sources/markley-2003-attitude-error-representations]] — error-representation taxonomy.
- [[sources/wan-vandermerwe-2000-ukf]] — modern UKF (scaled UT).
- [[sources/julier-uhlmann-1997-ukf]] — original UKF.
- [[decisions/004-hybrid-attitude-estimation-mode-logic]] — ADR mandating USQUE as the canonical fallback.
