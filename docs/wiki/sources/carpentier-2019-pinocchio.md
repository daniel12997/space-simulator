---
type: source
title: "The Pinocchio C++ library — A fast and flexible implementation of rigid body dynamics algorithms and their analytical derivatives"
raw_path: docs/raw/papers/carpentier-2019-pinocchio.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Carpentier, Justin; Saurel, Guilhem; Buondonno, Gabriele; Mirabel, Joseph; Lamiraux, Florent; Stasse, Olivier; Mansard, Nicolas]
publication_date: 2019-01
venue: "IEEE/SICE International Symposium on System Integrations (SII), Paris, January 2019"
hal_id: hal-01866228v2
url: https://hal.laas.fr/hal-01866228
---

# Carpentier et al. (2019) — The Pinocchio C++ library

The introductory paper for [[concepts/pinocchio-library|Pinocchio]] — the C++ rigid-body dynamics framework Apsis uses for its multi-body subsystem (architecture §3 Foundation > Spacecraft internals; the build-vs-reuse table). Describes the library's design goals (be fast *and* flexible — the two paradigms that prior frameworks forced you to choose between), the spatial algebra foundation, the model/data separation, the main algorithms (FK, [[concepts/recursive-newton-euler-algorithm|RNEA]], [[concepts/composite-rigid-body-algorithm|CRBA]], [[concepts/articulated-body-algorithm|ABA]]), the analytical-derivatives capability that distinguishes Pinocchio from prior libraries, and benchmarks against RBDL.

## Two paradigms; Pinocchio supports both

Prior MBD libraries split into two camps (§I):

- **Code-generation** (SD/Fast, ROBOTRAN, HuMAnS, Symoro, METAPOD, RobCoGen) — meta-programs emit robot-specific C++ source from a description file. Fast at runtime, but *any* model change requires regeneration, and parameters are hard-coded.
- **Dynamic-loading** (RBDL, OpenHRP, SymBody, RigidBodyDynamics.jl, Drake, Bullet, DART) — one compiled library that loads any robot at runtime via URDF or similar. Versatile, but with runtime dispatch cost.

Pinocchio offers both paradigms within one framework. The default dynamic-loading mode matches RBDL on RNEA performance and outperforms it on ABA/CRBA (Fig. 2). The optional CppADCodeGen-backed code generation (github.com/joaoleal/CppADCodeGen) produces robot-specific C++ at runtime that delivers further 3-8× speedup (Fig. 3).

## Strict model/data separation

§II.B is the load-bearing API design choice for Apsis: every algorithm has the signature `algo(model, data, args...)`.

- **`Model`** holds the immutable description: kinematic tree, link inertias, joint parameters. Built once from URDF (or Lua, or programmatic API) and never modified by an algorithm.
- **`Data`** holds working storage for one algorithm invocation: current `q`, `v`, `a`, intermediate spatial transforms, results.
- Multiple `Data` objects may share a single `Model` for parallel evaluation.

**Apsis implication.** This maps cleanly to Apsis's tiered execution model:
- One `Model` per spacecraft type (URDF + sidecar mass updates).
- One `Data` per active spacecraft instance for the integration step.
- Per-trial `Data` for Monte Carlo (REQ-MC-005, REQ-MC-006) — the worker process clones `Data` from the snapshot, the immutable `Model` is shared.
- No heap allocation in the inner integration loop — all working storage is preallocated in `Data`.

## URDF root = free-floating SE(3) joint

§II.C lists the joint types Pinocchio supports: Revolute (X/Y/Z/custom axis), Prismatic (any axis), Spherical (3-DOF rotation, SO(3) configuration), Translation (3-DOF), Planar (2-DOF), **Free-floating (6-DOF, SE(3) configuration)**, and Composite. Pinocchio's `JointModelFreeFlyer` is the primitive Apsis uses for the spacecraft floating base ([[concepts/floating-base-dynamics]]); the orbital position/velocity and attitude/angular-velocity are the configuration and tangent of this joint.

REQ-MBD-005 mandates revolute / prismatic / fixed / continuous joints. Pinocchio supports all four (continuous = revolute without joint limits) plus spherical (needed for Apsis's CMG gimbal pairs) and free-floating (needed for the spacecraft root). ✓ requirement coverage.

## Lie-group geometry awareness

§II.D — each joint type carries its own configuration space and tangent space, with `exp` (integration) and `log` (differentiation) operators. For the free-floating joint these are the SE(3) exponential and logarithm; for spherical, SO(3); for revolute/prismatic, ordinary R. **For Apsis**, this means joint integration done by Pinocchio (via the `integrate` operator) is automatically correct on the manifold — no special-casing needed for quaternion-state attitude or spherical-joint configuration.

## Three core dynamics algorithms

§II.F:

- **Forward kinematics** (to second order) — given `q`, `v`, `a`, computes the spatial placement / velocity / acceleration of every joint.
- **[[concepts/recursive-newton-euler-algorithm|RNEA]]** (Luh, Walker & Paul 1980) — inverse dynamics: given (`q`, `v`, `a`), returns required joint torques `τ`. Forward + backward pass over the kinematic tree. Useful as a controller torque-feedforward primitive *and* as the building block for gravity-only torque vectors (call with `v=0`, `a=0`).
- **[[concepts/composite-rigid-body-algorithm|CRBA]]** (Walker & Orin 1982) — joint-space inertia matrix `M(q)`. Pinocchio has slight modifications for efficiency.
- **[[concepts/articulated-body-algorithm|ABA]]** (Featherstone 1983) — forward dynamics: given (`q`, `v`, `τ`, external forces), returns joint accelerations `a`. **This is what Apsis calls every integrator step** to advance the multi-body state.

Plus constrained forward dynamics, impulse dynamics, inverse-of-mass-matrix, centroidal dynamics (§II.F.f).

## Analytical derivatives — the differentiator

§II.G: *"To the best of our knowledge, Pinocchio is the first rigid body framework which implements this feature natively."* Analytical derivatives of RNEA and ABA with respect to (`q`, `v`, `τ`, external forces) are critical for whole-body trajectory optimization, MPC, and LQR around a reference trajectory. Fig. 4 reports analytical derivatives 1-2 orders of magnitude faster than finite-difference equivalents on KUKA-LWR, HyQ, Atlas, and TALOS. The derivation is in [[sources/carpentier-2018-rbd-analytical-derivatives]] (next planned ingest).

For Apsis, this directly enables REQ-GNC-008 (MPC attitude/orbit controller, S priority) at compute cost competitive with simpler EKF-based control approaches.

## Static polymorphism via CRTP

§III.B — same C++ idiom Eigen uses. `JointModelBase<Derived>` is templated on its child class for compile-time dispatch; no virtual call overhead. Trade-off: `vector<JointModelBase>` doesn't work because different derived classes don't share a runtime base. Recovered via `JointModelVariant` (boost::variant pattern) at the model storage layer.

**Apsis implication**: any Apsis-side wrapper around Pinocchio joints (e.g. effector classes that target a specific joint type) should inherit this CRTP/variant pattern rather than impose virtual dispatch. Fighting Pinocchio's static polymorphism with a virtual interface above it would erase the performance benefit.

## Performance

§V.A and Fig. 1: dynamics evaluation in ~1 µs for a 7-DOF manipulator (KUKA LWR), ~3 µs for 30+ DOF humanoids (Atlas, TALOS). For a typical Apsis spacecraft (rigid body + a handful of reaction wheels, gimbals, solar arrays — maybe 10-15 DOF), expect ~1-2 µs per ABA call. At a 100 Hz GNC tick that's negligible (0.01-0.02% CPU); even at a 10 kHz integrator inner loop it's 1-2% — Pinocchio is not a constraint on Apsis's design.

## License and dependencies

- **License**: BSD 2-Clause. Compatible with everything Apsis touches.
- **Repo**: github.com/stack-of-tasks/pinocchio
- **Tutorials**: github.com/stack-of-tasks/pinocchio-tutorials (mostly Python)
- **Hard dependencies**: Eigen (Apsis already commits to Eigen — architecture §3 Foundation > Math). FCL for collision/distance.
- **Optional**: ADOL-C / CasADi / CppAD for autodiff; CppADCodeGen for runtime code generation.
- **Install**: robotpkg APT repository on Ubuntu (`apt-get install robotpkg-py27-pinocchio`); Homebrew on macOS; or build from source.

## Apsis relevance

- **REQ-SC-001**: URDF loading. Pinocchio's URDF loader follows the RBDL convention. ✓
- **REQ-MBD-001**: floating-base MBD. `JointModelFreeFlyer` as the URDF root. ✓
- **REQ-MBD-002**: forward dynamics. ABA. ✓
- **REQ-MBD-003**: inverse dynamics. RNEA. ✓
- **REQ-MBD-004**: angular momentum coupling. Inherent to the spatial-algebra-based formulation. ✓
- **REQ-MBD-005**: revolute / prismatic / fixed / continuous joints. All supported, plus spherical and free-floating. ✓
- **REQ-MBD-006**: joint limits, friction, damping. Pinocchio reads these from URDF but **does not apply friction/damping torques automatically** — see flag below.
- **REQ-MBD-007**: Jacobian and mass matrix queries. Spatial Jacobian + CRBA. ✓
- **subsystems §4.1, 4.2, 4.3**: URDF model, floating-base, aggregate dynamics. Pinocchio is the engine.
- **architecture §3 Spacecraft internals**: Pinocchio model + data + effectors. ✓ confirms the build-vs-reuse choice.
- **REQ-MC-005, REQ-MC-006**: snapshot/restore + parallel trials. Pinocchio's model/data separation supports this directly — clone Data per trial; share Model.

## Surfaced for human review (no silent spec edits)

**Pinocchio does not apply joint friction or damping torques automatically.** REQ-MBD-006 mandates "joint limits, friction, and damping as configured in URDF." Pinocchio's URDF loader reads `<dynamics damping="..." friction="..."/>` into the `Model` but its ABA implementation does not include these in the dynamics — they're metadata. **Apsis's integration loop must compute friction/damping torques from joint velocity and the URDF-declared parameters, then add them to the external-torque vector before calling ABA.** Neither architecture §3 nor subsystems §4.3 currently calls this out — worth one sentence in subsystems §4.3 step 3 ("Apply effector forces") to remind the implementer.
