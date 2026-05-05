---
type: source
title: "Unscented Filtering for Spacecraft Attitude Estimation"
raw_path: docs/raw/papers/crassidis-2003-ukf-attitude.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Crassidis, John L.; Markley, F. Landis]
publication_date: 2003-07
venue: "Journal of Guidance, Control, and Dynamics 26(4):536-542"
filter_name: USQUE (UnScented QUaternion Estimator)
---

# Crassidis & Markley (2003) — Unscented Filtering for Spacecraft Attitude Estimation

The canonical paper for applying the [[concepts/unscented-kalman-filter|Unscented Kalman Filter]] to spacecraft attitude estimation. Names the algorithm **USQUE** (UnScented QUaternion Estimator) and shows it converges from large initial errors where the [[concepts/mekf|MEKF]] fails outright. For Apsis this paper is the algorithmic basis for REQ-GNC-005 (UKF as alternative orbit estimator) and a strong candidate for an MEKF-fallback in contingency / acquisition modes.

## The MEKF failure mode that motivates USQUE

§"Introduction" and §"Simulation Examples". MEKF's linearization is valid only for small attitude errors. With large initial errors — at acquisition, after long star-tracker keepout, contingency reset — the linearization fails. **Figure 3** shows the EKF *never* converging from `(−50°, 50°, 160°)` initial attitude error plus 20°/hr bias error on a TRMM simulation; USQUE converges to <0.1° in 3.5 orbits. This is the regime where the unscented transform's full-nonlinear propagation pays off.

For nominal small-error operation (Simulation 1, no initial error), the two filters agree to 1 µrad — no advantage to USQUE in the linear regime, and it costs ~2.5× as much CPU.

## The naïve "UKF on quaternions" doesn't work

§"Unscented Attitude Filter" first paragraph. The unscented-transform predicted mean (Eq. 7) is a weighted *sum* of sigma-point quaternions — sums of unit quaternions are not unit. Brute-force normalization breaks the mean-and-covariance match the unscented transform is supposed to provide.

The fix: use a 3-component **generalized Rodrigues parameter (GRP)** error vector δp (Eq. 20):

```
δp = f · δq / (a + δq₄)
```

with `a ∈ [0, 1]` selecting the singularity location (180° at a=0, 360° at a=1) and `f` a scale factor. Choosing `f = 2(a+1)` linearizes ‖δp‖ to attitude angle for small errors. Recommended: `a=1, f=4` ("4× MRP"). Don't use `a > 1` — error vector loses physical meaning.

Multiplicative composition with the nominal quaternion is preserved exactly. Full UKF runs on the 6-D state `[δp̂, β̂]` (3-D GRP error + 3-D gyro bias); the 4-D nominal quaternion is propagated separately and updated multiplicatively from the converged δp̂.

## USQUE algorithm structure

§"Unscented Attitude Filter", Eqs 30-45. Each step:

1. Generate `2n+1 = 13` sigma points from Cholesky of `(P + Q̄)`.
2. For each sigma point, convert χ^δp(i) → error quaternion δq(i) via Eq. 33, multiply with current nominal `q̂(0)` to get sigma-point quaternion `q̂(i)` (Eq. 32b).
3. Propagate each `q̂(i)` by closed-form quaternion kinematics (Eqs 28-29) using bias-adjusted angular velocity `ω̂(i) = ũ - χ^β(i)`.
4. Recover propagated error quaternions `δq⁻(i) = q̂⁻(i) ⊗ [q̂⁻(0)]⁻¹`, convert back to GRP via Eq. 20, repack into sigma points (Eq. 37).
5. Standard UKF reconstruction (Eqs 7-13) → predicted mean and covariance.
6. Standard Kalman update (Eq. 2) → δp̂(+), β̂(+).
7. Multiplicative quaternion update `q̂(+) = δq(+) ⊗ q̂⁻(0)` (Eq. 44).
8. **Reset δp̂ to zero** for next propagation.

Same incremental-update-and-reset pattern as [[concepts/mekf|MEKF]] — the difference is in propagation (sigma points vs linearization), not in update structure.

## Trapezoid-rule process noise (§"Unscented Filtering", final paragraphs, Eq. 14)

```
Φ(Δt) Q̄_k Φᵀ(Δt) + Q̄_k = G_k Q_k G_kᵀ
```

A trapezoid-rule integration of the continuous Q over the interval — adds Q̄ at sigma-point generation *and* in the predicted covariance, more accurate than additive-only at endpoints. Eq. 42 gives the closed form for the [[concepts/farrenkopf-gyro-model|Farrenkopf gyro model]]:

```
Q̄_k = (Δt/2) [ (σ_v² - ⅙ σ_u² Δt²) I₃   0₃ ; 0₃   σ_u² I₃ ]
```

(with `σ_v` ↔ ARW, `σ_u` ↔ RRW — same notation as the [[concepts/farrenkopf-gyro-model]] page, just letter-renamed.)

## Computational cost

§"Unscented Attitude Filter" final paragraph. USQUE is **~2.5× slower than MEKF**, reducible to **~2× via Cholesky lower-triangular trick** (only first 3 columns have nonzero attitude entries, can skip computation of `q̂(i)` for half the sigma points). For Apsis at 100 Hz attitude estimation, both fit easily in budget.

For Apsis with [[concepts/pinocchio-library|Pinocchio]]'s [[concepts/analytical-rbd-derivatives|analytical derivatives]], the textbook "EKF needs Jacobians; UKF doesn't" advantage is reduced — Apsis can compute Jacobians cheaply. The remaining advantage of USQUE is robustness to large errors, not Jacobian avoidance.

## Tuning recommendations

§"Simulation Examples", Tables 1-2:

- **`a=1, f=4`** — recommended GRP parameters (4× MRP). `a=0` is Gibbs vector, `a=1` is MRP. Don't go above 1.
- **`λ=1`** is a safe default sigma-point scale. `λ=3-n` is fourth-order-Gaussian-optimal but the post-propagation distribution isn't Gaussian, so the optimum drifts. Worth tuning per scenario.
- **`a=3, λ=3`** is unstable (covariance loses positive-definiteness); use scaled unscented transformation (Julier 2002) if pushed there.

## Apsis relevance

- **REQ-GNC-005** (UKF as alternative orbit estimator, S priority) — algorithmic basis. Adaptation needed for orbital state (no quaternion gymnastics; standard 6-D position+velocity state).
- **REQ-GNC-003** (MEKF for attitude) — this paper is the alternative for cases where MEKF fails. Worth considering for Apsis as an MEKF-fallback in safe / acquisition modes (see arch-flag below).
- **REQ-SEN-005** (gyro/IMU with bias drift, ARW, RRW) — this paper uses the same [[concepts/farrenkopf-gyro-model]] as [[sources/lefferts-1982-mekf]] (Eq. 25 here).
- **subsystems §5.3** ("EKF / UKF for orbit") — UKF derivation pattern.

## Surfaced for human review (no silent spec edits)

1. **MEKF can fail to converge from large initial errors or with poor measurement geometry.** REQ-GNC-003 mandates MEKF without a fallback. Subsystems §5.3 doesn't address this. Worth considering USQUE as the contingency / safe-mode / initial-acquisition variant. Implementation cost: ~2× MEKF; algorithmic addition is bounded.

2. **The "UKF avoids Jacobians" advantage is reduced for Apsis** because [[concepts/pinocchio-library|Pinocchio]] offers [[concepts/analytical-rbd-derivatives|analytical derivatives]] at near-EKF cost. For attitude-only the UKF still wins on robustness against large errors; for orbit estimation with the full force-model Jacobian, the analytical-vs-sigma-point cost is closer than the textbook treatment suggests. Implication: REQ-GNC-005's "UKF as alternative" framing is correct, but the *reason* to prefer UKF is robustness to nonlinearity, not Jacobian avoidance — worth a sentence in subsystems §5.3.
