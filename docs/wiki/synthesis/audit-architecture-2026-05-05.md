---
type: synthesis
title: "Audit of 01-architecture.md against the wiki corpus"
date: 2026-05-05
audited_doc: docs/01-architecture.md
audited_doc_status: "Draft v0.1"
---

# Audit — 01-architecture.md (2026-05-05)

A systematic review of `docs/01-architecture.md` (Draft v0.1) against the wiki corpus. The architecture document is derivative of REQUIREMENTS, so most findings here are downstream of [[synthesis/audit-requirements-2026-05-05]] (read that first). This audit focuses on the architecture-specific findings: the dependency table, the framing of subsystem boundaries, and internal consistency between architecture and REQUIREMENTS.

Severity tags as in the REQUIREMENTS audit (HIGH / MEDIUM / LOW).

## §3 Foundation

### A3.1 — "ICRF/J2000" conflation [MEDIUM]

§3 Foundation > Frames: *"Inertial root: ICRF/J2000."*

Same finding as [[synthesis/audit-requirements-2026-05-05|F1.2]]: ICRF and J2000 differ by the [[concepts/frame-bias|frame bias]] (~17 mas). Per [[sources/kaplan-2005-usno-circular-179|Kaplan 2005]] §3 they are distinct frames. Architecture should distinguish.

**Resolution:** distinguish in the architecture doc and propagate the correction to subsystems §1.3.

### A3.2 — IAU 2006/2000A pipeline form not specified [LOW]

§3 Foundation > Frames: *"IAU 2006/2000A precession-nutation for ICRF↔ITRF."*

Same as F1.3 — doesn't say CIO-based vs equinox-based. [[decisions/001-use-ceo-based-icrs-to-itrs|ADR-001]] picked CIO-based. Architecture should cite the ADR.

**Resolution:** reference ADR-001 here.

### A3.3 — Time scales gap (TCG/TCB) [LOW]

§3 Foundation > Time system lists *"TAI/TT/UTC/UT1/TDB"*. Same finding as [[synthesis/audit-requirements-2026-05-05|F1.1]]: TCG and TCB are missing.

## §3 Dynamics core

### A3.4 — `RelativisticCorrection` listed but priority unclear [LOW]

§3 Dynamics core lists `RelativisticCorrection` as a force-model class. In REQUIREMENTS this maps to **REQ-PHY-012** (S, Schwarzschild) and **REQ-PHY-013** (C, Lense-Thirring/de Sitter). The architecture treats it as a peer of other force models.

Per [[sources/iers-conventions-2010|IERS Conventions]] Ch. 10, Schwarzschild is standard in high-precision force models. Architecture promotion of it to "always present, can be disabled" matches the engineering norm; REQUIREMENTS demoting to S/C is the inconsistency (see audit F2.4).

**Resolution:** consistent with F2.4 — promote REQ-PHY-012 to M; the architecture is right.

### A3.5 — State conversions complete [no finding]

§3 Dynamics core > State conversions lists Cartesian, Keplerian, modified equinoctial, Brouwer-Lyddane, TLE-compatible, plus quaternion ↔ DCM ↔ Euler ↔ MRPs. This matches the corpus completely ([[sources/brouwer-1959-artificial-satellite-theory|Brouwer]] + [[sources/lyddane-1963-small-eccentricity-inclination|Lyddane]] for B-L; [[sources/markley-2003-attitude-error-representations|Markley 2003]] + [[concepts/generalized-rodrigues-parameters]] for MRPs).

✓

## §3 ECS world

### A3.6 — "Debris — J2-only, lightweight" tier ambiguous [LOW]

The fidelity tier *"Debris — J2-only, lightweight"* doesn't distinguish between:

- **Tracked debris**: in the SSN catalog, propagated via [[concepts/sgp4|SGP4]], a separate tier from "Debris" (this is `CatalogObject` in the same architecture).
- **Statistical sub-cm debris flux**: per [[sources/krisko-2019-ordem-3-1-user-guide|NASA ORDEM 3.1]] — not deterministically propagated at all; treated as a probability density.

A "J2-only" propagation tier sits between SGP4 (which has J2 + drag + lunisolar + resonance for SDP4) and the SSN-untrackable statistical debris. It's not clear what "Debris" is.

**Resolution:** clarify what "Debris" tier represents and how it differs from `CatalogObject`. If it's "untracked-but-deterministic synthetic debris for testing," that's fine; should be stated.

## §3 Spacecraft internals

### A3.7 — "URDF flexibility (modal coordinates) is out of scope for v1" — conflict with REQ-SC-005 [MEDIUM]

§3 Spacecraft internals: *"URDF flexibility (modal coordinates) is out of scope for v1; rigid-body only."*

**REQ-SC-005** says MAY/C, which means "optional, deferred to v2." Architecture's "out of scope for v1" is consistent with REQ-SC-005's C priority but inconsistent with the [[synthesis/audit-requirements-2026-05-05|F4.1]] recommendation to promote flex to S.

Both docs are internally consistent at the moment (low MAY/C in REQ → OOS-v1 in arch). The audit-F4.1 question is whether C is the right priority given the project framing. If F4.1 is accepted (promote to S), then this architecture statement also needs revision.

**Resolution:** decide F4.1 (priority of flex), then update both docs together.

## §5 External dependencies (build-vs-reuse table)

### A5.1 — IGRF-13 row stale [LOW]

The dependency table lists *"Geomagnetic field | IGRF-13 reference | Free FORTRAN, C ports"*. Current generation is [[sources/igrf14-2024-coefficients|IGRF-14]] (2024).

**Resolution:** update to "IGRF-14 (or current generation)".

### A5.2 — Pinocchio description undersells analytical derivatives [LOW]

The dependency table lists *"Multi-body dynamics | Pinocchio (C++) | Featherstone algorithms, autodiff support"*. Pinocchio's headline differentiator is **analytical derivatives** ([[sources/carpentier-2018-rbd-analytical-derivatives|Carpentier & Mansard 2018]]) — 30-60% faster than autodiff+codegen, numerically exact. Calling out only "autodiff support" undersells what makes Pinocchio worth choosing for Apsis (since MPC and EKF need derivatives).

**Resolution:** revise to "Featherstone algorithms (ABA / RNEA / CRBA), analytical derivatives, autodiff support".

### A5.3 — Vallado SGP4 row should pin WGS-72 [LOW]

The dependency table lists *"TLE / SGP4 | Vallado SGP4 reference | Canonical implementation"*. Should specify WGS-72 fundamental constants per [[sources/hoots-roehrich-1980-spacetrack-report-3|STR#3]] (see F3.1).

**Resolution:** revise to "Vallado SGP4 reference (WGS-72 constants)".

### A5.4 — Missing dependency rows [LOW]

The dependency table is missing entries for:

- **JB2008** (atmosphere alternative, REQ-PHY-008 S) — distributed by Space Environment Technologies.
- **CCSDS CDM parser** — REQ-CAT-004 (S) + REQ-INT-007 require ingest of CCSDS CDM format ([[sources/ccsds-508-0-b-1-cdm]]). No third-party C++ library is canonical here; Apsis may write a custom parser. Either choice should be in the table.
- **NRLMSISE-00 reference C/Fortran** — listed under "Atmospheric density" ✓.
- **NAIF SPICE / CSPICE** — listed under "Ephemerides" ✓.
- **SOFA C source** — listed under "Time conversions, IAU models" ✓.
- **ICGEM** — runtime data source for non-vendored gravity coefficients. Optional; mention if relevant.

**Resolution:** add JB2008 and CCSDS CDM rows; consider ICGEM mention.

## §6 What gets built

No findings. The novel-engineering list aligns with the corpus understanding of where the build value lives.

## §7 Phased build plan

### A7.1 — Phase 3 (Multi-body / URDF) doesn't mention slosh [LOW]

Phase 3 covers "Pinocchio integration, floating-base coupling, effectors, validate angular momentum conservation." Slosh ([[sources/abramson-1966-nasa-sp-106-slosh|Abramson 1966]] / [[sources/dodge-2000-swri-slosh-update|Dodge 2000]]) is implementable as additional pendulum DOFs in the Pinocchio tree; should be in Phase 3 if REQ-SC-006 is promoted (see F4.2).

**Resolution:** if F4.2 is accepted (promote slosh to S), add slosh to Phase 3 deliverable.

## §1, §2, §4 (Goals, Design principles, Not-this)

No findings — these sections describe positioning and design philosophy, not testable specifications.

## Summary table

| ID | Severity | Title | Linked to REQ finding |
|---|---|---|---|
| A3.1 | MEDIUM | "ICRF/J2000" conflation | F1.2 |
| A3.2 | LOW | IAU 2006/2000A pipeline form not specified | F1.3 |
| A3.3 | LOW | Time scales gap (TCG/TCB) | F1.1 |
| A3.4 | LOW | RelativisticCorrection priority inconsistency | F2.4 |
| A3.6 | LOW | "Debris" tier ambiguous | F10.4 |
| A3.7 | MEDIUM | URDF flexibility OOS conflicts with REQ-SC-005 priority question | F4.1 |
| A5.1 | LOW | IGRF-13 row stale | F9.1 |
| A5.2 | LOW | Pinocchio description undersells analytical derivatives | (corpus only) |
| A5.3 | LOW | Vallado SGP4 row should pin WGS-72 | F3.1 |
| A5.4 | LOW | Missing dependency rows (JB2008, CDM parser) | F2.2, F10.5 |
| A7.1 | LOW | Phase 3 doesn't mention slosh | F4.2 |

Counts: **0 HIGH, 2 MEDIUM, 9 LOW**.

Most architecture findings are downstream propagations of REQUIREMENTS findings; resolve REQUIREMENTS first, then update architecture for consistency. The two MEDIUM findings (A3.1 and A3.7) need attention regardless: A3.1 is the same frame conflation as F1.2; A3.7 is the architecture-vs-REQ priority inconsistency that needs decision.

Architecture-only findings (not derived from REQ) are limited to **A5.2** (Pinocchio description) and a few text-level LOW items.
