---
type: concept
canonical_name: "Unscented Kalman Filter"
aliases: [UKF, unscented filter, UF, unscented transform]
created: 2026-05-04
updated: 2026-05-04
---

# Unscented Kalman Filter (UKF)

A nonlinear extension of the [[concepts/kalman-filter|Kalman filter]] that avoids analytical linearization by propagating a deterministic set of **sigma points** through the full nonlinear dynamics, then reconstructing the posterior mean and covariance from the propagated points. Introduced by Julier, Uhlmann & Durrant-Whyte (2000); the canonical textbook treatment is Wan & van der Merwe (2001).

For Apsis: REQ-GNC-005 mandates UKF as alternative orbit estimator. The attitude variant is [[concepts/mekf|USQUE]] from [[sources/crassidis-2003-ukf-attitude]].

## The unscented transform

Given a Gaussian prior `(x̂, P)` of dimension `n`, the UKF generates `2n+1` sigma points that exactly capture the prior's mean and covariance:

```
χ(0) = x̂                                              center
χ(i) = x̂ + (√((n+λ)P))_i        for i = 1..n          positive offsets
χ(n+i) = x̂ - (√((n+λ)P))_i      for i = 1..n          negative offsets
```

`(√M)_i` denotes the i-th column of a matrix square root of M (Cholesky is the standard choice). The scalar `λ` is a tuning parameter — `λ = 3-n` minimizes 4th-order Gaussian moment error; `λ < 0` risks non-PD covariance reconstruction (fix: scaled unscented transformation, Julier 2002).

Each sigma point is propagated through the *full nonlinear* dynamics:
```
χ_{k+1}(i) = f(χ_k(i), k)
```

and the posterior mean and covariance are reconstructed as weighted sums:

```
x̂⁻_{k+1} = (1/(n+λ)) [ λ χ_{k+1}(0) + ½ Σ_{i=1..2n} χ_{k+1}(i) ]
P⁻_{k+1}  = (1/(n+λ)) [ λ Δχ(0) Δχ(0)ᵀ + ½ Σ Δχ(i) Δχ(i)ᵀ ] + Q̄
```

with `Δχ(i) = χ_{k+1}(i) - x̂⁻_{k+1}`.

## Update step

Apply `h` to each sigma point: `γ_{k+1}(i) = h(χ_{k+1}(i), k)`. Weighted-sum-and-deviation gives the predicted measurement mean `ŷ⁻`, output covariance `P^yy`, and cross-correlation `P^xy`. Innovation covariance `P^vv = P^yy + R`. Kalman gain `K = P^xy (P^vv)⁻¹`. Standard update:

```
x̂(+) = x̂(-) + K (y - ŷ⁻)
P(+) = P(-) - K P^vv Kᵀ
```

(Joseph form variant available; see [[concepts/kalman-filter]].)

## Comparison with EKF

| Property | EKF | UKF |
|---|---|---|
| Cost per step | 1 dynamics + 1 Jacobian eval | 2n+1 dynamics evals |
| Approx order | 1st-order Taylor | 2nd-order moments (Gaussian) |
| Handles non-differentiable f | No | Yes |
| Handles large errors | Poorly | Well |
| Parallelizable per step | No | Yes (sigma points independent) |
| Jacobian implementation | Required | Not needed |

For spacecraft applications:

- **Attitude estimation**: MEKF (linearization on the *error*, which stays small with the reset pattern) is fine for nominal operation; UKF (USQUE) wins at acquisition or large-error contingency. See [[sources/crassidis-2003-ukf-attitude]] Figure 3.
- **Orbit estimation**: EKF is the workhorse; UKF can help when the nonlinearity is severe (low-thrust trajectories with poor a-priori, deep-space optical-only navigation).

## The "UKF avoids Jacobians" caveat for Apsis

Standard textbook framing: UKF is preferable when Jacobians are expensive or non-existent. **For Apsis this advantage is reduced** because [[concepts/pinocchio-library|Pinocchio]] provides [[concepts/analytical-rbd-derivatives|analytical derivatives]] at ~3-5× the cost of one dynamics call ([[sources/carpentier-2018-rbd-analytical-derivatives]]) — competitive with the UKF's 2n+1 dynamics evaluations.

The remaining UKF advantage is **robustness to nonlinearity** and **handling of non-differentiable dynamics** (discontinuous control modes, hard-stop joint limits). For Apsis, choose UKF based on the nonlinearity-of-error story, not the Jacobian-cost story.

## Process-noise integration

[[sources/crassidis-2003-ukf-attitude]] §"Unscented Filtering" final paragraphs derives a trapezoid-rule augmentation of the simple additive Q approach:

```
Φ(Δt) Q̄ Φᵀ(Δt) + Q̄ = G Q Gᵀ
```

This adds Q̄ at both sigma-point generation (Eq. 5a) and in the predicted covariance (Eq. 8) — more accurate than additive-only-at-end. Eq. 42 gives the closed form for the attitude problem with the [[concepts/farrenkopf-gyro-model]].

## References

- **Julier, S.J., Uhlmann, J.K. & Durrant-Whyte, H.F. (2000)** — *A New Method for the Nonlinear Transformation of Means and Covariances in Filters and Estimators*, IEEE Trans. Auto. Control 45(3):477-482. The seminal UKF paper. (Not in Apsis corpus; Julier-Uhlmann 1997 SPIE paper IS — see `julier-uhlmann-1997-ukf.pdf`.)
- **[[sources/julier-uhlmann-1997-ukf]]** (planned next ingest) — earlier conference version of the unscented-transform idea.
- **[[sources/wan-vandermerwe-2000-ukf]]** (planned next ingest) — Wan & van der Merwe textbook chapter, the standard reference.
- **[[sources/crassidis-2003-ukf-attitude]]** — USQUE: UKF applied to spacecraft attitude with multiplicative quaternion error.
- **Julier, S.J. (2002)** — *The Scaled Unscented Transformation*, Proc. American Control Conf. Pittsburgh. The fix for non-PD reconstructed covariance.
