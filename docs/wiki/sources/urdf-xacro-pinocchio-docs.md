---
type: source
title: "URDF / Xacro / Pinocchio documentation snapshots"
raw_path: docs/raw/specs/  (urdfdom-docs-snapshot.html, ros-urdf-xml-spec.html, ros2-xacro-tutorial.html, pinocchio-docs-snapshot.html)
source_type: spec
reliability: authoritative
ingested: 2026-05-05
authors: [URDF / Xacro maintainers (Open Robotics); Pinocchio team (LAAS-CNRS / Inria)]
publication_date: 2024
venue: "ROS Documentation, urdfdom docs, Xacro tutorial, Pinocchio documentation snapshots"
---

# URDF / Xacro / Pinocchio documentation snapshots

A bundled snapshot of the **multi-body-dynamics tooling stack** Apsis uses for spacecraft mechanical model description, parsing, and dynamics evaluation:

| File | Topic | Apsis use |
|---|---|---|
| `ros-urdf-xml-spec.html` | URDF XML schema | Spacecraft mechanical model file format |
| `urdfdom-docs-snapshot.html` | urdfdom C++ parser | URDF → in-memory tree |
| `ros2-xacro-tutorial.html` | Xacro macro / templating | URDF authoring with macros, parameters, conditionals |
| `pinocchio-docs-snapshot.html` | [[concepts/pinocchio-library\|Pinocchio]] library docs | RBD algorithms (ABA, RNEA, CRBA), analytical derivatives |

Together these define the **input pipeline**: mechanical configuration written as Xacro → expanded to URDF → parsed by urdfdom into a tree → loaded into Pinocchio as a `pinocchio::Model` → dynamics computed by Pinocchio per-tick.

## URDF XML format

URDF (Unified Robot Description Format) is the ROS-community-standard XML schema for kinematic + dynamic descriptions of articulated robots. Apsis adopts it as the canonical mechanical-model format because:

- Mature ecosystem: visualization (RViz), collision-detection (FCL), planning (MoveIt), dynamics (Pinocchio).
- **Pinocchio reads URDF natively** via its built-in parser.
- Simple to author or auto-generate.

URDF describes:

- **Links** with mass, inertia tensor, COM, visual mesh, collision mesh.
- **Joints** with parent/child links, joint type (revolute, prismatic, fixed, continuous, floating), axis, limits, dynamics (damping, friction).
- **Materials** for visualization.

For spacecraft, the **floating joint** between the world and the spacecraft body root is the [[concepts/floating-base-dynamics|floating-base joint]] from Pinocchio.

## Xacro: practical URDF authoring

Pure URDF is verbose. **Xacro** is a Python-based macro/preprocessor that lets you parameterize URDFs:

- `<xacro:property name="solar_panel_length" value="2.5"/>`
- `<xacro:macro name="reaction_wheel" params="name xyz">...</xacro:macro>`
- `<xacro:include filename="$(find apsis_models)/spacecraft_bus.urdf.xacro"/>`
- Conditional sections via `<xacro:if value="${...}">`.

Apsis ships its mechanical configurations as Xacro files; Xacro preprocesses them to plain URDF at scenario load time.

## Pinocchio: dynamics on the URDF

Once URDF is loaded, [[sources/carpentier-2019-pinocchio|Pinocchio]] provides:

- **Forward dynamics**: `aba(model, data, q, v, tau)` returns `q̈` ([[concepts/articulated-body-algorithm|ABA]]).
- **Inverse dynamics**: `rnea(model, data, q, v, qddot)` returns `tau` ([[concepts/recursive-newton-euler-algorithm|RNEA]]).
- **Mass matrix**: `crba(model, data, q)` (Composite-Rigid-Body Algorithm).
- **Jacobians**: per-frame, per-joint analytical Jacobians.
- **Derivatives**: analytical `∂(forward dynamics)/∂q`, `∂/∂v`, `∂/∂tau` per [[sources/carpentier-2018-rbd-analytical-derivatives]] — needed by MPC, EKF, MEKF.

## Apsis relevance

- **REQ-MBD-001/002** (multi-body dynamics) — Pinocchio is the engine; URDF/Xacro is the input format.
- **REQ-MBD-003** (model authoring) — Xacro provides the macro/parameterization layer.
- **REQ-INT-008** (mechanical model interchange) — URDF makes Apsis interoperable with the ROS ecosystem.
- **Subsystems §4** (MBD) — this stack is the canonical implementation path.
- **Subsystems §1** build-vs-reuse table — link Pinocchio + urdfdom + Xacro; do not re-implement.

## Items for human review

- URDF natively supports only **tree topologies** (no closed kinematic loops). Spacecraft typically *are* tree-structured (no closed loops in the deployed configuration), so this is fine; if Apsis ever needs to model a deployable mechanism with a loop, it must extend or use SDF (Gazebo's superset of URDF that supports loops).
- URDF inertia is specified at link level. **Slosh DOFs** ([[sources/abramson-1966-nasa-sp-106-slosh|Abramson 1966]] / [[sources/dodge-2000-swri-slosh-update|Dodge 2000]]) are added as additional pendulum joints in the URDF tree, anchored at the tank link.
- **Flexible-body modes** ([[sources/likins-1970-flexible-space-vehicles|Likins 1970]]) are *not* in vanilla URDF. Apsis's Xacro extension layer must add them as URDF-extension elements with custom semantics that Apsis's Pinocchio loader recognizes.

## Cross-references

- [[sources/carpentier-2019-pinocchio]] — Pinocchio core paper.
- [[sources/carpentier-2018-rbd-analytical-derivatives]] — Pinocchio's analytical derivatives.
- [[concepts/pinocchio-library]] / [[concepts/articulated-body-algorithm]] / [[concepts/floating-base-dynamics]] — Apsis MBD concepts.
- [[sources/likins-1970-flexible-space-vehicles]] — flexible-mode extension Apsis adds beyond vanilla URDF.
- [[sources/abramson-1966-nasa-sp-106-slosh]] / [[sources/dodge-2000-swri-slosh-update]] — slosh DOFs added as URDF pendulum joints.
