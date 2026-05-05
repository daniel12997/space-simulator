---
type: source
title: "The New 'Dynamic Behavior of Liquids in Moving Containers'"
raw_path: docs/raw/papers/dodge-2000-swri-slosh-update.pdf
source_type: technical-report
reliability: authoritative
ingested: 2026-05-04
authors: [Dodge, Franklin T.]
publication_date: 2000
venue: "Southwest Research Institute (SwRI), San Antonio TX"
---

# Dodge (2000) — SwRI update of NASA SP-106 slosh monograph

The modern (2000) revision of [[sources/abramson-1966-nasa-sp-106-slosh|Abramson 1966 NASA SP-106]], prepared by Franklin T. Dodge at SwRI. SP-106 went out of print but remained the only comprehensive reference, so SwRI re-issued an **updated, condensed monograph** focused on the chapters that flight-dynamics simulators actually use: lateral slosh, nonlinear effects, damping, equivalent mechanical models, vehicle stability/control, and **greatly-expanded low-g / zero-g and propellant-management material**. Adds a **new chapter on liquid motions in a spinning tank** — important for spin-stabilized spacecraft. This is the canonical *modern* slosh reference Apsis should consult; SP-106 retains historical authority but Dodge supersedes it for current numerical values and modeling practice.

## What's updated vs SP-106

From the Preface, Dodge focuses on these SP-106 chapters with material updates:

| SP-106 chapter | Dodge update |
|---|---|
| Ch. 2: Lateral sloshing in moving containers | Updated numerics; better damping data |
| Ch. 3: Nonlinear effects in lateral sloshing | New experimental data, better large-amplitude theory |
| Ch. 4: Damping of liquid motions and lateral sloshing | Expanded baffle effectiveness data |
| Ch. 6: Analytical representation by equivalent mechanical models | **Refined model parameters** — what flight software consumes |
| Ch. 7: Vehicle stability and control | Greatly condensed (well-covered elsewhere now) |
| Ch. 11: Liquid propellant behavior at low and zero G | **Greatly expanded** with PMD design and capillary management |
| **NEW**: Liquid motions in a spinning tank | New chapter — spin-stabilized spacecraft application |

Topics omitted from the update: principles of similitude (covered well elsewhere), vertical excitation of tanks (less practical relevance), structural dynamics (covered elsewhere), most of SP-106 Ch.2's lengthy mathematical derivations (replaced by computer-code references).

## What Apsis takes from this

The same equivalent-mechanical-model abstraction as SP-106, but with **modern parameter tables** and **explicit low-g / capillary regime** treatment that Apsis needs for upper-stage and microgravity coast scenarios where Bond number `Bo = ρ g R² / σ < ~10`.

The new spinning-tank chapter applies if Apsis ever models **spin-stabilized configurations** (e.g., older comsats, sounding rockets, certain interplanetary cruise modes). Even a slowly-spinning vehicle (~1 rpm) significantly modifies slosh frequencies and damping vs the non-spinning case.

## Apsis relevance

- **REQ-MBD-006** (propellant slosh modeling) — Dodge supersedes SP-106 as the **modern parameter source**. SP-106 retained for historical context and the topics Dodge omits.
- **REQ-PHY-010** (microgravity propellant management): Dodge's expanded Chapter 11 covers Propellant Management Devices (PMDs), capillary screens, vanes — relevant for any cruise-phase propellant settling.
- **REQ-MBD-007** (spin-stabilized configurations) — Dodge's new spinning-tank chapter is the modeling reference if Apsis supports this configuration class.
- **Bond-number regime switching** — Apsis subsystems §4.4 should switch between inertial-regime models (high-g, this report's main chapters) and capillary-regime models (low-g, Ch. 11) at a configurable Bond number threshold.

## Items for human review (no silent spec edits)

- Subsystems §4.4 currently doesn't address spin-stabilized configurations. If Apsis's mission set might include spin-stabilized spacecraft (some smallsats, sounding rockets), the new spinning-tank slosh model is needed; otherwise it can be omitted for the V1 scope.
- PMD modeling (capillary screens, vanes) is a separate complexity tier from "equivalent pendulum" slosh — Apsis should make explicit whether it covers PMD-equipped tanks or only free-surface tanks. Dodge Ch. 11 is the reference for PMD-equipped.

## Cross-references

- [[sources/abramson-1966-nasa-sp-106-slosh]] — the original 1966 monograph this updates.
- [[sources/likins-1970-flexible-space-vehicles]] — broader flexible + slosh + control coupled-modeling framework.
- [[concepts/floating-base-dynamics]] — Apsis's mechanism for adding slosh DOFs.
