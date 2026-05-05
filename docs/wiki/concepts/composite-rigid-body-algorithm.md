---
type: concept
canonical_name: Composite-Rigid-Body Algorithm
aliases: [CRBA]
---

# Composite-Rigid-Body Algorithm (CRBA)

The third member of the Featherstone trio (alongside [[concepts/articulated-body-algorithm|ABA]] for forward dynamics and [[concepts/recursive-newton-euler-algorithm|RNEA]] for inverse dynamics). CRBA computes the **joint-space mass matrix** `M(q)` of a multi-body system in **O(nd)** time, where n is the number of joints and d is the kinematic-tree depth — typically O(n²) in practice for branching trees.

## What CRBA computes

For an articulated rigid-body system with configuration `q`, CRBA returns the symmetric positive-definite mass matrix `M(q)` such that the equations of motion in joint coordinates are:

```
M(q) q̈ + C(q, q̇) q̇ + g(q) = τ
```

`M(q)` appears in:

- **Forward dynamics** the long way: `q̈ = M⁻¹ (τ - C q̇ - g)`. (ABA computes `q̈` directly without materializing `M⁻¹`; CRBA + Cholesky is competitive for small n and lets you reuse `M` across multiple right-hand sides — useful for differential-dynamic-programming MPC, Lagrange-multiplier constraint solvers, and operational-space inertia computations.)
- **Operational-space inertia**: `Λ(q) = (J M⁻¹ Jᵀ)⁻¹` for end-effector control — needs `M⁻¹` explicitly.
- **Constraint solvers**: contact / closure constraints with Lagrange multipliers `λ = (J_C M⁻¹ J_Cᵀ)⁻¹ (...)` — CRBA + Cholesky inverse is a common building block (see [[sources/mistry-2010-floating-base-inverse-dynamics|Mistry 2010]]).
- **Spectral analysis** of system modes — eigenvalues of `M(q)⁻¹ K(q)` (linearized stiffness) give natural frequencies.

## Featherstone trio summary

| Algorithm | Computes | Cost | Apsis use |
|---|---|---|---|
| **ABA** | `q̈` from `(q, q̇, τ)` | O(n) | Forward dynamics in numerical integrator |
| **RNEA** | `τ` from `(q, q̇, q̈)` | O(n) | Inverse dynamics for feedforward control, gravity comp |
| **CRBA** | `M(q)` | O(n²) practical | Mass matrix when needed explicitly |

[[concepts/pinocchio-library|Pinocchio]] implements all three with Apsis-suitable performance and analytical derivatives ([[sources/carpentier-2018-rbd-analytical-derivatives|Carpentier & Mansard 2018]]).

## When Apsis uses CRBA vs ABA

- **Default forward dynamics**: ABA — single shot, no `M⁻¹` materialization.
- **Need `M⁻¹` for downstream use** (operational-space, constraints, MPC linearization): CRBA + Cholesky factor + back-substitute.
- **Mode analysis / linearization**: CRBA — `M(q)` and `K(q)` together feed the eigenvalue problem.

## See also

- [[concepts/articulated-body-algorithm]] — forward dynamics, O(n).
- [[concepts/recursive-newton-euler-algorithm]] — inverse dynamics, O(n).
- [[concepts/spatial-algebra]] — the underlying 6-vector math.
- [[sources/carpentier-2019-pinocchio]] — Pinocchio's CRBA implementation.
- [[sources/mistry-2010-floating-base-inverse-dynamics]] — example of CRBA + constraint Jacobian + pseudo-inverse for floating-base under-actuation.
