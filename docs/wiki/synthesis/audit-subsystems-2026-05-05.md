---
type: synthesis
title: "Audit of 02-subsystems.md against the wiki corpus"
date: 2026-05-05
audited_doc: docs/02-subsystems.md
audited_doc_status: "Companion to 01-architecture.md"
---

# Audit — 02-subsystems.md (2026-05-05)

A systematic review of `docs/02-subsystems.md` against the wiki corpus. Subsystems is the most detailed of the three docs; many findings are concrete enough to act on without further design discussion.

Severity tags as in the REQUIREMENTS audit. Read [[synthesis/audit-requirements-2026-05-05]] and [[synthesis/audit-architecture-2026-05-05]] first; some findings here are downstream of those.

## §1 Time and frames

### S1.1 — Five time scales (TCG/TCB gap) [LOW]

§1.1 lists TAI/TT/UTC/UT1/TDB. Same as F1.1.

### S1.2 — Two-component time precision claim [no finding]

§1.2: *"preserves nanosecond precision for offsets up to ±10⁷ seconds (roughly four months) before requiring epoch rectification."*

Verified: 1 ns / 10⁷ s ≈ 10⁻¹⁶ ≈ 0.5 ULP at f64. ✓ aligned with [[sources/sofa-2023-time-scale-cookbook|SOFA cookbook]] §2.3.

### S1.3 — "ICRF/J2000" conflation [MEDIUM]

§1.3: *"Inertial root: ICRF / J2000."* Same as F1.2 / A3.1.

### S1.3 — Body-frame convention [LOW]

§1.3: *"Vehicle body frame (X forward, Y right, Z down — aircraft convention)."*

The aircraft NED convention is not standard for spacecraft. Common spacecraft body-frame conventions vary: some align +Z to nadir, +X to velocity (LVLH-natural); others align to instrument boresight (mission-specific); the +X-forward / +Y-right / +Z-down "aircraft" convention is rare in practice for satellites.

**Resolution:** either justify the aircraft convention explicitly (consistency with re-entry / flight-vehicle cases?), or pick a more standard spacecraft convention (e.g., Z-nadir LVLH-aligned, with body-fixed instrument frames defined relative).

### S1.4 — "f64 has ~mm precision at LEO altitudes" understated [LOW]

§1.4: *"f64 has ~mm precision at LEO altitudes."*

Actual: r = 7×10⁶ m, ULP ≈ 7×10⁶ × 2⁻⁵² ≈ 1.5×10⁻⁹ m = 1.5 nm. So f64 precision at LEO is ~6 orders of magnitude better than mm. The "mm precision" framing implies the limit is f64 itself, but the real limits at the mm level are everything else (tides, ephemeris, force-model truncation).

**Resolution:** revise to clarify that f64 itself supports nm precision; mm-level uncertainty floor at LEO comes from physical model fidelity, not arithmetic.

### S1.4 — "transition is instantaneous" needs SOI nuance [LOW]

§1.4: *"the transition is instantaneous and the state in both frames is mathematically identical."*

True for CCI ↔ CCF rotations. For CCI ↔ HCI (SOI crossing), the two-body assumption used to define SOI is approximate; in reality the spacecraft's three-body state has a continuous Cartesian position but the *primary attractor switches discontinuously*. The propagator-internal force balance changes; integrator settings may need adjustment at the boundary.

**Resolution:** add a sentence on SOI-crossing precision (the transition is mathematically continuous in position but the integrator may need to re-bracket adaptive step size at the boundary).

## §2 Force models

### S2.2 — Tides off-by-default conflicts with Pc precision [MEDIUM]

§2.2: *"Solid Earth tides, ocean tides, pole tides. Optional per IERS Conventions 2010. Centimeter-level effects; off by default."*

Same as F2.3. For self-consistent CA ops with M-priority Pc requirements, ~10 cm position effects from tides should be on-by-default for that scenario class.

**Resolution:** either tier the default (off for most scenarios, on for CA pipeline), or reconsider per-requirement.

### S2.2 — Permanent-tide convention not specified [MEDIUM]

§2.2 doesn't specify the permanent-tide convention. Same as F2.5.

**Resolution:** add explicit choice (recommend zero-tide per IERS Conventions default).

### S2.2 — Lunar gravity model name [LOW]

§2.2: *"Lunar: GRGM1200A to 165×165 for low orbits."*

The corpus has [[sources/lemoine-2014-grgm900c|GRGM900C]] (degree 900). GRGM1200A (degree 1200) is the successor. Methodology unchanged. Apsis can use either; the name in subsystems is fine but the corpus reference paper for GRGM1200A specifically should be ingested if available.

**Resolution:** ingest GRGM1200A reference paper if accessible; otherwise document that GRGM900C is the source-cited predecessor and methodology applies.

### S2.5 — Relativistic-corrections defaults not specified [LOW]

§2.5: *"Schwarzschild correction... included when relativistic flag is set."*

Doesn't specify default-on vs default-off. Per [[sources/iers-conventions-2010|IERS Conventions]] Ch. 10, Schwarzschild is standard for high-precision force models.

**Resolution:** explicitly state default-on for Schwarzschild gravitational acceleration; default-off for Shapiro/light-time except deep-space scenarios.

### S2 — Atmospheric drag panel-model citation gap [LOW]

§2.3 mentions *"panel model summing drag contributions over each spacecraft surface based on attitude"* as the high-fidelity option, but the canonical multi-panel drag-coefficient theory (e.g., Doornbos for accommodation coefficients, Sentman for diffuse reflection) is not in the corpus.

**Resolution:** ingest a panel-drag reference if Apsis intends to ship panel-drag at v1; otherwise note that v1 ships cannonball only.

## §3 Integrators

### S3 — All findings consistent with REQUIREMENTS [no finding]

Selection guidance ✓, integrator interface ✓, compensated summation ✓, Encke formulation ✓ aligned with corpus and REQUIREMENTS.

## §4 Spacecraft and multi-body

### S4 — Sidecar YAML format spec is custom [LOW]

§4.1 introduces a YAML sidecar format for effectors and sensors that is Apsis-specific (no spec in corpus). This is the right approach (URDF doesn't natively model effectors / sensors), but the format itself is original work — not a finding against the corpus.

**Resolution:** document the format as a versioned schema in Apsis.

### S4.4 — Mass-property rebuild threshold worth specifying [LOW]

§4.4: *"Pinocchio's `Model` is rebuilt when mass properties change (rebuilt only on significant changes — typically >1% mass change)."*

Pinocchio Model rebuild is fast but not free. Choosing 1% as the threshold is a discretization choice — affects CG accuracy during finite burns. For a mission with ~100 kg fuel out of 500 kg dry, 1% = 5 kg = ~1% of CG offset.

**Resolution:** either parameterize the threshold per-scenario, or document why 1% is appropriate.

### S4.5 — CMG specification confused with VSCMG/DGCMG [MEDIUM]

§4.5: *"Control moment gyro. Two angles per CMG (gimbal angle, spin rate). Commanded gimbal rate. Output torque is `ω_cmg × h_cmg`."*

A **single-gimbal CMG** (SGCMG) has *one* gimbal angle (γ) plus a constant-spin wheel. A **double-gimbal CMG** (DGCMG) has two gimbal angles. A **VSCMG** ([[sources/schaub-1998-vscmg|Schaub, Vadali & Junkins 1998]]) has one gimbal + variable wheel rate.

The description "two angles per CMG (gimbal angle, spin rate)" is consistent with VSCMG (one gimbal + variable spin), not "two gimbal angles" — the wording mixes terms. Also no mention of the steering law, which is the actual control problem (singularity avoidance / robustness).

**Resolution:** clarify which CMG variant Apsis supports; recommend VSCMG framework subsumes SGCMG and (with extension) DGCMG. Add reference to Schaub 1998 for the steering law.

### S4.5 — Reaction wheel "momentum coupled via Pinocchio" [LOW]

§4.5 RW: *"Momentum coupled to spacecraft via Pinocchio."*

Conceptually correct: a RW is a one-DOF revolute joint between the wheel link and the bus link in the URDF tree, with the wheel inertia loaded into Pinocchio. The momentum coupling falls out automatically.

**Resolution:** worth a sentence on how this is modeled in URDF (revolute joint, wheel as a link with non-zero inertia about the spin axis), since this is non-obvious.

### S4.6 — Star tracker keepout default values [LOW]

§4.6 doesn't specify keepout half-angle defaults. Typical hardware: 30° Sun, 15-25° Earth, 15-25° Moon (mission-specific).

**Resolution:** add representative defaults; let the YAML config override.

### S4.6 — Magnetometer cross-talk note [LOW]

§4.6 notes magnetorquer cross-talk: *"must zero magnetorquer command before sampling, or model the interference."* Standard ACS-design pattern. ✓ — actionable for users.

## §5 GNC stack

### S5.3 — MEKF 3-vector representation underspecified [LOW]

§5.3: *"MEKF... Error parameterization is a 3-vector (small rotation), so the covariance is non-singular."*

Per [[sources/markley-2003-attitude-error-representations|Markley 2003]], the canonical 3-vector is **MRP** ([[concepts/generalized-rodrigues-parameters|Modified Rodrigues Parameters]]), with the global state as a 4-component quaternion. Subsystems should specify.

Also no mention of **second-order MEKF** (Markley 2003 second half) which is the robustness extension for large errors.

**Resolution:** specify "the 3-vector covariance state is MRP per Markley 2003; default to second-order MEKF as the canonical implementation."

### S5.3 — No MEKF fallback / acquisition-mode handling [MEDIUM]

§5.3 doesn't address MEKF-divergence on large initial errors. Same as F8.1.

**Resolution:** specify USQUE (or UKF-attitude variant) as acquisition-mode fallback.

### S5.4 — PD-on-quaternion as default for science is OK but limit-stated [LOW]

§5.4: *"PD on quaternion error (most common for science modes)."* OK for small angles. For large slews, see F8.2 — Lyapunov-stable nonlinear controller (Schaub 1998).

**Resolution:** make explicit that PD-on-quaternion is for small-angle / pointing-stability modes; large-slew uses a nonlinear controller.

### S5.4 — Sliding-mode "for robustness studies" not in corpus [LOW]

§5.4 mentions sliding-mode control. Standard literature but not specifically ingested. Low.

**Resolution:** ingest a sliding-mode reference if Apsis intends to ship the controller as a built-in.

### S5.5 — Failure injection list complete [no finding]

§5.5 ✓ aligned with REQ-SEN-010 and REQ-GNC-012.

## §6 Catalog and conjunction analysis

### S6.4 — Pc method list aligned [no finding]

§6.4 step 4: *"Foster's, Akella-Alfriend's, or Chan's method."* ✓ aligned with REQ-CAT-009.

### S6.5 — "Small along-track burn" is suboptimal [MEDIUM]

§6.5: *"typically a small along-track burn 1-3 orbits before TCA."* Same as F10.2 — [[sources/bombardelli-2015-collision-avoidance|Bombardelli & Hernando-Ayuso 2015]] provides analytic optimal Δv direction (eigenvector of `MᵀQM`); restricting to along-track is suboptimal.

**Resolution:** allow analytic-optimal CAM per Bombardelli; along-track is a constrained-mode option.

### S6 — CDM ingest not detailed [LOW]

§6 doesn't describe CDM ingest. REQ-CAT-004 (S) requires it; canonical format is [[sources/ccsds-508-0-b-1-cdm|CCSDS 508.0-B-1]].

**Resolution:** add §6.1.1 or extension on CDM ingest format and the HBR per-spacecraft config that CDM doesn't carry (see F10.5).

### S6 — RPO MPC not in §6 [LOW]

§6 covers conjunction screening + CAM. Constrained-trajectory **rendezvous and proximity ops MPC** ([[sources/dicairano-2012-mpc-rendezvous|Di Cairano et al. 2012]]) is structurally different (LOS cone, terminal velocity match, debris avoidance). Belongs in §5 (GNC) or §6 (CA proper). Currently silent.

**Resolution:** add an RPO/proximity-ops controller section, distinct from CAM.

## §7 Monte Carlo

No findings — aligned with REQ-MC and CLAUDE.md determinism mandate.

## §8 Time acceleration

No findings — design is reasonable.

## Summary table

| ID | Severity | Title | Linked to REQ/ARCH finding |
|---|---|---|---|
| S1.1 | LOW | TCG/TCB gap | F1.1 / A3.3 |
| S1.3a | MEDIUM | "ICRF/J2000" conflation | F1.2 / A3.1 |
| S1.3b | LOW | Body-frame aircraft convention non-standard for spacecraft | (subsystems-only) |
| S1.4a | LOW | "f64 ~mm precision at LEO" understated | (subsystems-only) |
| S1.4b | LOW | SOI transition continuity nuance | (subsystems-only) |
| S2.2a | MEDIUM | Tides off-by-default conflicts with Pc precision | F2.3 |
| S2.2b | MEDIUM | Permanent-tide convention not specified | F2.5 |
| S2.2c | LOW | Lunar gravity GRGM1200A vs corpus's GRGM900C | (corpus only) |
| S2.5 | LOW | Relativistic-corrections defaults not specified | F2.4 |
| S2 | LOW | Atmospheric panel-drag reference not in corpus | (corpus only) |
| S4 | LOW | Sidecar YAML format is original work (no audit issue) | — |
| S4.4 | LOW | Mass-property rebuild 1% threshold worth justifying | (subsystems-only) |
| S4.5a | MEDIUM | CMG specification confused with VSCMG/DGCMG | F6.1 |
| S4.5b | LOW | RW URDF modeling not described | (subsystems-only) |
| S4.6a | LOW | Star tracker keepout defaults not specified | (subsystems-only) |
| S5.3a | LOW | MEKF 3-vector type underspecified (should be MRP) | F8.1 |
| S5.3b | MEDIUM | No MEKF fallback for large-error acquisition | F8.1 |
| S5.4a | LOW | PD-on-quaternion default — clarify regime | F8.2 |
| S5.4b | LOW | Sliding-mode reference not in corpus | (corpus only) |
| S6.5 | MEDIUM | "Small along-track burn" is suboptimal CAM | F10.2 |
| S6 | LOW | CDM ingest not detailed | F10.5 |
| S6 | LOW | RPO MPC not in subsystems | F8.3 |

Counts: **0 HIGH, 6 MEDIUM, 16 LOW**.

The MEDIUM cluster centers on (a) frame conflation (S1.3a) — common across all three docs, (b) tides convention and default (S2.2a, S2.2b) — common with REQ, (c) the CMG/VSCMG terminology issue (S4.5a) which is subsystems-specific and worth fixing for clarity, (d) MEKF acquisition-mode fallback (S5.3b) — common with REQ, and (e) the CAM along-track restriction (S6.5) — common with REQ.

Subsystems-specific findings (not in REQ or ARCH) are limited to (a) the body-frame convention choice (S1.3b), (b) the LEO precision claim phrasing (S1.4a), (c) the SOI transition nuance (S1.4b), (d) the mass-property-rebuild threshold (S4.4), (e) the RW URDF modeling note (S4.5b), and (f) star tracker keepout defaults (S4.6a). All LOW.
