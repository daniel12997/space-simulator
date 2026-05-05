---
type: source
title: "Attitude Error Representations for Kalman Filtering"
raw_path: docs/raw/papers/markley-2003-attitude-error-representations.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Markley, F. Landis]
publication_date: 2003
venue: "Journal of Guidance, Control, and Dynamics 26(2):311-317"
doi: 10.2514/2.5048
---

# Markley (2003) — Attitude error representations for Kalman filtering

The canonical taxonomy of **three-component attitude-error representations** for Kalman-filter use, with emphasis on the [[concepts/mekf|Multiplicative EKF]]. Compares rotation vector, quaternion vector part, [[concepts/generalized-rodrigues-parameters|Gibbs / Rodrigues]] vector, and **Modified Rodrigues Parameters (MRP)** as covariance-state choices. Provides the algebraic relations between them, their kinematic equations, and the consistency conditions an EKF imposes. Also extends MEKF to a **consistent second-order extension** — the only known way to handle highly nonlinear attitude-error growth without breaking quaternion normalization.

## Why this paper exists

The unit quaternion is the lowest-dimension globally-non-singular attitude representation, but the unit-norm constraint conflicts with linear EKF measurement updates. [[sources/lefferts-1982-mekf|Lefferts, Markley & Shuster 1982]] established the now-canonical resolution: **estimate the global attitude as a quaternion, but parameterize the attitude error as a 3-vector with a 3×3 covariance**. This paper formalizes which 3-vector choices are valid and what their tradeoffs are.

## The four candidate error representations

### Rotation vector φ = φe (Eq. 1)

Most direct (`A(φ)` is the rotation matrix), but the kinematic equation is **transcendental and singular at φ = 2π**. Wraps from 0 to 2π and discontinuously back. Limited usefulness as a global rep.

### Quaternion vector part q (Eq. 3)

Quaternion `q = [q_v; q₄] = [e sin(φ/2); cos(φ/2)]`. Vector-part `q_v` alone is a 3-vector, but its norm `≤ 1`. Naturally bounded but loses sign information at φ = π.

### Gibbs vector / Rodrigues parameters g (Eq. 11)

`g = q_v / q₄ = e tan(φ/2)`. **Singular at φ = π** (the q₄ = 0 equator of S³). Useful when bounded rotation magnitudes are guaranteed (small-angle filters, between-update steps).

### Modified Rodrigues Parameters (MRP) p (Eq. 13)

`p = q_v / (1 + q₄) = e tan(φ/4)`. **Singular only at φ = 2π** — twice the dynamic range of Gibbs. The "preferred" three-component representation for attitude filters that may see large errors.

## MEKF derivation rebuild (mid-paper)

For an MEKF, the **error quaternion** `δq = q ⊗ q̂⁻¹` is small (small angles between truth and estimate). Its 3-vector parameterization (any of the above) provides the covariance state. Markley shows:

- For small errors, all four candidates reduce linearly to `2 δq_v` (the small-angle vector component) → all choices are equivalent at first order.
- For larger errors, MRP is the most well-behaved — bounded singularity at 2π.
- The full nonlinear quaternion update **after the linearized 3-vector measurement update** restores the global-attitude consistency. This is the MEKF "covariance reset" step.

## Second-order MEKF extension

Most of the second half develops a **consistent second-order MEKF** — accounts for the second-order term in the error-quaternion algebra so that quaternion normalization is preserved exactly to second order. Important for:

- **Large initial errors** where MEKF can be slow / unreliable to converge ([[sources/crassidis-2003-ukf-attitude]] showed UKF outperforms first-order MEKF here).
- **Long propagation between measurements** where covariance grows large.

The second-order MEKF closes some of the gap to UKF without abandoning the EKF framework.

## Apsis relevance

- **REQ-GNC-003** (MEKF mandate) — this paper is the *implementation* reference for MEKF. The first-order version is what most flight software ships; the second-order extension is a robustness upgrade.
- **MRP as covariance state**: Apsis subsystems §5.3 should make this concrete. Default should be MRP for the 3-component error state (p in Eq. 13), with the 4-component quaternion as the global state.
- **Validation against [[sources/crassidis-2003-ukf-attitude|USQUE]]**: REQ-MC-007 should compare first-order MEKF, second-order MEKF, and USQUE on a Monte Carlo battery to characterize where each fails.
- **Bridges to [[sources/schaub-1998-vscmg]]**: Schaub's Lyapunov controllers commonly use MRPs as the *control state*; Markley provides the *estimation state* in the same parameterization → estimator and controller speak the same language.

## Items for human review (no silent spec edits)

- Subsystems §5.3 should specify the canonical Apsis attitude error representation. Recommend: **MRP for the 3-vector covariance state, quaternion for the global attitude, second-order MEKF as default, USQUE as fallback for large-error / acquisition modes**. Currently the doc just says "3-vector small rotation".

## Cross-references

- [[concepts/mekf]] — the algorithm Markley extends.
- [[concepts/quaternion-attitude-representation]] — the global state.
- [[concepts/generalized-rodrigues-parameters]] — Gibbs vector + MRP family (deferred concept now broken out below).
- [[sources/lefferts-1982-mekf]] — predecessor paper this extends.
- [[sources/crassidis-2003-ukf-attitude]] — the UKF alternative for cases where second-order MEKF still struggles.
- [[sources/schaub-1998-vscmg]] — Lyapunov control law that consumes the MRP / quaternion estimate.
