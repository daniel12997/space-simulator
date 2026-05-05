---
type: decision
title: "CMake plus CPM.cmake for build system and upstream dependency management"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: []
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Phase 0 (project bootstrap) prerequisite.

## Context

Greenfield C++17 project. Need a build system that interops cleanly with the
mandatory upstream stack: SOFA (vendored C source, no upstream CMake), CSPICE
(vendored C source, no upstream CMake), Eigen (header-only, ships CMake
config), GoogleTest (CMake), and Pinocchio (CMake) when Phase 3 lands.

A dependency strategy is also required: pin upstream versions for
reproducibility; allow CI to fetch them on a clean machine; avoid asking
contributors to install a separate package manager beyond what their compiler
already provides.

## Decision

- **Build system: CMake ≥ 3.25.** Modern target-property model, `FetchContent`
  works deterministically, every upstream we need has CMake support or yields
  to a thin wrapper.
- **Dependency fetching: CPM.cmake** (vendored as `cmake/CPM.cmake`, pinned
  by SHA). Single-header CMake module that wraps `FetchContent` with version
  pinning, source caching, and a `CPMAddPackage` interface. No tool to
  install. Deterministic across machines.
- **Generator: Ninja** preferred; Make and MSBuild remain supported via the
  standard CMake generator selection.

## Rationale

- CMake is the only realistic option for a project with this dependency mix —
  every alternative (Bazel, Meson) requires more wrapping for SOFA / CSPICE
  / Pinocchio than CMake does, and the Pinocchio CMake export is the
  reference path used by every other consumer.
- CPM avoids the "install a package manager first" friction of vcpkg / Conan
  while still pinning by SHA. New contributors run `cmake -S . -B build` and
  the deps fetch on first configure into a content-addressed cache shared
  across builds.
- Pinning by SHA (not tag) makes builds reproducible even if upstream
  retags or force-pushes.

## Alternatives considered

- **vcpkg manifest mode.** Better binary caching across CI; less heavy on
  first-build time. Rejected because contributors must install vcpkg
  separately and bootstrap it per-platform; adds a tool that adds nothing
  CPM doesn't, for our dependency size.
- **Conan 2.** More featureful (binary caches, profiles); higher ceremony
  (recipe maintenance, separate CLI). Rejected for the same friction reason.
- **Git submodules.** No version pinning beyond a SHA, manual update flow,
  awkward for shallow clones in CI. Strictly worse than CPM for our case.
- **Meson, Bazel.** Both viable in isolation; both require us to wrap
  Pinocchio's CMake build system, doubling the integration cost.

## Consequences

- Repo carries `cmake/CPM.cmake` as a vendored single file (~1 kLOC).
  Pinned version recorded in `cmake/CPM_VERSION`.
- Top-level `CMakeLists.txt` exposes options `APSIS_BUILD_TESTS` (default
  ON), `APSIS_FETCH_LARGE_DATA` (default OFF, see ADR-011),
  `APSIS_ENABLE_SANITIZERS` (default OFF in Release).
- CI uses CMake's `ctest -L <label>` to select test subsets (`unit`,
  `conformance`, `regression`).
- All third-party CMake fetches funnelled through one
  `cmake/apsis_dependencies.cmake` so pin bumps are one-file changes.
