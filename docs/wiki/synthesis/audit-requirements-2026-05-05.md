---
type: synthesis
title: "Audit of REQUIREMENTS.md against the wiki corpus"
date: 2026-05-05
audited_doc: docs/REQUIREMENTS.md
audited_doc_status: "Draft v0.1"
---

# Audit — REQUIREMENTS.md (2026-05-05)

A systematic review of `docs/REQUIREMENTS.md` (Draft v0.1) against the wiki corpus. Covers all 18 sections plus appendices. Findings are tagged by severity:

- **HIGH** — internal inconsistency that blocks implementation, or contradiction with a load-bearing source.
- **MEDIUM** — gap or conflict that the project's "flight-dynamics-grade" framing makes worth resolving before code.
- **LOW** — text-level fix, version-staleness, or under-specification with low cost-of-being-wrong.

Each finding cites the wiki source(s) supporting it. **No edits are made to REQUIREMENTS.md** — per the CLAUDE.md scope guard, those are deliberate human-or-jointly-authored changes.

## §1 Time and frame management

### F1.1 — Missing TCG and TCB time scales [LOW]

**REQ-TIME-001** (M) names TAI, TT, UTC, UT1, TDB — five scales. The full astronomical time-scale set is seven, adding **TCG** (Geocentric Coordinate Time) and **TCB** (Barycentric Coordinate Time) per [[sources/sofa-2023-time-scale-cookbook|SOFA cookbook]] §3.6. The wiki concept [[concepts/time-scales]] documents all seven.

For typical engineering use, TT and TDB cover what TCG and TCB would have covered (with definitional rate-scale differences). Strict relativistic GCRS/BCRS dynamics would need them.

**Resolution options:** (a) add TCG/TCB at MAY priority for relativistic-strict use cases; (b) document explicitly that Apsis uses TT-rate-scaled GCRS as a deliberate engineering simplification.

### F1.2 — "ICRF/J2000" conflates two different frames [MEDIUM]

**REQ-TIME-005** (M) lists "ICRF/J2000" as a single inertial frame. They differ by the **frame bias** (~17 mas) per [[sources/kaplan-2005-usno-circular-179|Kaplan 2005]] §3 and [[concepts/frame-bias]]. ICRF (current realization [[sources/charlot-2020-icrf3|ICRF3]]) is the IAU-1998+ kinematic frame defined by extragalactic radio sources; J2000 is the equator-and-equinox-of-J2000 dynamical frame.

For mm-precision work (REQ-TIME-009), the 17 mas matters: at LEO, 17 mas × 7000 km ≈ 0.6 m.

The same conflation appears in `01-architecture.md` §3 ("Inertial root: ICRF / J2000") and `02-subsystems.md` §1.3.

**Resolution:** distinguish ICRF (specifically ICRF3) from J2000 throughout, with explicit application of the frame bias.

### F1.3 — REQ-TIME-006 underspecifies the IAU 2006/2000A pipeline form [LOW]

**REQ-TIME-006** (M) requires "IAU 2006/2000A precession-nutation" but doesn't specify CIO-based vs equinox-based. [[decisions/001-use-ceo-based-icrs-to-itrs|ADR-001]] already chose the CIO-based pipeline (currently `proposed`). [[sources/wallace-capitaine-2006-iau2006-procedures|Wallace & Capitaine 2006]] establishes both pipelines as IAU-blessed alternatives.

**Resolution:** cite ADR-001 from REQ-TIME-006, or state "via the CIO-based pipeline implemented by [[sources/sofa-2023-earth-attitude-cookbook|SOFA `iauC2t06a`]]".

### F1.4 — REQ-TIME-009 / REQ-TIME-011 dependency not made explicit [LOW]

**REQ-TIME-009** (M) requires mm-level precision in CCI frames out to 50 AU. **REQ-TIME-011** (C) provides the floating-origin frame mechanism. At 50 AU = 7.5e12 m, f64's relative precision is ~1.5 mm — *just* enough to claim mm-level if frame-partitioning is rigorous, but fragile. The two-component time + Encke perturbation approach (REQ-INT-007, S) is the actual mechanism.

**Resolution:** make REQ-TIME-009 conditional on REQ-INT-007 being met, or explicitly require Encke-style propagation as the mechanism.

## §2 Physics and force models

### F2.1 — REQ-PHY-016 (variational equations) priority too low [HIGH]

**REQ-PHY-016** (S) requires per-force `∂a/∂r` and `∂a/∂v` partials. But:

- **REQ-GNC-004** (M) requires orbit EKF — needs partials for state-transition matrix.
- **REQ-CAT-009** (M) requires Pc computation — needs covariance propagation, which needs partials.
- **REQ-PHY-016** itself acknowledges "for use in variational equations".

If variational equations are used by M-priority requirements, the partials must be available at M priority.

**Resolution:** promote REQ-PHY-016 to M, or document why M-priority orbit estimation/CA can use approximate/finite-difference partials.

### F2.2 — JB2008 indices missing from REQ-ENV-005 [MEDIUM]

**REQ-PHY-008** (S, JB2008 alternative) requires the four UV solar indices F10, S10, M10, Y10 plus Dst per [[sources/bowman-2008-jb2008|Bowman et al. 2008]]. **REQ-ENV-005** (M) lists only "F10.7, Ap, Kp" — sufficient for NRLMSISE-00 only.

JB2008 cannot be implemented without these indices ingested.

**Resolution:** expand REQ-ENV-005 to include JB2008 indices when REQ-PHY-008 is in scope, or note the dependency explicitly.

### F2.3 — Tides priority conflicts with Pc precision claim [MEDIUM]

**REQ-PHY-014** (C, solid Earth + ocean + pole tides) is C-priority. But **REQ-CAT-009** (M) requires Pc computation, and **REQ-CAT-008** (M) requires miss distance to 10 m. Per [[sources/iers-conventions-2010|IERS Conventions]] Ch. 7-8, tides modify station/satellite position by ~10 cm at LEO altitudes — order-of-magnitude with the miss-distance spec.

For self-consistent CA ops at the M-priority level, tides should be at least S.

**Resolution:** consider promoting REQ-PHY-014 to S, or document the systematic error tides-off contributes to the CA pipeline.

### F2.4 — Relativistic corrections priority vs project framing [LOW]

**REQ-PHY-012** (S, Schwarzschild) and **REQ-PHY-013** (C, Lense-Thirring/de Sitter) are below M. [[sources/iers-conventions-2010|IERS Conventions]] Ch. 10 specifies Schwarzschild as standard for high-precision orbit work; the project's "flight-dynamics-grade" claim weakly suggests M for Schwarzschild specifically.

**Resolution:** consider promoting REQ-PHY-012 to M, or document the engineering compromise.

### F2.5 — Permanent-tide convention not specified [MEDIUM]

No requirement specifies which **permanent-tide convention** Apsis uses (zero-tide vs tide-free vs mean-tide) per [[sources/iers-conventions-2010|IERS Conventions]] Ch. 6. Mismatch between Apsis and the input gravity-field file produces ~30 cm offset in C₂₀.

**Resolution:** add an explicit requirement stating which convention Apsis uses (recommend zero-tide per IERS Conventions default).

## §3 Integration and propagation

### F3.1 — REQ-INT-005 doesn't pin SGP4 constants [MEDIUM]

**REQ-INT-005** (M, SGP4) doesn't specify which fundamental constants (WGS-72 vs WGS-84). [[sources/hoots-roehrich-1980-spacetrack-report-3|STR#3]] mandates **WGS-72**, and that's what the operational AFSPC TLE-producing code uses. Modern implementations sometimes drift to WGS-84 causing systematic differences vs operational predictions.

**Resolution:** add subclause "SGP4 SHALL use WGS-72 fundamental constants per STR#3."

### F3.2 — REQ-INT-005 silent on TEME output frame [LOW]

SGP4 returns position/velocity in **TEME** (True Equator Mean Equinox), not GCRS or ITRS. The conversion recipe is in [[sources/vallado-2006-revisiting-spacetrack-3|Vallado et al. 2006]] §VI.

**Resolution:** add subclause requiring TEME-to-target-frame conversion per Vallado 2006 §VI.

## §4 Spacecraft modeling

### F4.1 — Flexible-body modeling priority too low [MEDIUM]

**REQ-SC-005** (C, flexible-body modal coordinates) is C-priority. Per [[sources/likins-1970-flexible-space-vehicles|Likins 1970]] (JPL TR 32-1329), flexible-mode dynamics is *fundamental* for any spacecraft with appendages whose first elastic modes (0.1-2 Hz) overlap ACS bandwidth. Most modern spacecraft fall in this category.

C-priority is too low for a "flight-dynamics-grade" framing.

**Resolution:** promote to S minimum, or restate the project's scope to explicitly exclude flexible-body spacecraft.

### F4.2 — Slosh modeling priority too low [MEDIUM]

**REQ-SC-006** (C, slosh modeling) is C-priority. Per [[sources/abramson-1966-nasa-sp-106-slosh|Abramson 1966]] / [[sources/dodge-2000-swri-slosh-update|Dodge 2000]], slosh is fundamental for fueled craft and the pendulum-equivalent model is well-established. Slosh modes (0.1-2 Hz, very low damping) are squarely in ACS bandwidth.

**Resolution:** promote to S minimum, or restate scope to exclude fueled craft.

### F4.3 — No requirement on contact dynamics [LOW]

No requirement addresses contact dynamics for capture / berthing / landing. [[sources/mistry-2010-floating-base-inverse-dynamics|Mistry et al. 2010]] is the canonical algorithm (orthogonal-decomposition floating-base inverse dynamics).

**Resolution:** if Apsis intends to support capture/berthing/landing scenarios, add an explicit requirement; otherwise mark explicitly OOS in §17.

## §5 Multi-body dynamics

### F5.1 — REQ-MBD-007 (Jacobian, mass matrix) priority [LOW]

**REQ-MBD-007** (S, Jacobian + mass matrix queries) provides what controller design and [[sources/dicairano-2012-mpc-rendezvous|MPC]] need. Same priority as REQ-GNC-008 (MPC, S) — consistent. ✓

No finding.

## §6 Effectors

### F6.1 — VSCMG framework not addressed [LOW]

**REQ-EFF-006** (S, CMG) supports CMG but not [[sources/schaub-1998-vscmg|VSCMG]] (Variable-Speed CMG). VSCMG subsumes RW + CMG + VSCMG configurations under one steering law and provides singularity-robust torque tracking that pure CMG cannot.

**Resolution:** consider adding VSCMG as MAY/C, or note that REQ-EFF-006 is implemented atop a VSCMG framework configured to single-gimbal-with-fixed-spin-rate.

### F6.2 — REQ-EFF-008 magnetorquer references stale field model [LOW]

**REQ-EFF-008** references "configured geomagnetic field" — REQ-ENV-003 says **IGRF-13**. As of 2024, [[sources/igrf14-2024-coefficients|IGRF-14]] is current.

**Resolution:** update REQ-ENV-003 to "IGRF-14 (or current generation)" — see F9.1.

## §7 Sensors

No findings — all sensor requirements align with corpus ([[sources/lefferts-1982-mekf|Farrenkopf gyro model]], etc.).

## §8 Guidance, navigation, control

### F8.1 — MEKF mandate has no fallback [MEDIUM]

**REQ-GNC-003** (M) mandates MEKF without a fallback. Per [[sources/crassidis-2003-ukf-attitude|Crassidis & Markley 2003]] Figure 3, MEKF can fail to converge from large initial errors where USQUE succeeds (8h non-convergence vs 30 min convergence on a single-magnetometer simulation).

[[sources/markley-2003-attitude-error-representations|Markley 2003]] discusses second-order MEKF as a partial mitigation but acknowledges UKF outperforms first-order MEKF on large-error cases.

**Resolution:** add USQUE (or UKF-attitude variant) as required acquisition-mode fallback at S, or constrain REQ-GNC-003 to small-error operating regimes.

### F8.2 — No nonlinear/Lyapunov attitude controller [LOW]

**REQ-GNC-006** (M) requires PD and LQR. Per [[sources/schaub-1998-vscmg|Schaub, Vadali & Junkins 1998]], a globally-asymptotically-stable Lyapunov nonlinear controller (typically MRP- or quaternion-based) is the standard for slew maneuvers from large initial errors. PD-on-quaternion-error is OK for small angles only.

**Resolution:** add nonlinear attitude controller as M or S; PD-on-quaternion is a special case for small-angle regimes.

### F8.3 — REQ-GNC-008 lumps MPC together [LOW]

**REQ-GNC-008** (S, "MPC attitude/orbit controller") lumps two distinct algorithms. [[sources/dicairano-2012-mpc-rendezvous|Di Cairano et al. 2012]] specifically addresses **constrained-trajectory RPO MPC** (LOS cone, terminal velocity match, debris avoidance) which is structurally different from MPC for attitude slew.

**Resolution:** split into REQ-GNC-008a (attitude MPC) and REQ-GNC-008b (RPO/proximity-ops MPC with state constraints).

### F8.4 — No FDIR requirement [LOW]

REQ-SEN-010 and REQ-GNC-012 cover *injection* of sensor/effector failures (S each). No requirement on FDIR (Fault Detection, Isolation, Recovery) *response* logic. Standard for flight-dynamics-grade GNC verification.

**Resolution:** add an FDIR requirement category or explicitly mark OOS in §17.

## §9 Environment models

### F9.1 — REQ-ENV-003 stale (IGRF-13 → IGRF-14) [LOW]

**REQ-ENV-003** specifies IGRF-13. Current generation is [[sources/igrf14-2024-coefficients|IGRF-14]] (released 2024).

**Resolution:** "IGRF-14 (or current generation)".

### F9.2 — REQ-ENV-005 indices don't cover JB2008 [MEDIUM]

See F2.2.

## §10 Catalog and conjunction analysis

### F10.1 — REQ-CAT-008 internal precision unit mismatch [LOW]

**REQ-CAT-008** (M) requires "1 second / 10 m precision." At typical relative velocities (7-15 km/s), 10 m position implies ~1 ms TCA timing — three orders of magnitude tighter than the 1-sec spec. The binding constraint is 10 m.

**Resolution:** restate to "miss distance < 10 m, which implies TCA precision < ~1 ms" or unify the bounds.

### F10.2 — REQ-CAT-011 restriction to along-track suboptimal [MEDIUM]

**REQ-CAT-011** (S) requires "minimum-Δv along-track burns." Per [[sources/bombardelli-2015-collision-avoidance|Bombardelli & Hernando-Ayuso 2015]], the analytic optimal CAM Δv direction is the *eigenvector of `MᵀQM` with maximum eigenvalue* — generally not along-track. Restricting to along-track is suboptimal in many geometries (especially anisotropic covariance).

**Resolution:** allow non-along-track per Bombardelli-2015 analytic optimal, with along-track as a constrained-mode option.

### F10.3 — Akella-Alfriend and Chan methods cited but not in corpus [LOW]

**REQ-CAT-009** (M) names "Foster's, Akella-Alfriend's, or Chan's method." Only [[sources/foster-estes-1992-jsc-25898-pc|Foster]] is in the corpus. Akella-Alfriend and Chan are referenced from [[sources/bombardelli-2015-collision-avoidance|Bombardelli 2015]] §II but not ingested.

**Resolution:** if Apsis intends to implement A-A or Chan, ingest those references; otherwise narrow the requirement to Foster.

### F10.4 — No sub-tracking-threshold debris flux requirement [LOW]

REQ-CAT-001..012 address tracked debris (≥10 cm in LEO via SSN catalog). Mission-design lifetime risk requires statistical sub-cm debris flux per [[sources/krisko-2019-ordem-3-1-user-guide|NASA ORDEM 3.1]].

**Resolution:** add MAY/C MMOD-flux requirement, or explicitly OOS in §17.

### F10.5 — CCSDS CDM ingest format not in REQ-INT-007 details [LOW]

**REQ-CAT-004** (S) requires CDM ingestion, and **REQ-INT-007** (CCSDS message I/O) covers the canonical format per [[sources/ccsds-508-0-b-1-cdm|CCSDS 508.0-B-1]]. CDMs do not carry HBR (Hard-Body Radius); Apsis must maintain a per-spacecraft HBR config alongside CDM ingest.

**Resolution:** add HBR-config requirement (M, since needed for any Pc result).

## §11 Monte Carlo

No findings — all align with corpus and CLAUDE.md determinism mandate.

## §12 Scenario and orchestration

### F12.1 — REQ-SCN-007 priority [LOW]

**REQ-SCN-007** (S, scenario serialize/reload bit-identical) is S-priority. CLAUDE.md emphasizes determinism as non-negotiable for MC; bit-identical reload is part of that.

**Resolution:** consider M.

## §13 Performance

No findings — corpus doesn't directly address performance requirements; the numbers are reasonable based on Vallado's reported SGP4 timing.

## §14 Observability and validation

No findings — REQ-OBS-006 reference cases (ISS, MRO) are accessible via [[sources/naif-spice-required-reading|SPICE]] and CelesTrak.

## §15 Architecture

No findings — non-functional requirements align with CLAUDE.md.

## §16 Extensibility

No findings.

## §17 Out of scope (v1)

### F17.1 — Items implicitly OOS but not listed [LOW]

The findings above identify several capabilities (FDIR response logic, contact dynamics for capture/berthing, sub-tracking-threshold debris flux, advanced relativistic Pc-augmenting tides) that are silent in REQUIREMENTS — neither in scope nor explicitly OOS.

**Resolution:** for each, decide and either add a requirement or add to §17.

## §18 Acceptance criteria

No findings.

## Summary table

| ID | Severity | Title |
|---|---|---|
| F1.1 | LOW | Missing TCG and TCB time scales |
| F1.2 | MEDIUM | "ICRF/J2000" conflates two frames |
| F1.3 | LOW | REQ-TIME-006 underspecifies pipeline form |
| F1.4 | LOW | REQ-TIME-009/011 dependency not explicit |
| F2.1 | HIGH | REQ-PHY-016 variational-equations priority too low |
| F2.2 | MEDIUM | JB2008 indices missing from REQ-ENV-005 |
| F2.3 | MEDIUM | Tides priority conflicts with Pc precision |
| F2.4 | LOW | Relativistic corrections priority vs framing |
| F2.5 | MEDIUM | Permanent-tide convention not specified |
| F3.1 | MEDIUM | REQ-INT-005 doesn't pin SGP4 constants (WGS-72) |
| F3.2 | LOW | REQ-INT-005 silent on TEME output frame |
| F4.1 | MEDIUM | Flexible-body priority too low |
| F4.2 | MEDIUM | Slosh priority too low |
| F4.3 | LOW | No contact dynamics requirement |
| F6.1 | LOW | VSCMG framework not addressed |
| F6.2 | LOW | REQ-EFF-008 references stale field model |
| F8.1 | MEDIUM | MEKF mandate has no fallback |
| F8.2 | LOW | No nonlinear/Lyapunov attitude controller |
| F8.3 | LOW | REQ-GNC-008 lumps MPC together |
| F8.4 | LOW | No FDIR requirement |
| F9.1 | LOW | REQ-ENV-003 stale (IGRF-13 → IGRF-14) |
| F9.2 | MEDIUM | REQ-ENV-005 indices don't cover JB2008 |
| F10.1 | LOW | REQ-CAT-008 internal precision unit mismatch |
| F10.2 | MEDIUM | REQ-CAT-011 along-track restriction suboptimal |
| F10.3 | LOW | A-A and Chan Pc methods cited but not in corpus |
| F10.4 | LOW | No sub-tracking-threshold debris flux requirement |
| F10.5 | LOW | CCSDS CDM HBR-config gap |
| F12.1 | LOW | REQ-SCN-007 priority |
| F17.1 | LOW | Implicit-OOS items not listed in §17 |

Counts: **1 HIGH, 9 MEDIUM, 19 LOW**.

The HIGH finding (F2.1) is the single most actionable: variational-equations partials are required by M-priority orbit estimation and Pc, so they must be M-priority themselves.

The MEDIUM cluster centers on (a) frame-precision conflations (F1.2 + project-wide), (b) priority mismatches between physical model fidelity and downstream M-priority consumers (F2.2, F2.3, F4.1, F4.2, F8.1, F9.2, F10.2), and (c) a few SGP4 / CDM / tides-convention specification gaps (F2.5, F3.1).

LOW findings are mostly text fixes (versioning, citation gaps, minor under-specifications).
