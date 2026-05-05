---
type: source
title: "The Dynamic Behavior of Liquids in Moving Containers, with Applications to Space Vehicle Technology"
raw_path: docs/raw/papers/abramson-1966-nasa-sp-106-slosh.pdf
source_type: technical-report
reliability: authoritative
ingested: 2026-05-04
authors: [Abramson, H. Norman (editor)]
publication_date: 1966
venue: "NASA SP-106 (Southwest Research Institute, contract NASr-94(07))"
---

# Abramson (1966) — NASA SP-106: Dynamic behavior of liquids in moving containers

The foundational NASA monograph on **propellant slosh dynamics** in space-vehicle tanks. Compiled at SwRI under Norman Abramson, this is the canonical reference that flight-dynamics codes have used for the past six decades to construct **equivalent mechanical models** (pendulum, spring-mass) of liquid slosh that couple into spacecraft attitude and translation equations. Updated in [[sources/dodge-2000-swri-slosh-update]] (next ingest) which Apsis should consult for the modern revisions, but the conceptual framework Apsis builds on is from this report.

## What the report covers

- **Linearized lateral slosh** in cylindrical, spherical, and spheroidal tanks of various boundary conditions.
- **Nonlinear large-amplitude slosh** including the rotary, swirl, and breaking-wave phenomena pictured in the frontispiece.
- **Damping** mechanisms: viscous wall damping, baffle damping, anti-slosh ring effectiveness.
- **Equivalent mechanical models** — pendulum, spring-mass, free-surface modal coordinates — that reduce the continuum free-surface problem to ODEs that can be integrated alongside the rigid-body equations.
- **Coupling with vehicle dynamics**: how slosh modes couple into rigid-body attitude (roll, pitch, yaw) and translation, producing the characteristic slosh poles in the control-system root locus.
- **Low-gravity behavior**: capillary-dominated regime where surface tension shapes the free surface; relevant for upper stages, microgravity coast phases.

## The equivalent-mechanical-model abstraction

The conceptual contribution that survives in modern flight software:

For each tank, replace the liquid with:
- A **rigid sloshing mass** at a fixed location (the "non-sloshing fraction").
- One or more **pendulum (or spring-mass) modes** representing the dominant lateral-sloshing motions, each with an effective mass `m_n`, suspension length `L_n` (pendulum) or stiffness `k_n` (spring-mass), and damping ratio `ζ_n`.

Mass, suspension length, and damping are tabulated functions of tank geometry, fill fraction, and effective gravity. The resulting equivalent mechanical system has the **same coupling forces and moments on the vehicle** as the actual liquid, in the linear regime.

Apsis adds these slosh DOFs to its [[concepts/floating-base-dynamics|floating-base]] [[concepts/articulated-body-algorithm|ABA]] state vector, treating each pendulum as a 1-DOF or 2-DOF joint anchored at the tank center.

## Apsis relevance

- **REQ-MBD-006** (propellant slosh modeling) — this report is the canonical reference. Subsystems §4.4 specifies "pendulum-equivalent slosh model"; this is *that* model.
- **REQ-GNC-007** (control-structure interaction): slosh modes typically have natural frequencies of 0.1-2 Hz with very low damping (<0.5%). They sit squarely in ACS bandwidth → must be modeled to avoid instability. Notch-filter design and gain-margin analysis depend on the equivalent-mechanical-model parameters from this report.
- **REQ-PHY-009** (propellant mass tracking): even without slosh, propellant CG location and inertia change as fuel depletes. The static portion of this report's models (non-sloshing mass + CG) supplies the time-varying inertia function.
- **REQ-MC-008** (Monte Carlo over slosh-parameter uncertainty): pendulum-mass and damping are uncertain, especially in low-g; Monte Carlo over their distributions is standard practice.

## When Abramson's models break

- **Highly nonlinear slosh**: large amplitudes, breaking waves, rotary swirl — equivalent mechanical models are linearizations and lose accuracy. CFD or smoothed-particle hydrodynamics (SPH) needed for those regimes.
- **Microgravity capillary-dominated**: surface tension dominates over inertia; free-surface shape is determined by Bond number. Apsis subsystems §4.4 should specify Bond-number switching from inertial (high-g) to capillary (low-g) models.
- **Diaphragm / bladder tanks**: the liquid is constrained by a flexible boundary, not a free surface — different physics; this report touches on it but specialized references are preferred.

## Cross-references

- [[sources/dodge-2000-swri-slosh-update]] — the modern SwRI update with current modeling practice and CFD comparisons (next ingest).
- [[sources/likins-1970-flexible-space-vehicles]] — for the broader rigid + flexible + slosh + control coupled model.
- [[concepts/floating-base-dynamics]] — Apsis's framework for adding slosh DOFs to the spacecraft equations.
