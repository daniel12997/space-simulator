---
type: concept
canonical_name: "Quaternion Attitude Representation"
aliases: [quaternion, attitude quaternion, unit quaternion]
created: 2026-05-04
updated: 2026-05-04
---

# Quaternion Attitude Representation

The four-component representation of rigid-body orientation that Apsis uses as the canonical attitude state. A unit quaternion `q̄ = [q ; q₄]` with `|q̄|² = q²₁ + q²₂ + q²₃ + q²₄ = 1`, where the vector part `q = n̂ sin(θ/2)` and scalar part `q₄ = cos(θ/2)` encode rotation by angle `θ` about unit axis `n̂` ([[sources/lefferts-1982-mekf]] Eqs 30-31).

## Why quaternion (and not Euler angles or rotation matrices)

Three considerations established in [[sources/lefferts-1982-mekf]] §1 and §2:

**Stuelpnagel's no-go theorem.** *No three-parameter representation of attitude can be both global and non-singular.* Any three-parameter form (Euler angles, modified Rodrigues parameters, exponential coordinates) has either singularities (Euler → gimbal lock at θ=π/2) or a representation that wraps (Rodrigues → infinity at 180°). Quaternions have four parameters with one constraint (`|q̄|=1`), and that's the smallest globally non-singular description.

**Linear, simple kinematics.** The kinematics ODE `dq̄/dt = ½ Ω(ω) q̄` is bilinear in `q̄` and `ω`, with `Ω(ω)` a 4×4 skew matrix ([[sources/lefferts-1982-mekf]] Eqs 41-42). Closed-form solution exists for constant `ω` over a step (Eq. 47): `q̄(t+Δt) = [cos(|Δθ|/2) I + (sin(|Δθ|/2)/|Δθ|) Ω(Δθ)] q̄(t)`. No transcendental function evaluations on the fast path beyond the cos/sinc per step, no special cases, no singularities.

**No transcendentals on the rotation matrix.** The body-to-inertial rotation matrix `A(q̄)` is a quadratic polynomial in the four quaternion components ([[sources/lefferts-1982-mekf]] Eq. 33): `A(q̄) = (q²₄ - |q|²) I + 2 q qᵀ + 2 q₄ [q×]`. Cheap; no `sin`/`cos`. Composing rotations is quaternion multiplication — also bilinear.

## Composition convention

[[sources/lefferts-1982-mekf]] adopts a quaternion-multiplication convention where **the product order matches the rotation-matrix product order**, *opposite* to Hamilton's classical convention:

```
A(q̄') A(q̄) = A(q̄' ⊗ q̄)              [[sources/lefferts-1982-mekf]] Eq. 35
```

i.e., applying `q̄` first then `q̄'` is `q̄' ⊗ q̄` (not `q̄ ⊗ q̄'` as Hamilton would have it). Apsis adopts this convention because it matches the natural rotation-matrix ordering used in frame transformations and avoids a category of "is this the active or passive convention" bugs. Where Hamilton's convention appears in external references (e.g., much of the robotics literature), conversion is by reversing operand order or by using the conjugate `q̄⁻¹ = [-q ; q₄]`.

## Multiplicative vs additive composition

The single most important distinction for filter design ([[sources/lefferts-1982-mekf]] §§7-11):

- **Multiplicative composition** `q̄(+) = δq̄ ⊗ q̄(-)` preserves unit norm by construction. Used in:
  - Quaternion propagation under angular velocity (always)
  - [[concepts/mekf|MEKF]] update step: error quaternion composed multiplicatively with the estimate
  - Composing successive rotations
- **Additive arithmetic** `q̄(+) = q̄(-) + Δq̄` does *not* preserve unit norm and produces a singular covariance when used as the EKF update. Discussed in [[sources/lefferts-1982-mekf]] §8 specifically to argue against it.

The MEKF's "multiplicative" name refers to this choice — the error quaternion is composed multiplicatively rather than added arithmetically.

## Renormalization

Pure propagation is linear and norm-preserving in exact arithmetic; in float, round-off drifts the norm. Standard renormalization techniques:

- **Naïve**: `q̄ ← q̄ / |q̄|`. Requires `√` and divide; slow.
- **Periodic Newton step** ([[sources/lefferts-1982-mekf]] Eq. B-20): `q̄ ← q̄ · (3 + q̄ᵀq̄) / (1 + 3 q̄ᵀq̄)`. No `√`, no divide branch on the hot path; quartic convergence — a norm error of ε reduces to ε³/32. Apply every few mission seconds.
- **Per-step**: division at every integration step. Wasteful — the per-step drift is ULP-scale; periodic renormalization handles it.

Apsis should adopt the periodic Newton step. Naïve normalization on demand is acceptable when an operation requires the norm to be exactly 1 (e.g., before computing `A(q̄)` for an arcsec-precision pointing operation).

## Going to and from rotation matrices

- **Quaternion → rotation matrix**: closed form `A(q̄) = (q²₄ - |q|²) I + 2 q qᵀ + 2 q₄ [q×]` ([[sources/lefferts-1982-mekf]] Eq. 33).
- **Rotation matrix → quaternion**: many algorithms; the standard "singularity-free" extraction is Sheppard's method (Sheppard 1978, [[sources/lefferts-1982-mekf]] ref [50]) which picks the largest of the four candidate components for numerical stability. Klumpp 1976 ([[sources/lefferts-1982-mekf]] ref [49]) and Spurrier 1978 ([[sources/lefferts-1982-mekf]] ref [51]) are alternatives.

## Going to and from Euler angles

Conversions exist for any of the 12 valid Euler-angle conventions (XYZ, ZYX, ZXZ, etc.). All have gimbal-lock singularities at certain attitudes — that's why Apsis carries quaternions internally and converts to Euler only at user-facing boundaries (telemetry display, scenario-file inputs in human-readable form). **Never run filters or controllers on Euler-angle state**; convert at the boundary, run on quaternions.

## Apsis use

- **Canonical attitude state** for every spacecraft (subsystems §1.3, §4).
- **State for [[concepts/mekf|MEKF]]** (REQ-GNC-003).
- **Pinocchio's `JointModelFreeFlyer` configuration** carries the spacecraft attitude as a quaternion ([[sources/carpentier-2019-pinocchio]] §II.C, [[concepts/floating-base-dynamics]]).
- **Per-spacecraft body, LVLH, RSW, NTW frames** (REQ-TIME-007) are computed from the quaternion + orbital state.

## What the quaternion is NOT

- Not a "vector with a scalar part" in the geometric-algebra sense — the multiplication is non-commutative.
- Not the same as the **error quaternion** δq̄ used in MEKF (which is a small-rotation perturbation, not a full attitude state).
- Not unique: `q̄` and `-q̄` represent the same physical rotation (double cover of SO(3) by S³). Convention: keep `q₄ ≥ 0` to break the ambiguity for human-readable output; internally either sign is fine.
