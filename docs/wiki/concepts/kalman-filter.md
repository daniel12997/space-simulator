---
type: concept
canonical_name: "Kalman Filter"
aliases: [KF, EKF, extended Kalman filter]
created: 2026-05-04
updated: 2026-05-04
---

# Kalman Filter

The umbrella concept for the family of recursive Bayesian estimators that propagate a Gaussian belief about a system state through linear or linearized dynamics and a noisy measurement model. The basic linear KF (Kalman 1960) handles linear systems exactly; the **Extended KF (EKF)** linearizes nonlinear dynamics around the current estimate; the **Unscented KF (UKF)** propagates sigma points through the full nonlinear dynamics; the **Multiplicative EKF (MEKF)** is an attitude-specific EKF variant that uses the multiplicative quaternion error to keep the covariance non-singular.

For Apsis the KF family is the spine of estimation: REQ-GNC-003 mandates [[concepts/mekf|MEKF]] for attitude; REQ-GNC-004 mandates an EKF for orbit; REQ-GNC-005 calls for UKF as an alternative orbit estimator (S priority).

## The basic structure

State equation (continuous form):
```
ẋ(t) = f(x, t) + G(t) w(t)              with E[w(t)] = 0, E[w(t)w(t')ᵀ] = Q δ(t-t')
```

Measurement equation:
```
y_k = h(x_k, k) + v_k                    with E[v_k v_kᵀ] = R_k
```

Two operations alternate:

**Predict** (between measurements): propagate `x̂` via integration of `f`; propagate `P` via the Lyapunov / Riccati ODE `Ṗ = F P + P Fᵀ + G Q Gᵀ` where `F = ∂f/∂x | x̂`. In discrete form, `P(+) = Φ P Φᵀ + Q̄`.

**Update** (at each measurement): compute Kalman gain `K = P Hᵀ (H P Hᵀ + R)⁻¹` where `H = ∂h/∂x | x̂(-)`; update mean `x̂(+) = x̂(-) + K (y - h(x̂(-)))`; update covariance `P(+) = (I - K H) P(-)` (or **Joseph form** `P(+) = (I - K H) P(-) (I - K H)ᵀ + K R Kᵀ` for numerical stability).

## Variants and when to use each

| Variant | Dynamics handling | Measurement handling | Best for | Apsis use |
|---|---|---|---|---|
| Linear KF | Linear `f` exactly | Linear `h` exactly | Linear systems (rare in spacecraft) | Reference for derivations |
| EKF | Linearization `F = ∂f/∂x` | Linearization `H = ∂h/∂x` | Nonlinear with small errors | REQ-GNC-004 orbit estimation |
| MEKF | EKF on a 6-D error state, multiplicative quaternion update | Multiplicative quaternion error | Spacecraft attitude with small errors | REQ-GNC-003, [[concepts/mekf]] |
| [[concepts/unscented-kalman-filter\|UKF]] | Sigma-point propagation through full nonlinear `f` | Sigma points through `h` | Strong nonlinearity, large errors | REQ-GNC-005, [[sources/crassidis-2003-ukf-attitude]] |
| Square-root forms | Same as base, factored P storage | Same | Higher numerical stability, hardware with reduced precision | Optional improvement |

## Joseph form

The naïve covariance update `P(+) = (I - K H) P(-)` is mathematically correct but loses positive-definiteness of P over many iterations due to round-off. The **Joseph form**:

```
P(+) = (I - K H) P(-) (I - K H)ᵀ + K R Kᵀ
```

is symmetric in structure and preserves positive-definiteness numerically, at slight extra cost. Apsis's KF implementations should use Joseph form by default. ([[sources/lefferts-1982-mekf]] Eq. 29 makes this the recommended form.)

## Sequential scalar measurement updates

Vector measurements (e.g., a star-tracker quaternion, a 3-component sun unit vector, a 3-component magnetometer reading) decompose into orthogonal scalar measurements when `R` is diagonal. Process them sequentially — recompute the Kalman gain after each scalar update, then move to the next. **Avoids the explicit `(H P Hᵀ + R)⁻¹` matrix inverse**, which is both faster and more numerically stable when measurement noises differ widely. Standard practice; both [[sources/lefferts-1982-mekf]] §5 and [[sources/crassidis-2003-ukf-attitude]] §"Attitude Kinematics" recommend this for Apsis-style sensor suites.

## What the basic KF assumes (and how Apsis violates each)

- **Linear dynamics** → spacecraft dynamics are nonlinear. EKF linearizes; UKF propagates sigma points; MEKF linearizes the *error*.
- **Gaussian noise** → spacecraft sensor noise is approximately Gaussian for short observation windows; bias drift breaks Gaussianity but is handled by including bias in the state.
- **State on R^n** → quaternion states live on the unit-norm 3-sphere, not R^n. MEKF / USQUE handle this with multiplicative composition.
- **Known noise statistics** → sensor noise PSDs are estimated from manufacturer specs and on-orbit calibration; uncertainties handled by tuning Q upward in conservative modes.
- **Independent noise** → cross-correlations between sensors (e.g., gyro and magnetometer interference) are usually ignored at the filter level and handled by hardware design.

## References

- **Kalman, R.E. (1960)** — *A New Approach to Linear Filtering and Prediction Problems*, ASME J. Basic Engineering 82(D):35-45. The original. INDEX-only.
- **Gelb, A. (ed.) (1974)** — *Applied Optimal Estimation*, MIT Press. The reference textbook for the EKF. INDEX-only.
- **[[sources/lefferts-1982-mekf]]** — MEKF derivation and the multiplicative attitude error.
- **[[sources/crassidis-2003-ukf-attitude]]** — UKF for attitude (USQUE).
- **Markley & Crassidis (2014)** — *Fundamentals of Spacecraft Attitude Determination and Control*, Springer. Comprehensive textbook. INDEX-only.
- **Tapley, Schutz & Born (2004)** — *Statistical Orbit Determination*, Elsevier. The reference for orbit estimation with EKF/UKF. INDEX-only.
