---
type: concept
canonical_name: "Spatial Algebra"
aliases: [spatial vector algebra, Plücker coordinates, six-dimensional algebra]
created: 2026-05-04
updated: 2026-05-04
---

# Spatial Algebra

The 6-dimensional vector calculus that rigid-body dynamics is naturally expressed in. Concatenates linear and angular parts of velocity, acceleration, and force into single 6-vectors, with operators that respect the underlying Lie-group geometry of SE(3). Featherstone's "Rigid Body Dynamics Algorithms" (Springer 2008, INDEX-only paywalled) is the canonical textbook treatment; [[sources/carpentier-2019-pinocchio]] §II.A grounds the [[concepts/pinocchio-library|Pinocchio]] implementation on this same notation.

## The four primitive types

| Type | What it is | Example |
|---|---|---|
| **SE(3)** | Rigid-body placement (rotation + translation) | Position and orientation of a link in the world frame |
| **Motion** (M) | Spatial velocity or acceleration | (ω, v) — angular + linear, in the same 6-vector |
| **Force** (F) | Spatial force or wrench | (τ, f) — moment + force, in the same 6-vector |
| **Inertia** (I) | 6×6 spatial inertia tensor | mass + COM + moment-of-inertia, in one block |

Motion and Force live in **dual** spaces: a Force acts on a Motion to produce power (`P = F · M`).

## Why it matters for Apsis

Three things make spatial algebra the right framework for Apsis's MBD subsystem:

**1. Single-vector parsimony.** Linear and angular dynamics are coupled through the inertia tensor and through gravity-torque effects on a body whose COM is offset from its joint. Carrying both halves as one 6-vector keeps the bookkeeping clean; `f = I · a` (where `a` is spatial acceleration) generalises Newton's `F = ma` to rigid bodies with non-trivial geometry.

**2. Tree-traversal algorithms become natural.** [[concepts/recursive-newton-euler-algorithm|RNEA]], [[concepts/articulated-body-algorithm|ABA]], and [[concepts/composite-rigid-body-algorithm|CRBA]] are all expressed as forward + backward passes over the kinematic tree, propagating Motion vectors down (kinematics) and Force vectors up (dynamics). The propagation operators are coordinate transformations through SE(3) plus spatial-algebra products.

**3. Lie-group structure preserved.** SE(3) is a Lie group; `exp` and `log` operators on it (used by Pinocchio's `integrate` / `difference` joint operations) keep states on the correct manifold automatically. Critical for Apsis's [[concepts/floating-base-dynamics|floating-base coupling]] — the spacecraft attitude updates as an SE(3) `exp`, not as element-wise quaternion arithmetic that drifts off-norm.

## Frames

Spatial-algebra quantities always carry a **frame label**. A Motion vector expressed in a body-local frame is *not* equal to the same Motion vector expressed in the world frame — coordinate transformation is non-trivial because the angular and linear parts mix under SE(3) action. Pinocchio offers algorithms that produce results in world frame (e.g. `getJointVelocity` with `WORLD` reference frame), local frame, or local-world-aligned frame; choosing the right one is part of using the library correctly.

For Apsis: effector wrenches are typically natural to express in the **link-local** frame (a thruster's thrust axis is fixed in the link). Reading sensor measurements is typically natural in the **body** frame of the sensor's mount link. Frame-explicit naming on Apsis's effector and sensor APIs avoids a category of frame-confusion bugs.

## Reading list

- **Featherstone (2008)** — *Rigid Body Dynamics Algorithms*, Springer. The canonical textbook. INDEX-only paywalled; consult library copy.
- **[[sources/carpentier-2019-pinocchio]]** §II.A — quick overview tied to Pinocchio's implementation.
- **Featherstone (1983)** — *The Calculation of Robot Dynamics Using Articulated-Body Inertias*, IJRR. The original ABA paper. INDEX-only paywalled.
