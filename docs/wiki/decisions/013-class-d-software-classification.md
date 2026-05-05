---
type: decision
title: "Software classification: NPR 7150.2D Class D (mission-support / engineering) with reasonable conventions"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: []
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Establishes the NASA software-classification posture
that frames every other process / standards-adherence decision in the
project. Implemented by `docs/process/software-management-plan.md`,
`docs/process/software-development-plan.md`, and
`docs/process/software-test-plan.md`.

## Context

NPR 7150.2D defines five software classes (A through E) ranging from
human-rated flight software (Class A — DO-178C-equivalent rigor, formal
methods, IV&V) to engineering / analysis tools (Class E — minimal
process). Each class drives a different envelope of mandatory practices:
coding standards, static analysis, testing rigor, traceability,
configuration management, independent verification & validation.

Apsis's design overview describes a **flight-dynamics-grade spaceflight
simulator** — "flight-dynamics-grade" referring to the *precision of
the physics*, not the safety classification of the executing software.
Apsis is ground software: a simulation, analysis, and verification
tool. It will never be loaded into a spacecraft flight computer. Its
outputs feed mission analysis and GNC algorithm development; the
algorithms it helps validate are *separately* classified by the
projects that adopt them.

The classification needs to reflect what Apsis actually is, not the
fidelity of the physics it models or the seriousness of the missions
that consume its outputs.

## Decision

Apsis is **Class D** under NPR 7150.2D — mission-support / engineering
software running on ground systems, used to inform mission-critical
decisions but not itself executing in a safety-critical context.

The Class D envelope drives the following mandatory practices, each
addressed by a referenced project artefact:

| NPR 7150.2D Class D requirement                              | Apsis artefact                                                |
| ------------------------------------------------------------ | ------------------------------------------------------------- |
| Software Management Plan (SMP)                               | `docs/process/software-management-plan.md`                    |
| Software Development Plan (SDP)                              | `docs/process/software-development-plan.md`                   |
| Software Test Plan (STP)                                     | `docs/process/software-test-plan.md`                          |
| Requirements management with traceability                    | `docs/REQUIREMENTS.md` `REQ-*` IDs; ADR + test frontmatter    |
| Configuration management                                     | git + GitHub; `cmake/CPM_VERSION` / `external/*/PINNED_VERSION` for deps |
| Coding standards                                             | `.clang-format`, `.clang-tidy`, project conventions in ADRs   |
| Peer review                                                  | GitHub PR review process                                      |
| Static analysis                                              | clang-tidy gate in CI                                         |
| Unit + regression testing                                    | GoogleTest + CTest; `tests/{unit,conformance,regression}/`    |
| Code coverage measurement                                    | gcov gate in CI (Phase 1 followup; minimum 80% statement)     |
| Anomaly tracking                                             | GitHub issues                                                 |
| Software classification documented and justified             | This ADR                                                      |

Class A/B (flight-grade) practices that Apsis **does not** adopt:
formal methods, independent V&V, MISRA C++ conformance, JPL Power-of-10
no-heap-after-init / no-recursion / no-function-pointer rules,
DO-178C-equivalent process. Each of these would invalidate one or more
existing ADRs (012 — Eigen heap allocation; 005 — strategy interfaces;
009 — recursive force-model evaluation; 007 — GoogleTest exception use;
etc.) and would be a 6–12 month structural reframe with permanent
overhead. None are warranted for ground software.

## Rationale

- **Classification follows blast radius, not physics fidelity.** A simulator
  whose outputs are wrong wastes engineering time and may cause a
  mission to mis-design hardware; it does not crash a spacecraft. The
  cost of a defect is a re-run, not a loss-of-vehicle event.
  NPR 7150.2D's classification framework is built around exactly this
  distinction.
- **Class D is the right answer for mission-analysis / sim tools.** It
  carries enough rigor to keep a multi-decade-arc, ten-thousand-trial,
  fifty-thousand-object simulator trustworthy without imposing
  flight-grade overhead. Most of NASA's own astrodynamics ground
  software (GMAT, MONTE, Orekit) is Class D-equivalent.
- **The existing project artefacts already cover most of Class D.** The
  REQ-* IDs in `REQUIREMENTS.md`, the ADR system, the wiki citation
  policy, the CI matrix, the clang-tidy / clang-format gates, and the
  conformance-test pattern at every seam are individually below the
  bar of any Class A practice but collectively above the bar for
  Class D. The remaining delta is documentation of the framework
  (SMP / SDP / STP) plus a code-coverage gate.
- **Class C is overkill.** Class C adds mandatory peer-review records
  per change, formal static-analysis reports per release, MC/DC
  coverage on critical components, and IV&V handoff process. None of
  this changes outcomes for a mission-analysis tool whose users are
  themselves the principal reviewers of its outputs.
- **Class E is under-rigorous.** Class E permits skipping SMP / STP
  and reduces testing to ad-hoc. We need the conformance and
  regression-test discipline already in `docs/structure.md` to make
  the long-arc precision claims in `docs/00-design-overview.md`
  defensible.

## Alternatives considered

- **Class C (mission-critical).** Adds mandatory peer-review records
  beyond what GitHub PRs already provide, formal release-time static
  analysis, MC/DC coverage on critical paths, and an IV&V handoff
  process. ~2–3 months of added project lifetime. Rejected because
  the added rigor doesn't change defect-detection outcomes for the
  software class Apsis is — its users review outputs against
  reference cases (JPL DE, ISS vectors, historical conjunction
  events) which is a stronger validation surface than process
  formalism.
- **Class E (engineering / analysis).** Skips formal SMP / SDP / STP,
  reduces testing rigor. Rejected because the regression-test
  discipline at the heart of `docs/structure.md` and the
  long-arc-precision claims in `00-design-overview.md` need a
  documented test plan to be credible to consumers.
- **Class A or B (safety-critical / human-rated).** Inappropriate for
  ground simulation software per NPR 7150.2D's classification
  framework. Would invalidate ADRs 005, 007, 009, 012 and add 6-12
  months of structural rework for guarantees Apsis doesn't need to
  provide.
- **No formal classification — just adopt good engineering practice.**
  Tempting and broadly what the project does already. Rejected
  because (a) NPR 7150.2D Class D is itself "good engineering
  practice plus the discipline of writing it down", and (b) consumers
  of Apsis outputs (mission-analysis teams) reasonably want to know
  the classification posture in writing before relying on results.

## Consequences

- **Three new process documents land alongside this ADR**: SMP, SDP, STP
  under `docs/process/`. Each is skeleton-grade — a real outline that
  gets populated as the project matures, not a one-shot 50-page
  document.
- **Code coverage gate added in Phase 1 CI.** Minimum 80% line coverage
  on first-party code (`src/`, `include/apsis/`); upstream-vendored
  SOFA / CSPICE / Eigen excluded. Tracked as a Phase 1 followup to
  this ADR.
- **Requirements traceability** mechanism: every test that targets a
  specific `REQ-*` ID declares it in a header comment or test
  fixture. A `tools/lint/req_traceability.py` script (Phase 1
  followup) reports REQ IDs without a covering test.
- **No code rewrites under existing ADRs.** ADRs 001–012 all stand.
- **Process artefacts are versioned in git** alongside the code; SMP /
  SDP / STP are themselves subject to the same review process as code
  changes (via `meta:` or `process:` commit prefixes — to be added to
  CLAUDE.md).
- **Software classification rationale is public and reviewable.** Future
  consumers can see exactly what process posture Apsis claims and
  verify the artefacts that support it.
- **Reclassification is possible but deliberate.** If a consumer's use
  case migrates Apsis's outputs into a Class A/B context (e.g. a
  derived component is integrated into flight code), this ADR is
  superseded by a successor that reframes the relevant subset under
  the higher class. The Class D vs. Class A distinction is not
  transitive — flight projects that consume Apsis outputs do their
  own classification of the consuming component independently.
