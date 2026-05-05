---
type: decision
title: "Phantom-typed Time<Scale> and State<Frame> with bespoke tag types"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: []
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Phase 1 implementation pattern. Operationalises
[[decisions/003-tagged-time-scale-types]] (which decided *that* scale tags
exist and that mixing is a compile error) by deciding *how* the tagging is
implemented.

## Context

ADR-003 mandates that time-scale mixing is a compile error and that
conversions are explicit and SOFA-mediated. By analogy, every state /
position / velocity / attitude carries a frame tag (design overview:
"Frames are first-class"). The tagging mechanism is an implementation
decision: a tag-type strategy, a strong-typedef library, or a dimensional-
analysis library could all express the same constraint.

The pattern needs to:

- Make scale / frame mixing a compile-time error with readable diagnostics.
- Cost zero at runtime (representation identical to the underlying
  numeric type).
- Compose cleanly with Eigen ([[decisions/012-eigen-with-apsis-math-aliases]])
  and with template-based generic numerical code (the integrators).
- Not add a third-party dependency unless that dependency earns its
  weight elsewhere.

## Decision

Bespoke phantom tag types and `Time<Scale>` / `State<Frame>` class
templates.

```cpp
namespace apsis::time::tags {
struct TAI {};
struct TT {};
struct UTC {};
struct UT1 {};
struct TDB {};
}

namespace apsis::time {
template <class Scale> class Time { /* two-component (jd1, jd2) */ };

template <class To, class From>
Time<To> convert(Time<From>);  // SOFA-mediated, declared per (To,From)
}

namespace apsis::frames::tags {
struct ICRF {}; struct GCRS {}; struct ITRS {};
struct J2000 {}; struct TEME {}; struct BodyFixed {};
}

namespace apsis::frames {
template <class Frame> class State { /* pos, vel */ };
}
```

Cross-scale arithmetic is forbidden via deleted overloads:
`Time<TAI>::operator+(Time<UTC>)` does not exist. Cross-frame state
mixing is forbidden the same way. The only path between scales / frames
is the explicit `convert<>` / `transform<>` function, which goes through
SOFA.

## Rationale

- Phantom types add zero runtime cost and zero dependency cost.
- The compile errors on mixing are readable: "no matching function for
  call to `operator+`(Time<TAI>, Time<UTC>)" is far clearer than
  whatever a units library produces with three layers of expression
  templates.
- Scale tags are project nouns, not generic units — TAI / TT / UT1
  carry physical meaning beyond "seconds" that a units library doesn't
  know how to express. Encoding them as bespoke tag types keeps the
  semantics localised in our code.
- Template specialisations of `convert<To, From>` give us a closed,
  enumerable conversion graph. Each pair has exactly one implementation,
  and adding a new scale is a deliberate, reviewable change.

## Alternatives considered

- **Mp-Units (C++23 candidate library).** Excellent for dimensional
  analysis. Rejected because the project doesn't need unit propagation
  — we never multiply seconds by metres expecting the compiler to
  figure out joules. Mp-Units would impose a heavy template apparatus
  for a guarantee we don't need.
- **Boost.Units.** Same as Mp-Units; older interface; pulls Boost.
- **`BOOST_STRONG_TYPEDEF`.** Single-header. Rejected for the same
  Boost-dep reason and because it doesn't compose with the
  two-component representation we need (a strong typedef of `double`
  gives us seconds-since-epoch, not the conditioned `(jd1, jd2)` we
  require).
- **Strong-typedef libraries (NamedType, fluentcpp).** Pull a small
  dependency for ergonomics we already get with templates. Net loss.
- **Just use `double` everywhere with naming conventions
  (`tt_seconds_since_epoch`).** Rejected; ADR-003 explicitly demands
  compile-time enforcement.

## Consequences

- Every public function that takes a time or state declares its scale or
  frame in the type, eliminating one whole class of bug at the type
  level.
- Generic code (e.g. integrators) is templated over `Frame` and operates
  on `State<Frame>` without knowing the concrete frame.
- `convert<>` is the only public function that bridges scales; missing
  conversions are link errors, not silent fallbacks.
- Compile-error tests are written as `static_assert` in `// expected-error`
  blocks (compile-fail tests, run via CMake's `try_compile` with
  `EXPECT_FAIL`).
- Frame transitions during a propagation arc must be explicit
  `transform<>` calls at named seams (atmosphere edge, sphere-of-
  influence crossings) per the design overview's "frame transitions
  happen at precision-rich boundaries" pattern.
