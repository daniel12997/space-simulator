---
type: synthesis
title: "Audit summary — REQUIREMENTS, architecture, subsystems (2026-05-05)"
date: 2026-05-05
---

# Audit summary — 2026-05-05

Cross-document summary of the three audits done 2026-05-05 against the wiki corpus:

- [[synthesis/audit-requirements-2026-05-05]] — 1 HIGH, 9 MEDIUM, 19 LOW
- [[synthesis/audit-architecture-2026-05-05]] — 0 HIGH, 2 MEDIUM, 9 LOW
- [[synthesis/audit-subsystems-2026-05-05]] — 0 HIGH, 6 MEDIUM, 16 LOW

Total **62 findings** (1 HIGH, 17 MEDIUM, 44 LOW). Many MEDIUM/LOW findings cross multiple docs — the cross-doc count of unique issues is smaller (see clusters below).

## The single HIGH finding

**F2.1 — REQ-PHY-016 variational-equations partials priority too low.** S-priority partials block M-priority orbit estimation (REQ-GNC-004) and M-priority Pc computation (REQ-CAT-009). Promote to M.

## MEDIUM findings clustered

### Cluster A — Frame precision (3 docs)

**F1.2 / A3.1 / S1.3a** — "ICRF/J2000" conflation. Three docs treat ICRF and J2000 as one frame; they differ by the [[concepts/frame-bias|frame bias]] (~17 mas, ~0.6 m at LEO). Resolution: distinguish across all three docs, plus update [[decisions/001-use-ceo-based-icrs-to-itrs|ADR-001]] frame-naming as needed.

### Cluster B — Tides + Pc precision (REQ + subsystems)

**F2.3 / S2.2a** — tides at C priority but Pc / miss distance at M priority. ~10 cm tidal effects at LEO are order-of-magnitude with the 10-m miss-distance spec.

**F2.5 / S2.2b** — permanent-tide convention not specified. Mismatch ≈ 30 cm in C₂₀.

Resolution: pick zero-tide convention; consider promoting REQ-PHY-014 to S; default tides on for CA pipeline scenarios.

### Cluster C — Atmosphere indices (REQ-only)

**F2.2 / F9.2** — JB2008 needs F10/S10/M10/Y10/Dst; REQ-ENV-005 only provides F10.7/Ap/Kp. Either expand index requirement or drop REQ-PHY-008.

### Cluster D — SGP4 specification (REQ + arch)

**F3.1 / A5.3** — REQ-INT-005 doesn't pin WGS-72 constants per STR#3. Easy fix.

### Cluster E — Spacecraft fidelity scope (REQ + arch)

**F4.1 / A3.7** — flexible-body C-priority vs project framing. Per Likins 1970, fundamental for any spacecraft with appendages whose modes overlap ACS bandwidth (most modern spacecraft).

**F4.2** — slosh C-priority vs project framing. Per Abramson/Dodge, fundamental for fueled craft.

Resolution: decide whether "flight-dynamics-grade" includes flex+slosh by default (recommend yes → promote to S); update arch §3 OOS statement consistently.

### Cluster F — MEKF robustness (REQ + subsystems)

**F8.1 / S5.3b** — MEKF mandate has no fallback. Per Crassidis 2003, MEKF can fail to converge from large initial errors. Add USQUE acquisition-mode fallback.

### Cluster G — CAM optimization (REQ + subsystems)

**F10.2 / S6.5** — restriction to "small along-track burn" is suboptimal. Per Bombardelli 2015, analytic optimal is the eigenvector of `MᵀQM`. Allow non-along-track.

### Cluster H — CMG / VSCMG terminology (subsystems-only MEDIUM)

**S4.5a** — subsystems §4.5 description of CMG mixes single-gimbal-CMG, double-gimbal-CMG, and VSCMG terms. Per Schaub 1998, VSCMG framework subsumes all; recommend specifying.

## LOW findings worth bundling

- **Stale model versions:** IGRF-13 → IGRF-14 (F9.1, A5.1).
- **Citation completeness:** ADR-001 should be cited from REQ-TIME-006 / arch §3 / sub §1.3 (F1.3, A3.2).
- **Time scales (TCG/TCB):** mentioned in three docs; either add or document the simplification (F1.1, A3.3, S1.1).
- **Pinocchio analytical-derivatives credit** (A5.2, S4.5b).
- **Body-frame convention** (S1.3b), **LEO precision phrasing** (S1.4a), and other subsystems-only LOW items.

## What this audit does not claim

- It does not propose specific text edits to the docs (per CLAUDE.md scope guard, those are deliberate human-or-jointly-authored).
- It does not adjudicate among priority choices (e.g., whether flex/slosh *should* be S or C — that's a project-scope question for the human).
- It does not validate the doc against external standards beyond the corpus (e.g., it doesn't check NASA Software Class B compliance).

## Recommended order of operations

1. **Settle the HIGH finding (F2.1)** — promote REQ-PHY-016 to M. Single-line change in REQUIREMENTS.
2. **Decide the priority questions** (F4.1 flex, F4.2 slosh, F2.4 relativistic, F2.3 tides) — these affect what gets implemented in v1 and what's deferred.
3. **Apply the easy text fixes** — IGRF version, "ICRF/J2000" → "ICRF (ICRF3)", WGS-72 pin on SGP4, JB2008 indices, CDM HBR config, ADR-001 citations.
4. **Address the underspecifications** — permanent-tide convention, CMG/VSCMG terminology, MEKF MRP/second-order/fallback, CAM analytic-optimal.
5. **Re-audit when the docs reach v0.2** — the corpus has stabilized; subsequent audits should be quick deltas.
