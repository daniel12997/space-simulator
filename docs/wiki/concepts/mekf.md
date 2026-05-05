---
type: concept
canonical_name: "Multiplicative Extended Kalman Filter"
aliases: [MEKF, multiplicative EKF]
created: 2026-05-04
updated: 2026-05-04
---

# Multiplicative Extended Kalman Filter (MEKF)

The standard spacecraft-attitude estimator since the early 1980s. An extended Kalman filter whose key trick is to keep the **estimate** of attitude as a unit [[concepts/quaternion-attitude-representation|quaternion]] (4 components, on-manifold) while the **covariance** lives on the 3-D tangent space at the current estimate (vector part of a small error quaternion). This separation gives a non-singular 6×6 covariance with direct physical interpretation, while keeping the attitude estimate globally valid and free of singularities — the best of both worlds, given that [[concepts/quaternion-attitude-representation|Stuelpnagel]] proved no three-parameter representation can be both.

[[sources/lefferts-1982-mekf]] is the seminal reference. For Apsis, MEKF is mandated by REQ-GNC-003 (architecture §3 GNC stack; subsystems §5.3).

## State

```
x = [ q̂̄ ; b̂ ]      estimate state, dimension 7 (4-component quaternion + 3-component bias)
δx = [ δq ; Δb ]    error state, dimension 6 (3-component small-rotation vector + bias error)
P̃ = E[δx δxᵀ]      covariance, 6×6, non-singular
```

The attitude estimate `q̂̄` is a unit quaternion. The error quaternion `δq̄ = q̄ ⊗ q̂̄⁻¹` has small vector part `δq` (≈ half-angle of small rotation about each body axis) and scalar part `δq₄ ≈ 1 - ½|δq|²`. Only `δq` is carried in the covariance state.

`b̂` is the [[concepts/farrenkopf-gyro-model|gyro bias]] estimate; `Δb` is its error. Optional additional state components (scale factors, misalignments, etc.) extend the state with their own additive errors.

## Prediction (between measurements)

Estimated angular velocity from gyro measurement `ũ` and bias estimate:

```
ω̂ = ũ - b̂                                                     (Eq. 65)
```

Quaternion propagation by the kinematics ODE, integrated over the gyro sample interval:

```
d q̂̄/dt = ½ Ω(ω̂) q̂̄          where Ω(ω̂) is the 4×4 quaternion-update matrix
```

For constant `ω̂` over the interval (the typical small-step assumption), the closed-form solution `q̂̄(t+Δt) = M(Δθ̂) q̂̄(t)` from [[sources/lefferts-1982-mekf]] Eq. 47 is what the implementation actually uses.

Bias estimate is constant across the prediction (no measurement → no information):

```
b̂(t+Δt) = b̂(t)
```

Covariance propagates by the linearized error dynamics ([[sources/lefferts-1982-mekf]] Eqs 137-139):

```
d δq/dt = -ω̂ × δq - ½ Δb - ½ n₁         (Eq. 135 with sign convention)
d Δb/dt = n₂

F̃ = [ -[ω̂ ×]   -½ I ; 0   0 ]            (Eq. 138)
G̃ = [ -½ I   0 ; 0   I ]                  (Eq. 139)

P̃(t+Δt) = Φ̃ P̃(t) Φ̃ᵀ + N̄(t, t+Δt)        Riccati propagation
```

`Φ̃` is the transition matrix corresponding to `F̃`, `N̄` is the integrated process noise, `n₁` is the gyro angle-random-walk, `n₂` is the gyro rate-random-walk (see [[concepts/farrenkopf-gyro-model]]).

## Update (at each measurement)

For each measurement (typically scalar — vector measurements decomposed into orthogonal scalars per [[sources/lefferts-1982-mekf]] §5):

```
H̃ = ∂h/∂(δq, Δb) | x̂(-)              measurement Jacobian, 1×6
K̃ = P̃(-) H̃ᵀ / (H̃ P̃(-) H̃ᵀ + R)        Kalman gain, 6×1
δx(+) = K̃ · (z - h(x̂(-)))             error-state update
P̃(+) = (I - K̃ H̃) P̃(-) (I - K̃ H̃)ᵀ + K̃ R K̃ᵀ      Joseph form, numerically stable
```

Then the **incremental composition + reset** pattern ([[sources/lefferts-1982-mekf]] Eqs 158-160) — the heart of the MEKF:

```
δq̄(+) = [ δq(+) ; √(1 - |δq(+)|²) ]      reconstruct full error quaternion
q̂̄(+) = δq̄(+) ⊗ q̂̄(-)                    apply multiplicatively to estimate
b̂(+) = b̂(-) + Δb(+)                      additive bias update
δq(+) → 0,  Δb(+) → 0                    reset error state
```

The error state is **always zero immediately after an update.** The linearization point moves with each measurement. This is what keeps the small-rotation assumption valid across an entire mission.

## Why multiplicative beats additive

[[sources/lefferts-1982-mekf]] §§7-11 compares three covariance parameterizations. All produce identical *predictions*; they differ in numerical robustness:

| Form | Dimension | Singular? | Notes |
|---|---|---|---|
| Additive `Δq = q̄ - q̂̄` | 7×7 | **Yes** — rank deficient by 1 from `q̄ᵀΔq̄ = 0` | Round-off erodes singularity → can yield negative eigenvalues |
| Truncated (delete a component of `q̄`) | 6×6 | No | Must swap which component is deleted as `q̄` rotates; large errors when deleted component is small |
| **Multiplicative `δq̄ = q̄ ⊗ q̂̄⁻¹`** (MEKF) | 6×6 | **No** — by construction | Direct physical interpretation; structural unit-norm preservation under composition |

In finite precision the multiplicative form is the only one that's robust without special handling. The architecture's subsystems §5.3 captures this in one sentence: *"Error parameterization is a 3-vector (small rotation), so the covariance is non-singular."*

## Quaternion renormalization

Propagation is linear in `q̂̄` so unit-norm is preserved analytically; float round-off drifts it. Periodic renormalization by `q̂̄ ← q̂̄ · (3 + q̂̄ᵀq̂̄) / (1 + 3 q̂̄ᵀq̂̄)` ([[sources/lefferts-1982-mekf]] Eq. B-20) reduces a norm error of ε to ε³/32 — fourth-order convergence, essentially free per call. Apply every few seconds of mission time, or on demand before any operation that depends on unit norm (attitude matrix evaluation, log/exp on SO(3)).

## Sequential scalar updates

[[sources/lefferts-1982-mekf]] §5 last paragraph: vector measurements (e.g., a star-tracker quaternion or unit vector) decompose into orthogonal scalar measurements. Process them one at a time, recomputing the Kalman gain after each. Avoids the explicit `(HPHᵀ + R)⁻¹` matrix inversion — both faster (no matrix inverse) and more numerically stable (no near-singular inversion when measurements have wildly different noise levels).

## What the MEKF does NOT estimate (typical Apsis configuration)

The standard MEKF state is `[q̄, b]`. Optional state extensions seen in practice:

- **Scale factor errors** on each gyro axis — needed if the gyro is mis-calibrated.
- **Star tracker boresight misalignment** — slowly varying, observable from the residuals of a good star tracker.
- **Magnetometer hard-iron / soft-iron** — when magnetometer is in the loop.

Out-of-scope at the MEKF level (handled separately): orbit estimation (use a separate orbit EKF / UKF — Apsis's REQ-GNC-004), thrust calibration, slosh-state estimation. The clean separation between attitude and orbit estimation is standard practice; coupling between them is at the application layer, not in the filter.

## When MEKF isn't enough — USQUE (UKF for attitude)

The EKF-style local linearization fails when the error grows large between updates — typically after a long gap with no attitude sensor (star tracker keepout during a slew), at acquisition with poor a-priori, or with a single vector measurement (TAM-only mode). Two remedies:

- **Crank up the process noise** during the gap so the covariance grows realistically; the estimator then trusts the next measurement strongly.
- **Switch to a [[concepts/unscented-kalman-filter|UKF]] variant** — [[sources/crassidis-2003-ukf-attitude]] derives **USQUE** (UnScented QUaternion Estimator), which combines the [[concepts/unscented-kalman-filter|unscented transform]]'s sigma-point propagation with the multiplicative quaternion error and a generalized-Rodrigues-parameter (GRP) representation of the 3-D error vector. Cost is ~2-2.5× MEKF; the gain is convergence from arbitrarily large initial errors. [[sources/crassidis-2003-ukf-attitude]] Figure 3 shows MEKF *never* converging from `(−50°, 50°, 160°)` initial error + 20°/hr bias; USQUE converges in 3.5 orbits.

For Apsis the MEKF is the default for nominal operation; USQUE is a strong candidate for safe-mode / acquisition / contingency. REQ-GNC-005 mandates UKF for orbit (S priority); for attitude it's not currently in REQ-* but worth considering.

## References

- **[[sources/lefferts-1982-mekf]]** — the seminal paper that names the MEKF and derives all three covariance forms.
- **[[sources/markley-2003-attitude-error-representations]]** — Markley's later survey of error parameterizations (already in the corpus, not yet ingested).
- **Markley & Crassidis (2014)** — *Fundamentals of Spacecraft Attitude Determination and Control*, Springer. Comprehensive textbook treatment of MEKF and variants. INDEX-only paywalled.
