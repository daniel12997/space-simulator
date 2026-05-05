---
type: source
title: "Dynamics and Control of Flexible Space Vehicles"
raw_path: docs/raw/papers/likins-1970-flexible-space-vehicles.pdf
source_type: technical-report
reliability: authoritative
ingested: 2026-05-04
authors: [Likins, Peter W.]
publication_date: 1970-01
venue: "NASA / JPL Technical Report 32-1329, Revision 1"
---

# Likins (1970) — Dynamics and Control of Flexible Space Vehicles (JPL TR 32-1329 Rev 1)

The foundational synthesis of **flexible-spacecraft dynamics-and-control modeling**, developed at JPL and used as a structural reference across the spacecraft control community for the next four decades. Surveys three major method families and develops in detail the **hybrid-coordinate method** that combines 6-DOF rigid-body modes with modal coordinates of flexible appendages — the canonical formulation for any spacecraft with antennas, solar arrays, or booms whose first elastic modes overlap the attitude-control bandwidth.

## Three method families (Abstract)

1. **Discrete-coordinate methods** (§II) — what we now call multi-body dynamics:
   - **Augmented-body** (§II.A): Hooker-Margulies-style topological tree of point-connected rigid bodies with augmented inertias to handle constraint forces between hinged subbodies.
   - **Nested-body** (§II.B): recursive parent-child treatment.
   - **Generalized-force methods** (§II.C): Lagrangian / Kane-style.
2. **Hybrid-coordinate method** (§III) — combines a discrete-coordinate skeleton with **modal (distributed-parameter) coordinates** for each flexible appendage. The vehicle equations couple rigid-body 6-DOF with the modal amplitudes, producing the inertia, Coriolis, modified-stiffness, and forcing matrices (Figs. 5-8) of the coupled flexible spacecraft.
3. **Vehicle normal-coordinate methods** (§IV) — treats the entire spacecraft (rigid + flexible) as a single linearized normal-mode system. Efficient for linear control synthesis; loses fidelity for nonlinear large-rotation motions.

Likins' verdict: **hybrid-coordinate has the widest range of practical application** — discrete-coordinate-level versatility for the rigid skeleton, normal-mode-level efficiency for the flexible appendages.

## Why this matters for Apsis

The Apsis problem statement (architecture §1) explicitly calls for **multi-body dynamics with flexible appendages** — solar arrays, antennas, deployable booms whose first elastic modes can be 0.1-2 Hz, comparable to ACS bandwidth. The choices Likins surveys are still the choices facing Apsis today, just with modern algorithmic backing:

| Likins method | Apsis corollary |
|---|---|
| Augmented-body / nested-body | [[concepts/articulated-body-algorithm|ABA]] / [[concepts/recursive-newton-euler-algorithm|RNEA]] in [[concepts/pinocchio-library\|Pinocchio]] — modern O(n) successors |
| Hybrid-coordinate | Floating-base Pinocchio rigid skeleton + per-appendage modal-amplitude integrators coupled at the appendage roots |
| Vehicle normal-mode | LTI spacecraft model for control synthesis only |

The hybrid-coordinate method *as a modeling pattern* is exactly what Apsis needs: the ABA/RNEA core handles the rigid topology and base motion; flexible appendages contribute modal-amplitude DOFs that are integrated alongside the joint coordinates with appendage-base coupling forces fed in as generalized forces. Modern frameworks (Pinocchio + custom modal integrator) realize this pattern with vastly better numerical conditioning and computational efficiency than Likins' 1970 contemporaries.

## Apsis relevance

- **REQ-MBD-001/002** (multi-body dynamics with flexible appendages) — this report is the canonical reference. Subsystems §4 should cite Likins' hybrid-coordinate framing as the architectural pattern even though the underlying algorithms (ABA via Pinocchio) are modern.
- **REQ-GNC-007** (control-structure interaction): Likins §V (Control System Simulation, including root-locus analysis with vehicle flexibility) is directly applicable — flexible-mode poles in the loop affect ACS phase margin.
- **REQ-OBS-***  modal observability: Likins §III.D.6-7 (modal analysis of nonrotating structures using cantilever modes) gives the mode-shape basis that on-board sensors (accelerometers, fiber-optic strain) observe.
- **Subsystems §5 (ACS) interaction with §4 (dynamics)**: reading Likins is the classical preparation for understanding why modern spacecraft ACS designs pre-filter outside the flexible-mode passbands.

## Items for human review (no silent spec edits)

- Architecture §3 Multi-Body Dynamics calls for *"rigid + flexible-appendage"* but does not specify the modal-coordinate integration approach. Worth a sentence: "hybrid-coordinate-style coupling, with cantilever modes for unrestrained appendages and constrained-mode formulations for tightly-coupled ones (Likins 1970, §III.D.6-7)."
- Subsystems §5.5 "Pointing budget" should account for flexible-mode contribution to LOS jitter — a standard Likins outcome that requires modal damping ratios as scenario inputs.

## Cross-references

- [[concepts/floating-base-dynamics]] — modern framing of Likins' "vehicle equations".
- [[concepts/articulated-body-algorithm]] / [[concepts/recursive-newton-euler-algorithm]] — modern O(n) successors to augmented- and nested-body methods.
- [[sources/carpentier-2019-pinocchio]] — current implementation vehicle for Apsis MBD.
- [[sources/mistry-2010-floating-base-inverse-dynamics]] — floating-base inverse dynamics for spacecraft (next ingest).
