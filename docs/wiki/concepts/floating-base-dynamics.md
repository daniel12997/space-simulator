---
type: concept
canonical_name: "Floating-Base Dynamics"
aliases: [floating base, free-flyer joint, free-floating joint, floating-base coupling]
created: 2026-05-04
updated: 2026-05-04
---

# Floating-Base Dynamics

The technique of modelling a vehicle whose root link is **unconstrained in 3D space** by adding a virtual 6-DOF "free-flyer" joint between the world frame and the vehicle's root link. The joint's configuration is the rigid-body placement (position + attitude) of the vehicle; its tangent is the spatial velocity (linear + angular). All internal joints attach to this floating root in the usual kinematic-tree fashion.

For Apsis, **the floating base is the spacecraft's orbital state plus its attitude.** Architecture §3 Foundation > Spacecraft internals: *"Floating-base coupling — the URDF root joint is the orbital state from the ECS."* Subsystems §4.2: *"Pinocchio's `Model` is built with a 6-DOF free-flyer joint as the root. The orbital propagator updates the floating-base pose; Pinocchio handles internal joint dynamics; aggregate forces and torques are applied to the appropriate links."*

## SE(3) configuration, R⁶ tangent

The free-flyer joint's configuration space is **SE(3)** — the manifold of rigid-body placements, parameterised in [[concepts/pinocchio-library|Pinocchio]] as a position vector plus a unit quaternion (7 numbers; 6 DOF after the unit-quaternion constraint). The tangent space is **R⁶** — spatial velocity (3 angular + 3 linear, [[concepts/spatial-algebra|spatial-algebra]] convention).

Integration is on the manifold: `q_{n+1} = q_n ⊕ Δt · v_n`, where `⊕` is the SE(3) `exp` operator. Pinocchio's `integrate` operation applies this correctly — quaternion stays unit-norm by construction, no separate normalization step is needed.

## Coupling pattern in Apsis

The integration loop pattern (one derivative evaluation per call):

1. **ECS → Pinocchio.** Read the spacecraft's `OrbitalState { r, v, frame }` and attitude `(quaternion, ω)` from the ECS. Pack into the floating-base's `q` and `v` slots in Pinocchio's state.
2. **External forces and torques.** Compute environment forces (gravity, drag, SRP, third-body, ERP) and apply them as external wrenches at the COM link or per-panel. Compute effector forces (thrusters, reaction-wheel reaction torques, magnetorquers, gimbal drives) and apply them at their mount links.
3. **Joint friction/damping (Apsis-side).** Pinocchio doesn't apply these — see [[concepts/pinocchio-library]]. Compute from joint velocity and URDF parameters; add to the joint-torque vector.
4. **Forward dynamics.** Call [[concepts/articulated-body-algorithm|ABA]] with the assembled torques and external wrenches. Pinocchio returns the joint accelerations including the floating-base spatial acceleration.
5. **Pinocchio → integrator.** Extract the floating-base linear acceleration → orbital propagator. Extract the floating-base angular acceleration → attitude propagator. Extract internal joint accelerations → integrate alongside.

Subsystems §4.3 enumerates this exact pattern.

## Why it matters

**Angular momentum coupling is automatic.** Because the floating-base joint and the internal joints share the same `M(q)` mass matrix in the joint-space dynamics equation, internal joint motion (a reaction wheel spinning up, a gimbal slewing, a solar array deploying) automatically produces the correct reactive torque on the spacecraft attitude — no separate "back-EMF" or momentum-coupling code path needed. REQ-MBD-004 mandates this; the floating-base formulation delivers it for free.

**Conservation laws fall out.** With no external torques, total angular momentum (orbital + attitude + internal joint momentum + reaction-wheel momentum) is exactly conserved by ABA up to integrator precision. Useful as the primary validation invariant for the MBD subsystem (REQ-INT-012).

**One model, two propagators.** The orbital propagator (Encke / Keplerian / Cowell — REQ-INT-001..-005) handles the floating-base translational state at the integrator's chosen step size and order. The same floating-base joint participates in Pinocchio's dynamics for attitude and internal joint state. The two propagators see the same state through different lenses.

## What this is NOT

- **Not a fixed-base manipulator with a moving "world".** Manipulator robots typically have a fixed base; floating-base dynamics adds 6 DOF, fundamentally changing the structure of M(q). The "spacecraft = floating manipulator" framing is correct only with the explicit free-flyer joint.
- **Not a workaround for "make the world coordinate the joint".** Some early sim codes attached the world frame to the spacecraft body and expressed everything relative — that's a non-inertial reference frame and produces fictitious-force bookkeeping. Floating-base dynamics keeps the world inertial and lets the spacecraft float in it.

## References

- **[[sources/carpentier-2019-pinocchio]]** — Pinocchio's free-flyer joint and Lie-group `integrate` operator. §II.C, §II.D.
- **Mistry, Buchli & Schaal (2010)** — *Inverse Dynamics Control of Floating Base Systems Using Orthogonal Decomposition*, ICRA 2010 (`docs/raw/papers/mistry-2010-floating-base-inverse-dynamics.pdf`, ingested separately) — controller-design treatment of the floating-base inverse-dynamics problem.
- **Featherstone (2008)** — *Rigid Body Dynamics Algorithms*, Ch. 9. Textbook treatment of the floating-base extension. INDEX-only.
