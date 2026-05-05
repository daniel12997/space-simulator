---
type: concept
canonical_name: "Articulated Body Algorithm"
aliases: [ABA, forward dynamics algorithm]
created: 2026-05-04
updated: 2026-05-04
---

# Articulated Body Algorithm (ABA)

The O(n) recursive **forward dynamics** algorithm for kinematic trees: given a multi-body configuration (`q`), velocity (`v`), applied joint torques (`τ`), and external wrenches (`f_ext`), returns the joint accelerations (`a`). Originally derived by Featherstone in his 1983 IJRR paper "The Calculation of Robot Dynamics Using Articulated-Body Inertias" (INDEX-only paywalled); reformulated and extended by many subsequent authors.

In Apsis, **ABA is what gets called every integrator step** to advance the multi-body state (REQ-MBD-002, subsystems §4.3 step 4 "Forward dynamics"). [[concepts/pinocchio-library|Pinocchio]] provides the implementation.

## What "forward dynamics" means

The dynamics equation for a kinematic tree:

```
M(q) · a + h(q, v) = τ + Σ J_iᵀ · f_ext,i
```

where `M(q)` is the [[concepts/composite-rigid-body-algorithm|joint-space inertia matrix]], `h(q, v)` collects Coriolis, centrifugal, and gravity terms, `J_i` are the spatial Jacobians at the wrench application points, and `τ` are the joint torques. Forward dynamics inverts this for `a`:

```
a = M(q)⁻¹ · (τ + Σ J_iᵀ · f_ext,i - h(q, v))
```

ABA computes `a` directly without ever explicitly forming or inverting `M(q)`. That's the algorithmic win — the explicit-form inversion is O(n³); ABA is O(n).

## Algorithm structure

A single tree traversal produces the result. Pinocchio's pseudo-code follows Featherstone's three-pass formulation:

1. **Forward pass (root → leaves)**: propagate spatial transforms, velocities, and Coriolis/bias terms down the tree.
2. **Backward pass (leaves → root)**: assemble the **articulated-body inertias** `I^A_i` and bias forces `p^A_i` at each joint, recursing parent-ward.
3. **Forward pass (root → leaves)**: solve for joint accelerations using the assembled articulated quantities.

The articulated-body inertia `I^A_i` is the spatial inertia of the entire subtree rooted at link `i`, *including* the constraint structure introduced by the joints further down. That's the namesake quantity.

## What Apsis cares about

Each integration step (whether Dormand-Prince RK87 or [[concepts/gauss-jackson-integration|Gauss-Jackson 8]]) calls ABA once per derivative evaluation:

1. Read current `q`, `v` from integrator state.
2. Evaluate environment forces (gravity, drag, SRP, third-body) at link COMs.
3. Evaluate effector forces (thrusters, RW reaction torques, magnetorquers) at their mount links.
4. **Compute friction and damping torques from joint velocity** (Pinocchio doesn't do this — see [[concepts/pinocchio-library]]).
5. Sum into `τ` and `f_ext` arrays.
6. Call `pinocchio::aba(model, data, q, v, τ, f_ext)` → fills `data.ddq` with `a`.
7. Return `(v, a)` to the integrator.

Performance for Apsis-scale spacecraft (10-15 DOF): ~1-2 µs per call — not a bottleneck even at 10 kHz.

## Variants

- **Constrained ABA** — handles closed-loop kinematic constraints via Lagrange multipliers; Pinocchio implements via a constrained-dynamics extension. Apsis spacecraft are open-tree by construction (URDF root + tree of links); no closed loops, so plain ABA suffices.
- **Impulse dynamics** — a discrete-event variant for instantaneous velocity changes (collisions, latch-engagement, instantaneous Δv). Apsis impulsive maneuvers (architecture §3 Dynamics core > Thrust) typically modify the floating-base velocity directly without invoking impulse-ABA.

## Derivatives

For controllers that need a dynamics gradient (LQR linearization, MPC, gradient-based trajectory optimization), [[concepts/analytical-rbd-derivatives]] provides ∂a/∂(q, v, τ, f_ext) at 3-5× the cost of one ABA call. The crucial structural identity ([[sources/carpentier-2018-rbd-analytical-derivatives]] Eq. 24): `∂FD/∂u = -M(q)⁻¹ · ∂ID/∂u`. Apsis derives ABA-derivatives this way rather than independently — half the implementation surface, same numerics.

## Validation

The conservation invariant tied to ABA is **angular momentum conservation under no external torques** (REQ-MBD-004, REQ-INT-012). With gravity zero, no external wrenches, no joint torques, and no friction — `a_ω,floating-base + Σ J_i,ang · v_internal = const`. A long-arc no-torque test should hold this to integrator precision. Useful regression check.

## Reading list

- **[[sources/carpentier-2019-pinocchio]]** §II.F.e — brief overview as implemented in Pinocchio.
- **Featherstone (1983)** — *The Calculation of Robot Dynamics Using Articulated-Body Inertias*, IJRR 2(1):13-30. The original derivation. INDEX-only.
- **Featherstone (2008)** — *Rigid Body Dynamics Algorithms*, Springer Ch. 7. Extended treatment in textbook form. INDEX-only.
- **[[sources/carpentier-2018-rbd-analytical-derivatives]]** — analytical derivatives of ABA via the chain-rule reduction to RNEA-derivatives.
