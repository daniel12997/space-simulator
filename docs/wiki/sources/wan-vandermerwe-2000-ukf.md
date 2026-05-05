---
type: source
title: "The Unscented Kalman Filter for Nonlinear Estimation"
raw_path: docs/raw/papers/wan-vandermerwe-2000-ukf.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Wan, Eric A.; van der Merwe, Rudolph]
publication_date: 2000-10
venue: "IEEE Adaptive Systems for Signal Processing, Communications, and Control Symposium (AS-SPCC)"
doi: 10.1109/ASSPCC.2000.882463
---

# Wan & van der Merwe (2000) — UKF for nonlinear estimation

The widely-cited modern reference for the **Unscented Kalman Filter** in its general (non-attitude-specific) form. Builds on Julier & Uhlmann's seminal proposal (next ingest) by extending the UKF to **parameter estimation** and **dual estimation** (joint state + parameter). Introduces the modern **scaled unscented transform** with the `(α, β, κ)` tuning triple that is now standard. Its Algorithm-3 formulation of the augmented-state UKF is the reference that most modern implementations cite.

## What's new vs Julier & Uhlmann 1997

- **State estimation, parameter estimation, dual estimation**: §1 introduces three nonlinear-estimation problem classes, with UKF formulations for each. Apsis cares about the first; the parameter-estimation framing is useful for online identification of force-model coefficients (e.g., drag `B*` correction).
- **Scaled unscented transform** (§3): replaces Julier's original symmetric sigma-point set with a scaled version using `λ = α²(L + κ) - L`. Three tuning parameters:
  - `α` (typically 1e-3) — spread of sigma points around mean.
  - `κ` (typically 0) — secondary scaling; usually 0.
  - `β` (β = 2 for Gaussian) — incorporates known higher moments of the state distribution.
- **Augmented state** (§3 footnote, Algorithm 3): concatenate state + process noise + measurement noise into an augmented vector `xₐ = [xᵀ vᵀ nᵀ]ᵀ`, generate sigma points in the augmented space. This handles non-additive noise correctly (a common EKF stumbling block).

## Sigma-point construction (Eq. 15)

For state of dimension L:
```
χ₀ = x̄                                            (mean)
χᵢ = x̄ + (√((L+λ) P_x))ᵢ      i = 1..L            (positive sigma points)
χᵢ = x̄ - (√((L+λ) P_x))ᵢ₋ₗ    i = L+1..2L         (negative sigma points)

W₀^(m) = λ/(L+λ)                                  (mean weight, center)
W₀^(c) = λ/(L+λ) + (1 - α² + β)                   (cov weight, center)
Wᵢ^(m) = Wᵢ^(c) = 1/(2(L+λ))    i = 1..2L         (off-center weights)
```

`(√P)ᵢ` is the i-th row of a matrix square root (Cholesky in practice).

## Why UKF beats EKF (§3 Figure 1)

Figure 1 illustrates the unscented transform on a 2D nonlinear function: actual transformed distribution vs EKF-linearized vs UT-propagated. The UT preserves mean and covariance to **third order** for any nonlinearity (Taylor expansion); EKF preserves only first order. For non-Gaussian inputs, UT is accurate to at least second order. Same computational complexity as EKF.

No Jacobians or Hessians needed.

## Apsis relevance

- **REQ-GNC-005** (orbit estimation EKF/UKF) — this is the canonical UKF reference for general nonlinear estimation problems. Apsis's orbit estimator should use the augmented-state, scaled-UT formulation per Algorithm 3.
- **Tuning defaults**: `α = 1e-3`, `β = 2` (Gaussian noise), `κ = 0`. Apsis subsystems §5.3 should make these the documented defaults; users can override per scenario.
- **Dual estimation for online force-model ID** (extension of REQ-GNC-005): could online-estimate `B*` (drag) or solar-radiation-pressure coefficient `C_R` jointly with state. The dual-estimation framework here is the path.
- **Pairs with [[sources/crassidis-2003-ukf-attitude]]**: that paper's USQUE is the *attitude-specific* application; this paper is the *general* UKF. Both belong in the [[concepts/unscented-kalman-filter]] umbrella.

## Cross-references

- [[concepts/unscented-kalman-filter]] — the algorithm.
- [[concepts/kalman-filter]] — the broader KF family.
- [[sources/julier-uhlmann-1997-ukf]] — the original UKF proposal (next ingest).
- [[sources/crassidis-2003-ukf-attitude]] — attitude-specific USQUE variant.
