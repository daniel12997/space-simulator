# Apsis Software Management Plan (SMP)

> **Class D** under NPR 7150.2D. Classification rationale and the
> mandatory-practice mapping live in
> [`decisions/013-class-d-software-classification`](../wiki/decisions/013-class-d-software-classification.md).
> This document is the management-level half of the Class D process
> trio (SMP / SDP / STP). The SDP covers *how* development happens; the
> STP covers *how testing happens*; this document covers *how the
> project is run*.
>
> Skeleton document — sections marked **TBD** are deferred to the next
> quarterly review or to a triggering event named inline.

## 1. Project overview

Apsis is a flight-dynamics-grade spaceflight simulator: a C++17 core
with a Python (pybind11) scenario layer targeting four use cases
simultaneously — single-spacecraft mission simulation, GNC
development, deterministic Monte Carlo verification, and catalog-scale
SGP4 conjunction screening. See
[`docs/00-design-overview.md`](../00-design-overview.md) for the full
project design statement and
[`docs/REQUIREMENTS.md`](../REQUIREMENTS.md) for the authoritative
feature requirements.

## 2. Software classification

**Class D — mission-support / engineering software** under NPR 7150.2D.
Ground software, never loaded into a spacecraft flight computer.
Justification, mandatory-practice envelope, and rejected alternatives
in [`decisions/013-class-d-software-classification`](../wiki/decisions/013-class-d-software-classification.md).

Reclassification trigger: if a derived Apsis component is integrated
into flight software, the consuming flight project performs its own
classification of that component. This SMP and the project-wide ADR
remain at Class D for the simulator itself.

## 3. Project organization

- **Maintainer / project lead**: Daniel Parker (<daniel12997@gmail.com>).
- **Reviewers**: project lead plus mission-analysis subject-matter
  experts on a per-PR basis when domain depth is required (e.g.
  attitude estimation, conjunction screening, multi-body dynamics).
- **Decision authority**: project lead, with ADRs as the durable
  record of architectural decisions.

## 4. Software lifecycle model

Iterative, brainstorm → design → structure → plan → implement loop
(QRSPI workflow). Each phase from `docs/structure.md` is a separate
delivery cycle:

1. Phase brainstorming surfaces requirements and trade-offs.
2. Design captured as ADRs (durable) and design-overview revisions.
3. Structure document fixes the slice boundaries and verification
   gates.
4. Per-phase plan documents under `docs/phase-N-plan.md` translate the
   structure outline into concrete file-level work.
5. Implementation lands per-phase in a feature branch, gated on plan
   verification checkboxes.
6. PR review; merge; cut next phase.

This is documented in the QRSPI workflow files under `.claude/skills/`
(externalised tooling) and reflected in the project's commit-prefix
convention (`design:` / `decision:` / `document:` / `phase-N:`).

## 5. Cost / schedule / resources

**TBD** — formalise once the project has a budget envelope. The
design overview's "approximately 6–8 months sustained effort" sets
the order-of-magnitude expectation through Phase 6 (usable
simulator); Phase 7 hardening is open-ended.

Tracking artefacts:
- Per-phase plan documents declare scope.
- Phase-completion is gated by the plan's verification checkboxes
  reaching `[x]`.
- Slip / over-run goes into `docs/wiki/log.md` as a project memory
  rather than into a separate schedule-tracking system at this stage.

## 6. Configuration management

- **SCM**: git, hosted at <https://github.com/daniel12997/space-simulator>.
- **Branching**: `main` is the integration branch; per-phase work on
  `phase-N-<slug>` branches; per-feature work on similarly-slugged
  branches. Worktrees under `~/wt/space-simulator/<branch>` for
  isolation.
- **Pinning**: every external dependency is pinned by SHA (CPM via
  `cmake/CPM_VERSION` and per-package `GIT_TAG`; SOFA and CSPICE via
  `external/<lib>/PINNED_VERSION`). Pin-bumps are reviewable PRs.
- **Releases**: not yet defined; first tagged release coincides with
  Phase 6 completion (usable simulator). **TBD** — release process
  document at that time.

## 7. Risk management

Top-level risks live in `docs/00-design-overview.md` §"Risks and open
questions" and are reviewed at each phase boundary. New risks
surfaced during work are appended to that section in the same edit
that introduces them; resolved risks are removed and the resolution
is noted in `docs/wiki/log.md`.

## 8. Tooling

| Tool                | Use                                | Pinned in            |
| ------------------- | ---------------------------------- | -------------------- |
| CMake ≥ 3.25        | Build system                       | top-level CMakeLists |
| Ninja               | Generator                          | CI install step      |
| gcc-13, clang-17    | C/C++ compilers (Linux)            | CI matrix + ADR-006  |
| GoogleTest 1.15.2   | Test framework                     | CPM in dependencies  |
| Eigen 3.4.0         | Linalg                             | CPM in dependencies  |
| clang-format-17     | Formatter                          | `.clang-format`      |
| clang-tidy-17       | Static analysis                    | `.clang-tidy`        |
| GitHub Actions      | CI                                 | `.github/workflows/` |
| gcov / lcov         | Coverage (Phase 1 followup)        | (TBD)                |

## 9. Standards adherence

Apsis claims compliance with the **NPR 7150.2D Class D** practice
envelope (mandatory items per Class D, table in
[`decisions/013-class-d-software-classification`](../wiki/decisions/013-class-d-software-classification.md)). Apsis explicitly does **not**
claim compliance with NASA-STD-8739.8 Class A/B / DO-178C / MISRA C++ /
JPL Power of 10 — see ADR-013's "Alternatives considered" for why.

Coding standards: project ADRs are the load-bearing internal standard
(vocabulary discipline, frame / time-scale tagging, VE contract,
strategy-interface threshold). Style is enforced by clang-format /
clang-tidy. Generic C++ guidance defers to the C++ Core Guidelines.

## 10. Document control

This SMP is reviewed at each phase boundary. Substantive edits go
through PR review with a `meta:` or `process:` commit prefix. The
`docs/wiki/log.md` records SMP edits the same way it records ADR
landings.

## 11. References

- NPR 7150.2D — NASA Software Engineering Requirements
- NASA-STD-8739.8B — NASA Software Assurance and Software Safety Standard
- [`docs/REQUIREMENTS.md`](../REQUIREMENTS.md) — authoritative requirements
- [`docs/00-design-overview.md`](../00-design-overview.md) — project design statement
- [`docs/process/software-development-plan.md`](software-development-plan.md) — companion development plan
- [`docs/process/software-test-plan.md`](software-test-plan.md) — companion test plan
