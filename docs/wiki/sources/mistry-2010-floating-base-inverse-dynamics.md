---
type: source
title: "Inverse Dynamics Control of Floating Base Systems Using Orthogonal Decomposition"
raw_path: docs/raw/papers/mistry-2010-floating-base-inverse-dynamics.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Mistry, Michael; Buchli, Jonas; Schaal, Stefan]
publication_date: 2010-05
venue: "2010 IEEE International Conference on Robotics and Automation (ICRA), Anchorage AK"
doi: 10.1109/ROBOT.2010.5509646
---

# Mistry, Buchli & Schaal (2010) — Floating-base inverse dynamics via orthogonal decomposition

The technique that solves the **ill-posed inverse-dynamics problem** for under-actuated [[concepts/floating-base-dynamics|floating-base systems]] (humanoids, legged robots, free-floating manipulators on spacecraft) by **projecting the rigid-body equations into the constraint null-space via QR decomposition**. Eliminates the need to estimate or measure contact forces, and avoids the discontinuities that plagued earlier passivity-based approaches. Directly applicable to Apsis whenever a spacecraft is in contact with another body (capture, berthing, surface contact for landers, on-orbit servicing).

## The problem (§II-III)

A floating-base system has configuration `q = [q_r; x_b]` with `q_r ∈ R^n` joint coordinates and `x_b ∈ SE(3)` base pose. Equations of motion under contact:

```
M(q) q̈ + h(q, q̇) = Sᵀ τ + J_Cᵀ λ          (Eq. 2)
```

where `S = [I_n  0_{n×6}]` is the **actuated joint selector** (the 6 base DOFs are unactuated), `τ` is the joint-torque vector, `J_C ∈ R^{k×(n+6)}` is the constraint Jacobian for `k` linearly-independent contact constraints, and `λ` are the contact-force multipliers.

Inverse dynamics — given desired `q̈_d`, solve for `τ` — is **ill-posed**: `λ` and `τ` are co-dependent because the contact forces depend on the actuation torques applied. Direct elimination via Eq. 6 (`λ = (J_C M⁻¹ J_Cᵀ)⁻¹ (...)`) requires accurate inertia matrix `M`, can be ill-conditioned, and the resulting projected equation (Eq. 7) is rank-deficient.

## The orthogonal-decomposition solution (§III.B)

QR-decompose `J_Cᵀ`:

```
J_Cᵀ = Q [R; 0]                              (Eq. 9)
```

with `Q` orthogonal and `R` upper-triangular full-rank. Pre-multiply Eq. 2 by `Qᵀ`, split into top-`k` and bottom-(n+6-k) blocks:

```
S_c Qᵀ (M q̈ + h) = S_c QᵀSᵀτ + Rλ            (Eq. 10)  [top: includes λ]
S_u Qᵀ (M q̈ + h) = S_u QᵀSᵀτ                 (Eq. 11)  [bottom: λ-free]
```

Eq. 11 — the projection into the constraint **null-space** — is independent of contact forces. Solve via pseudo-inverse:

```
τ = (S_u Qᵀ Sᵀ)⁺ S_u Qᵀ [M q̈_d + h]          (Eq. 15)
```

Properties (§III.B.3-5):

- **Analytically correct**: applying `τ` from Eq. 15 produces `q̈ = q̈_d` exactly (provided contact constraints hold).
- **No contact-force measurement needed**.
- **Inertia matrix used only once, non-inverted** (vs Eq. 6 which inverts a matrix involving `M⁻¹`) — much less sensitive to inertia-modeling error.
- **Time derivative `J̇_C` not required**.
- **Contact forces recoverable** post-hoc via Eq. 17 if needed for monitoring.

## Apsis relevance

Three Apsis use cases involve floating-base + contact:

1. **Capture / berthing** (subsystems §4 + §6): a free-flying spacecraft contacting a target via robotic arm or capture mechanism. The combined system is a floating-base under-actuated multi-body during the contact phase.
2. **Lander surface contact**: a lunar/Mars lander with deployed legs makes ground contact — same mathematical structure as a humanoid foot.
3. **On-orbit servicing / robotic-arm operations**: serviceable target grasped by service vehicle's arm — multi-body with point or surface contact at the end-effector.

For all three, this paper's method is the **canonical analytically-correct inverse-dynamics controller**. Its unmodified application to spacecraft is a topic in modern aerospace robotics literature.

- **REQ-MBD-005** (contact dynamics for capture / berthing / landing) — this paper is the inverse-dynamics control reference for those scenarios.
- **REQ-GNC-009** (model-based control with under-actuation) — the orthogonal-decomposition projection generalizes to any under-actuated system, not just contact problems.
- **Subsystems §4 (MBD) interaction with §5 (GNC)**: the controller in §5 needs a way to query the §4 dynamics module for analytically-correct torques given desired accelerations and a contact set. This paper specifies that interface mathematically.

## Implementation in Apsis

[[concepts/pinocchio-library|Pinocchio]] provides `M`, `h`, and `J_C` directly. The QR decomposition is a one-liner in Eigen. The pseudo-inverse can be computed via SVD or weighted (Eq. 16) to redistribute torques across joints (e.g., to limit any single actuator). Total cost is dominated by the Jacobian QR — small for typical 7-15 DOF spacecraft + arm systems.

## Cross-references

- [[concepts/floating-base-dynamics]] — the underlying spacecraft-mechanics framing.
- [[concepts/articulated-body-algorithm]] / [[concepts/recursive-newton-euler-algorithm]] — produce `M` and `h` for this paper's formulation.
- [[sources/carpentier-2019-pinocchio]] — implementation library.
- [[sources/likins-1970-flexible-space-vehicles]] — the classical-spacecraft analogue (without the modern numerical apparatus).
