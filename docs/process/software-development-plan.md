# Apsis Software Development Plan (SDP)

> **Class D** under NPR 7150.2D. Companion to the
> [Software Management Plan](software-management-plan.md) and the
> [Software Test Plan](software-test-plan.md). Where the SMP describes
> *how the project is run* and the STP describes *how testing
> happens*, this document describes *how development happens*: the
> environment, the standards, the workflow, and the build / release
> infrastructure.
>
> Skeleton document — sections marked **TBD** are deferred to the
> triggering event named inline.

## 1. Development environment

| Component        | Linux                                   | Windows         |
| ---------------- | --------------------------------------- | --------------- |
| OS               | Ubuntu 24.04 (Noble) — CI runner        | (deferred — see issue #2) |
| Compilers        | gcc-13 + clang-17                       | (MSVC — deferred) |
| Build system     | CMake ≥ 3.25 + Ninja                    | same            |
| Sanitizers       | ASan + UBSan via clang-17 in dedicated CI job | same      |
| Test framework   | GoogleTest 1.15.2 (gtest + gmock)       | same            |
| Linalg           | Eigen 3.4.0 under `apsis::math` aliases | same            |
| Domain libs      | IAU SOFA 2023-10-11, JPL CSPICE         | same / deferred |

Local development is expected to work on the same matrix the CI
exercises. Per-developer environments are not standardised beyond
this; project tooling configures itself via CMake on the machine it
runs on.

## 2. Coding standards

**Project-internal standards (ADR-derived):**

- **Frames are first-class.** Every state, position, velocity,
  attitude is `State<Frame>`-tagged at the type level
  (ADR-010). Mixing is a compile error.
- **Time scales are first-class.** Every `Time<Scale>` is type-tagged
  (ADR-003 / ADR-010). Conversions are SOFA-mediated and explicit.
- **VE contract on every force model.** `IForceModel` exposes
  `acceleration` and `partials_dadx`; admission to the seam requires
  passing the parameterised conformance test (ADR-002 / ADR-009).
- **Strategy interfaces only at real seams.** Strategy interface
  exists when ≥ 2 adapters are real (one-adapter "seams" are just
  indirection) — design overview "Patterns to follow" §"Deep modules
  with internal seams".
- **Vocabulary discipline.** Module / Interface / Depth / Seam /
  Adapter / Leverage / Locality. Never "component" / "service" /
  "boundary".
- **Conformance via the interface, not past it.** One parameterised
  conformance test covers all implementations of a seam.
- **Determinism is non-negotiable for Monte Carlo.** No global RNG,
  no within-trial parallelism, no hash-iteration-order dependencies.

**Style enforcement:**

- `clang-format-17` with the project's `.clang-format` (LLVM-derived,
  100-column, 2-space indent). Enforced as `--Werror` in CI.
- `clang-tidy-17` with the project's `.clang-tidy` (`bugprone-*`,
  `performance-*`, `modernize-*`, `cppcoreguidelines-pro-*`,
  `readability-identifier-naming`). Enforced with `WarningsAsErrors:
  '*'` in CI.
- Compiler warnings: `-Wall -Wextra -Wpedantic -Werror -Wshadow
  -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual
  -Wconversion -Wsign-conversion -Wdouble-promotion -Wformat=2`.
  Zero-warning policy; CI fails on any new warning.
- Vendored upstream code (SOFA, CSPICE) is exempt — see ADR-008 and
  the wrapper `CMakeLists.txt` files.

**Generic guidance:** the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/) cover anything the project ADRs don't speak to.

## 3. Source-tree layout

```
include/apsis/        first-party headers (PUBLIC)
src/                  first-party implementations
tests/{unit,conformance,regression,smoke}/
external/{sofa,cspice}/    vendored upstream + hand-written CMakeLists
data/                 vendored small reference data (Tier 1, ADR-011)
cmake/                helper modules + vendored CPM
docs/                 design / process / wiki content
  ├── 00-design-overview.md  (authoritative design statement)
  ├── REQUIREMENTS.md        (authoritative requirements)
  ├── structure.md           (vertical-slice phasing)
  ├── phase-N-plan.md        (per-phase implementation plans)
  ├── process/               (this document; SMP; STP)
  └── wiki/                  (research, decisions, components, concepts, sources)
tools/lint/           project-internal lint scaffolding (CSPICE seam, REQ traceability)
.github/workflows/    CI YAML
```

## 4. Branch / PR / review process

- `main` is the integration branch and is protected (PR-only,
  CI-required). **TBD** — set up GitHub branch protection rules
  before the first external contributor PR.
- Feature branches: `phase-N-<slug>` for phase work; `<topic>-<slug>`
  for cross-cutting work (e.g. this document landed on
  `docs-class-d-classification`).
- Worktrees: `~/wt/space-simulator/<branch>` keeps each branch
  isolated from the main checkout.
- PRs require:
  - Green CI matrix.
  - At least one review approval.
  - Updated plan-document checkboxes when the PR closes plan items.
  - Linked tracking issues when applicable.
- Commit prefixes per CLAUDE.md "Commit Message Prefixes".

## 5. Configuration management

- All upstream dependencies pinned by SHA (CPM via `GIT_TAG` to
  release tags; SOFA / CSPICE by tarball SHA-256 in `PINNED_VERSION`).
- Pin bumps are reviewable PRs; bumping a pinned version is never a
  silent edit.
- Vendored data (Tier 1, ADR-011) checksummed in `data/SHA256SUMS` and
  verified by a test (`tests/unit/data/integrity_test.cc`, Phase 1).
- Build artefacts (`build/`, `external/_cache/`, vendor `src/` /
  `include/`, fetched data) are gitignored.

## 6. Build infrastructure

- Top-level `CMakeLists.txt` orchestrates `external/sofa`,
  `external/cspice`, `src/`, `tests/` subdirectories.
- Compiler options applied via `cmake/apsis_compile_options.cmake`
  (first-party only — vendored upstreams override this).
- Dependencies fetched via `cmake/apsis_dependencies.cmake` (CPM).
- External-tarball fetch via `cmake/fetch_external.cmake`
  (configure-time, SHA-verified).
- Large reference data fetch via `cmake/fetch_large_data.cmake` (gated
  by `APSIS_FETCH_LARGE_DATA=ON`).
- CI builds: gcc-13, clang-17, clang-17 + ASan + UBSan. Each runs
  configure / build / ctest. Format / tidy / CSPICE-seam lint run on
  the appropriate single Linux job each.

## 7. Documentation requirements

- Every architectural decision lands as an ADR (`docs/wiki/decisions/`).
- Every component-level non-obvious knowledge lands as a component
  page (`docs/wiki/components/` — populated as components materialise).
- Every external research source consulted lands as a source page
  (`docs/wiki/sources/`) per the wiki citation policy.
- Per-phase plans live as `docs/phase-N-plan.md` and track
  verification progress via checkboxes.
- The wiki itself documents the wiki conventions in `docs/meta/`.

## 8. Release process

**TBD** — defined when Phase 6 completes (first usable release).
Skeleton expectation: tagged versions follow `vMAJOR.MINOR.PATCH`;
release notes generated from the merged PR titles since the previous
tag; release artefacts include the source tree plus, optionally, a
binary distribution of the C++ static library and the Python wheel.

## 9. References

- [`software-management-plan.md`](software-management-plan.md) — companion management plan
- [`software-test-plan.md`](software-test-plan.md) — companion test plan
- [`decisions/013-class-d-software-classification`](../wiki/decisions/013-class-d-software-classification.md) — classification justification
- [CLAUDE.md](../../CLAUDE.md) — project-wide conventions cheat sheet
- [`00-design-overview.md`](../00-design-overview.md) — project design statement
