---
type: decision
title: "GoogleTest as the project test framework"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: []
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Phase 0 prerequisite.

## Context

The project needs a test framework for unit tests, parameterised conformance
tests (one test class evaluated against every adapter behind a seam — used
heavily for `IIntegrator`, `IBroadPhase`, `IForceModel`), and regression
tests with tolerance comparisons.

## Decision

**GoogleTest (gtest + gmock).** Fetched via CPM (see [[decisions/006-cmake-cpm-build-system]])
pinned to `v1.15.2`. Used for unit, conformance, and regression suites
labelled by CTest label (`unit`, `conformance`, `regression`).

## Rationale

- `TYPED_TEST_SUITE` and `TEST_P` map directly onto the architecture's
  parameterised conformance tests. One conformance test parameterised over
  every adapter is exactly the pattern the design overview prescribes.
- gmock provides the mock infrastructure for sensor / force-model fakes
  used in GNC unit tests (Phase 4) without pulling another library.
- Largest mindshare; new contributors are likely to know it already.
- Native `EXPECT_NEAR` and floating-point matchers fit numerical-tolerance
  testing without bespoke matchers.

## Alternatives considered

- **Catch2 v3.** Cleaner DSL, native `BENCHMARK` macro. Rejected per
  user preference; gmock equivalence in Catch2 (trompeloeil etc.) is a
  separate dependency.
- **doctest.** Header-only and very fast to compile. Rejected for the same
  mock-library reason; smaller community.

## Consequences

- Tests live under `tests/{unit,conformance,regression,smoke}/` mirroring
  the source-tree shape of what they cover.
- Conformance tests use `INSTANTIATE_TEST_SUITE_P` to enumerate adapters.
- Performance benchmarks (Phase 7 hardening) will use a separate
  micro-benchmark library (likely Google Benchmark) — out of scope for
  Phase 1; flagged here for continuity.
- `ctest -L conformance` is the gate test for "does this new adapter
  honour the seam contract".
