---
type: concept
canonical_name: "Pinocchio"
aliases: [pinocchio, Pinocchio C++ library, pinocchio-cpp]
created: 2026-05-04
updated: 2026-05-04
---

# Pinocchio

The C++ rigid-body dynamics library Apsis uses for its multi-body subsystem (architecture §3 build-vs-reuse table; subsystems §4.1-4.3). Open source under **BSD 2-Clause** at github.com/stack-of-tasks/pinocchio. Maintained by the Gepetto team at LAAS-CNRS / Inria.

The introductory paper [[sources/carpentier-2019-pinocchio]] and the analytical-derivatives derivation [[sources/carpentier-2018-rbd-analytical-derivatives]] are the canonical references.

## What it provides

- **Spatial-algebra primitives** ([[concepts/spatial-algebra]]) — SE(3), Motion, Force, Inertia types with the standard operations.
- **Joint types** — Revolute, Prismatic, Spherical, Translation, Planar, **Free-floating (SE(3))**, Composite. Each carries its own configuration/tangent space with Lie-group `exp` / `log` integration & differentiation operators.
- **Model loading** from URDF (RBDL convention), Lua, or the C++/Python API. See [[sources/urdf-xacro-pinocchio-docs]] for the URDF/Xacro authoring pipeline.
- **Forward kinematics** to second order (placement, velocity, acceleration of every joint).
- **Spatial Jacobians** in body-local or world frame.
- **[[concepts/recursive-newton-euler-algorithm|RNEA]]** — inverse dynamics.
- **[[concepts/composite-rigid-body-algorithm|CRBA]]** — joint-space inertia matrix M(q).
- **[[concepts/articulated-body-algorithm|ABA]]** — forward dynamics. The one Apsis calls every integrator step.
- **Constrained forward dynamics, impulse dynamics, inverse-of-mass-matrix, centroidal dynamics**.
- **Analytical derivatives** of RNEA and ABA — first MBD framework to do this natively, derived in [[sources/carpentier-2018-rbd-analytical-derivatives]]. 3-5× the cost of one dynamics call; 6-27× faster than finite differences depending on DOF; 30-60% faster than autodiff+codegen; numerically exact (no rounding noise on partials that should be identically zero). See [[concepts/analytical-rbd-derivatives]] for the chain-rule structure that makes ABA-derivatives a free byproduct of RNEA-derivatives.
- **Automatic differentiation** via templated scalar type (ADOL-C, CasADi, CppAD).
- **Runtime code generation** via CppADCodeGen for performance-critical specialised builds.

## Design choices that affect Apsis's binding code

**Strict model/data separation.** Every algorithm signature is `algo(model, data, args...)`. `Model` is immutable; `Data` is per-call working storage. Allows multiple `Data` per shared `Model` — directly enables Monte Carlo parallelism (REQ-MC-005, REQ-MC-006).

**Static polymorphism via CRTP.** `JointModelBase<Derived>` templated on the child class. No virtual calls. Storage uses `JointModelVariant` to recover runtime polymorphism. Apsis-side wrappers should follow the same pattern; imposing a virtual interface above Pinocchio negates the performance benefit.

**Lie-group-aware integration.** The `integrate` operator handles SO(3), SE(3) and other non-vector tangent spaces correctly — no special-casing needed for quaternion attitude or spherical joints in the configuration update.

## What Pinocchio does NOT do (relevant to Apsis)

These are not failings — Pinocchio is a dynamics library, not a robot simulator — but they're load-bearing for Apsis's design:

- **Does not apply joint friction or damping torques automatically.** Reads URDF `<dynamics damping="..." friction="..."/>` into the Model as metadata only. Apsis's integration loop must compute these from joint velocity and add them to the external-torque vector before calling ABA. REQ-MBD-006 sits on this.
- **Does not enforce joint position limits** during integration. Same pattern: read from URDF, enforce externally (e.g. as soft penalty forces, or by clipping integrator state with appropriate caveat).
- **Does not handle environment collisions automatically.** FCL is bundled for distance/collision queries but the contact solver is the application's responsibility (Apsis doesn't need contact for orbital flight; relevant only if a future scope adds landing or docking with mechanical contact).
- **Does not include actuator dynamics.** Effector valve dynamics, motor electrical model, etc. are external. Apsis's Effector interface (architecture §3 Spacecraft internals) handles this above Pinocchio.

## Performance ballpark

From [[sources/carpentier-2019-pinocchio]] §V.A (Intel Core i7 @ 2.4 GHz):

| Robot | DOF | RNEA | ABA | CRBA |
|---|---|---|---|---|
| KUKA LWR | 7 | ~1 µs | ~1 µs | ~1 µs |
| Atlas / TALOS | ~30 | ~3 µs | ~3 µs | ~3 µs |

A typical Apsis spacecraft (rigid body + a handful of wheels, gimbals, arrays — 10-15 DOF) sits in between. Even at a 10 kHz integrator inner loop, Pinocchio is well under 2% of CPU.

## Dependencies & install

- **Eigen** (hard) — Apsis already commits to Eigen.
- **FCL** (hard, but only collision queries used) — fork at version 0.3.1 per [[sources/carpentier-2019-pinocchio]] footnote 1; check current Pinocchio for actual pinned version.
- **urdfdom** for URDF parsing.
- Optional: ADOL-C, CasADi, CppAD, CppADCodeGen.
- Install: Ubuntu via `robotpkg` APT repository; macOS via Homebrew; build from source for custom configurations.

## Future directions noted in the 2019 paper

§VII: planned support for transmission models (gear, pulley, tendon) and actuator dynamics (electric motor, pneumatic muscle). For Apsis v1 these belong in the Effector interface; if Pinocchio later integrates them natively, Apsis can simplify its Effector implementations to delegate.
