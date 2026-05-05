---
type: source
title: "Feedback Control Law for Variable Speed Control Moment Gyros"
raw_path: docs/raw/papers/schaub-1998-vscmg.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Schaub, Hanspeter; Vadali, Srinivas R.; Junkins, John L.]
publication_date: 1998-09
venue: "Journal of the Astronautical Sciences 46(3):307-328"
---

# Schaub, Vadali & Junkins (1998) — VSCMG feedback control law

The seminal paper on **Variable-Speed Control Moment Gyros (VSCMGs)** as a unified attitude actuator that subsumes both reaction wheels (RW) and single-gimbal control moment gyros (SGCMG). A VSCMG is a single-gimbal CMG whose flywheel speed is also commanded — adding one extra DOF per device. With the right steering law, a VSCMG cluster acts like a CMG cluster away from gimbal-angle singularities (high torque amplification) and **transitions smoothly to RW-like behavior near singularities** (lower torque, but exact torque tracking). The paper presents the equations of motion, two steering laws (gimbal-velocity-based and gimbal-acceleration-based), Lyapunov-based feedback control, and null-motion reconfiguration.

## Why VSCMGs

- **Reaction wheels**: simple, no singularities, but small torque per unit mass and limited by saturation.
- **Single-gimbal CMGs**: large torque amplification (small gimbal torque → large output torque), but suffer **gimbal-angle singularities** where the cluster cannot produce the commanded torque vector. Even with singularity-avoidance steering, the actual delivered torque deviates from commanded near singularities, causing path errors.
- **VSCMGs**: combine both. The extra DOF from variable wheel speed lets the steering law deliver exactly the commanded torque even at classical-CMG singularities — the "missing" torque component is supplied by wheel-speed change instead of gimbal motion.

## Key technical contributions

### Equations of motion

Generalized rigid-body + N-VSCMG dynamics with each VSCMG contributing 2 actuator DOFs (gimbal angle `γ_i` and wheel spin rate `Ω_i`). The framework specializes to:
- Pure RW system (gimbal frozen): `γ̇_i = 0`.
- Pure SGCMG system (constant wheel speed): `Ω̇_i = 0`.

So the same code path supports any of the three actuator topologies with the same equations.

### Steering law

For a redundant VSCMG cluster (N > 3), the mapping from VSCMG inputs `[γ̇; Ω̇]` to commanded body torque `L` is over-determined → infinite solutions. Schaub uses a **weighted minimum-norm pseudo-inverse**:

```
[γ̇; Ω̇] = W · Aᵀ (A W Aᵀ)⁻¹ L
```

with `W = diag(W_γ, W_Ω)` weights that bias toward CMG-mode (small `W_Ω`) away from singularities and toward RW-mode (large `W_Ω`) near them. Singularity proximity is detected via the manipulability measure `√det(A_γ A_γᵀ)`.

### Lyapunov-stable feedback control

A globally asymptotically stabilizing nonlinear feedback law (built on MRPs or quaternions) generates `L`; the steering law distributes it across the cluster.

### Null-motion reconfiguration

VSCMG clusters with N > 3 have a **null-space** of input combinations producing zero net torque. Use this to drive gimbal angles toward a preferred (singularity-distant) set without disturbing attitude. Variable wheel speed provides additional null-motion freedom over fixed-spin SGCMG clusters.

## Apsis relevance

- **REQ-SC-007** (RW + CMG actuator suite): Apsis must support both. VSCMGs unify them — implementing this paper's framework gets RW, SGCMG, and VSCMG support with one code path.
- **REQ-GNC-008** (singularity-aware steering law for CMG clusters): the weighted minimum-norm pseudo-inverse here is the canonical algorithm. Apsis subsystems §5 should specify it explicitly.
- **REQ-GNC-002** (Lyapunov-stable nonlinear attitude control): Schaub's MRP-based feedback, presented here in the VSCMG context, is the classical reference. Many subsequent control-law variants build on this paper.
- **Future MEKF + actuator-set integration** ([[concepts/mekf]] + steering): the Lyapunov controller assumes perfect attitude knowledge; combined with MEKF estimates, the closed-loop performance depends on covariance handover from estimator to controller — a subsystems §5 integration topic.

## Items for human review (no silent spec edits)

- Subsystems §5.4 actuator suite — does it currently distinguish SGCMG, DGCMG (dual-gimbal), and VSCMG? VSCMG is a strict superset of the first two; if Apsis supports the most general case (this paper's framework), the others fall out as configuration choices.
- The Lyapunov controller in this paper assumes rigid spacecraft. Coupling with [[sources/likins-1970-flexible-space-vehicles|flexible appendages]] requires either pre-filter design (notch filtering of flex modes from torque commands) or modal-cost-augmented control.

## Cross-references

- [[concepts/mekf]] — attitude estimator that feeds the controller in this paper.
- [[concepts/quaternion-attitude-representation]] — the Lyapunov controller's state representation (or MRPs as alternative).
- [[sources/likins-1970-flexible-space-vehicles]] — flex-mode interaction with the controller.
- [[sources/markley-2003-attitude-error-representations]] — error representations Schaub builds on (next ingest).
