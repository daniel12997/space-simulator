---
type: source
title: "Model Predictive Control Approach for Guidance of Spacecraft Rendezvous and Proximity Maneuvering"
raw_path: docs/raw/papers/dicairano-2012-mpc-rendezvous.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Di Cairano, Stefano; Park, Hyungjin; Kolmanovsky, Ilya V.]
publication_date: 2012
venue: "International Journal of Robust and Nonlinear Control (Mitsubishi Electric Research Labs TR2012-099)"
doi: 10.1002/rnc.2885
---

# Di Cairano, Park & Kolmanovsky (2012) — MPC for spacecraft rendezvous and proximity ops

A canonical reference for **Model Predictive Control (MPC)** applied to spacecraft **Rendezvous and Proximity Operations (RPO)**. Demonstrates that the typical RPO constraint set — thrust magnitude limits, line-of-sight (LOS) cone constraint near the target docking port, approach-velocity matching at contact — is **handled cleanly by MPC** as state and input constraints in a constrained-optimization formulation. Treats both non-rotating and rotating/tumbling target platforms; for the non-rotating case provides an **explicit off-line MPC solution** in the form of a piecewise-affine control law (no onboard optimizer required).

## Why RPO needs MPC

Traditional RPO guidance uses **open-loop maneuver planning** (e.g., Clohessy-Wiltshire glideslope) plus **ad hoc error corrections**. This breaks down when:

- The target is tumbling (docking-port pose is time-varying and must be predicted).
- LOS-cone constraints must hold continuously, not just at waypoints.
- Thrust authority is finite, asymmetric, or coupled to attitude (e.g., body-fixed thrusters).
- Debris in the rendezvous corridor must be avoided dynamically.

MPC handles all four naturally because constraints, prediction, and feedback are all in one formulation.

## Problem setup (orbital plane)

Plant: Hill / Clohessy-Wiltshire linearized relative dynamics in target-orbit-relative LVLH frame.

Constraints:
- **Thrust magnitude** `|u| ≤ u_max` per axis (or norm).
- **LOS cone** at the docking port: relative position must lie in a cone emanating from the docking port axis with half-angle θ_LOS.
- **Terminal velocity match** at contact: `|v_rel - v_dock| ≤ ε`.
- **Debris avoidance** (later sections): obstacle-shaped exclusion zones along the trajectory.

Cost: weighted combination of **fuel** (Σ |u|²), **trajectory time**, and **state-tracking error**. Receding-horizon QP solved at each time step.

## Two operational cases

### Non-rotating target (§III)

Docking-port pose is fixed in LVLH frame → MPC problem is time-invariant → **explicit MPC** solution exists. The optimal control law is a **piecewise-affine function of state**, computed offline and table-looked-up onboard. No QP solver needed → suitable for low-CPU flight processors.

### Rotating / tumbling target (§IV)

Docking-port pose is time-varying. MPC must **predict the platform rotation** over the prediction horizon to align the LOS cone correctly. Without rotation prediction, maneuvers are infeasible or wildly fuel-inefficient. Authors show that even a coarse rotation model dramatically improves performance vs treating the docking port as instantaneously-fixed.

### Debris avoidance (§V)

Adds debris-position predictions as time-varying obstacle constraints. MPC re-plans around them automatically.

## Apsis relevance

- **REQ-GNC-006** (constrained guidance for proximity ops) — this paper is the canonical MPC reference. Apsis subsystems §6 should specify MPC as the proximity-ops guidance algorithm with this paper's constraint set as the baseline.
- **Subsystems §6** (Conjunction / CAM): debris-avoidance MPC here generalizes the impulsive Bombardelli formulation ([[sources/bombardelli-2015-collision-avoidance]]) to multi-burn finite-thrust scenarios.
- **REQ-INT-009** (continuous-thrust + impulsive maneuvers): MPC handles both with the same machinery — impulses fall out as one-step input pulses; finite-thrust burns are multi-step input sequences.
- **REQ-PERF-002** (real-time control loop): the explicit-MPC formulation for non-rotating targets fits embedded targets; the on-line QP for tumbling targets needs a real-time QP solver (qpOASES, OSQP, HPIPM) integrated into Apsis's GNC stack.
- **MBD interaction** (subsystems §4): for tumbling-target cases, the rotation model the MPC uses is supplied by the Apsis MBD module — closes the loop with the rigid-body dynamics subsystem.

## Items for human review (no silent spec edits)

- Subsystems §6 should distinguish between **impulsive CAM** (Bombardelli 2015 analytic) and **constrained-trajectory proximity-ops MPC** (this paper). Different algorithms for different mission phases — "CAM" is a one-shot Δv against a debris pass; "RPO MPC" is a multi-step horizon-controlled trajectory near a cooperative target.
- The non-rotating explicit-MPC variant requires **Multi-Parametric Programming** tools (e.g., MPT3 toolbox) to compute offline. If Apsis intends to support both variants, this is a build-time tooling dependency to surface in subsystems §1.

## Cross-references

- [[sources/bombardelli-2015-collision-avoidance]] — single-impulse alternative for collision avoidance specifically.
- [[sources/carpentier-2018-rbd-analytical-derivatives]] — MPC needs analytical derivatives for fast convergence; Pinocchio supplies them for the multi-body case.
- [[concepts/floating-base-dynamics]] — relevant when the target is treated as a floating-base system.
