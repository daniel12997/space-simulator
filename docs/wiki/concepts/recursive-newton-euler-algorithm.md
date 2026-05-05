---
type: concept
canonical_name: "Recursive Newton-Euler Algorithm"
aliases: [RNEA, inverse dynamics algorithm]
created: 2026-05-04
updated: 2026-05-04
---

# Recursive Newton-Euler Algorithm (RNEA)

The O(n) recursive **inverse dynamics** algorithm for kinematic trees: given a multi-body configuration (`q`), velocity (`v`), and acceleration (`a`), returns the joint torques `τ` required to produce that motion. Originally derived by Luh, Walker & Paul in 1980 ("On-line computational scheme for mechanical manipulators", J. Dyn. Sys. Meas. Control 102(2):69-76; INDEX-only paywalled).

Where [[concepts/articulated-body-algorithm|ABA]] is `a = f(q, v, τ)`, RNEA is the dual: `τ = f(q, v, a)`. Both are O(n), both single-traversal of the kinematic tree, both built on [[concepts/spatial-algebra|spatial algebra]]. [[concepts/pinocchio-library|Pinocchio]] provides RNEA per [[sources/carpentier-2019-pinocchio]] §II.F.c.

## Apsis use cases

REQ-MBD-003 mandates RNEA; three concrete uses for Apsis:

**1. Controller torque-feedforward.** A controller that knows a desired joint trajectory `(q_d, v_d, a_d)` can call RNEA to compute the feedforward torque `τ_ff = RNEA(q_d, v_d, a_d)` exactly compensating the rigid-body dynamics. The feedback controller then only needs to handle disturbances, not the full nonlinearity. Used by computed-torque controllers and as the basis for inverse-dynamics MPC.

**2. Gravity-only torque vector.** Calling `RNEA(q, v=0, a=0)` returns the static torque required to hold the configuration against gravity — the gravity-compensation term `g(q)`. Useful for setpoint hold controllers and for separating the gravity term from Coriolis/centrifugal in dynamics-equation analyses.

**3. Coriolis/centrifugal vector.** Calling `RNEA(q, v, a=0) - RNEA(q, 0, 0)` returns the velocity-coupled bias `c(q, v) = h(q, v) - g(q)` — the Coriolis + centrifugal terms separated from gravity. Useful for stability proofs and for explicit-form `M(q) a + c(q, v) + g(q) = τ` formulations needed by some controller derivations.

## Algorithm structure

Two passes over the kinematic tree:

1. **Forward pass (root → leaves)** — propagate spatial velocities and accelerations down the tree using kinematic recursion. At each link compute the resultant spatial force from `f = I · a + v ×* I · v` (Newton-Euler, in spatial form).
2. **Backward pass (leaves → root)** — sum spatial forces back up the tree, projecting onto each joint's motion subspace to extract the joint-space torque component.

The forward pass is identical to forward-kinematics-to-second-order; the backward pass is what produces the torques.

## Derivatives

Analytical partial derivatives of RNEA with respect to (`q`, `q̇`, `τ`, `f^ext`) are derived in [[sources/carpentier-2018-rbd-analytical-derivatives]] (Algorithms 2, 3). Six lines added to the forward pass and four to the backward pass propagate motion-set and force-set partial derivatives alongside the dynamics quantities, preserving kinematic-tree sparsity and matching the O(n) complexity of RNEA itself. See [[concepts/analytical-rbd-derivatives]] for the algorithmic structure and the load-bearing chain-rule identity that makes ABA-derivatives a free byproduct.

## What it does NOT do

RNEA does not invert the dynamics in either direction it can't reach:
- **Doesn't compute joint accelerations from torques** — that's ABA.
- **Doesn't compute the joint-space inertia matrix M(q)** — that's [[concepts/composite-rigid-body-algorithm|CRBA]]. (Although a slow and obvious method to get M(q) is to call RNEA n times with `a = e_i` unit vectors and `v = 0`, gravity off; Pinocchio's CRBA is much faster.)
- **Doesn't handle joint friction or damping** — Apsis must compute these externally and either subtract them from `τ` (when using RNEA for inverse-dynamics control) or add them to the external-torque vector (when using ABA for forward simulation). Same caveat as [[concepts/articulated-body-algorithm|ABA]].

## Reading list

- **[[sources/carpentier-2019-pinocchio]]** §II.F.c — brief overview.
- **Luh, Walker & Paul (1980)** — *On-line computational scheme for mechanical manipulators*, J. Dyn. Sys. Meas. Control 102(2):69-76. The original derivation. INDEX-only.
- **Featherstone (2008)** — *Rigid Body Dynamics Algorithms*, Springer Ch. 5. Textbook treatment. INDEX-only.
- **[[sources/carpentier-2018-rbd-analytical-derivatives]]** — analytical derivatives of RNEA, with the algorithm pseudo-code (Algorithms 2, 3) and benchmarks vs finite differences and autodiff.
