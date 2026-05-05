---
type: decision
title: "Eigen 3.4 underneath, apsis::math type aliases at the public API"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: [carpentier-2019-pinocchio]
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Phase 1 implementation; relevant project-wide.

## Context

The project needs a linear-algebra library covering small fixed-size
shapes (3-vectors for position / velocity, 3×3 rotations, 4-component
quaternions, 6×6 state-transition matrices, 7×7 attitude STMs), plus
modest dynamic-size operations (covariance matrices for Phase 4 GNC,
Cholesky for Kalman update, eigendecomposition for whitening).

Phase 3 brings Pinocchio ([[sources/carpentier-2019-pinocchio]],
[[concepts/pinocchio-library]]) for multi-body dynamics. Pinocchio is
Eigen-based at the API level — every public function takes
`Eigen::Matrix*` types directly. This is true of every flight-dynamics-
grade MBD library (RBDL, Drake, iDynTree, DART) — Eigen is structural
to the entire ecosystem we draw from.

## Decision

- **Eigen 3.4** is the linalg library, pulled by CPM
  ([[decisions/006-cmake-cpm-build-system]]).
- **Public `apsis::math` API uses thin `using` aliases over Eigen
  templates** so the codebase reads in domain types, not Eigen
  templates:

```cpp
namespace apsis::math {
using Vec3  = Eigen::Matrix<double, 3, 1>;
using Vec6  = Eigen::Matrix<double, 6, 1>;
using Mat3  = Eigen::Matrix<double, 3, 3>;
using Mat6  = Eigen::Matrix<double, 6, 6>;
using Mat36 = Eigen::Matrix<double, 3, 6>;
using Mat66 = Mat6;
using Quat  = Eigen::Quaterniond;
using VecX  = Eigen::VectorXd;
using MatX  = Eigen::MatrixXd;
}
```

- Pinocchio interop is direct — `apsis::math::Vec3` *is* an Eigen type,
  no conversion needed at the Phase 3 MBD seam.
- If specific aliases later prove painful (compile-time errors, IDE
  hover noise), upgrading any single alias to a wrapper struct with
  `.eigen()` accessor is a localised change that doesn't break public
  API users — call sites continue to write `apsis::math::Vec3`.

## Rationale

- Pinocchio is Eigen-based, locking Eigen into the dependency tree from
  Phase 3 onward. Choosing any other primary linalg library forces a
  two-library design with conversions at every MBD-orbital boundary —
  free bugs, no upside.
- Eigen handles every shape Apsis needs at the speed compilers can
  optimise to. Fixed-size 3-vec and 3×3 ops compile down to the same
  SIMD as hand-rolled SSE intrinsics. Dynamic-size covariance work uses
  `LDLT`, `LLT`, and `SelfAdjointEigenSolver` from Eigen and matches
  LAPACK quality at sane sizes.
- The `using` alias layer is essentially free — zero runtime cost, no
  ABI surface beyond Eigen itself, and the surface area of Eigen
  exposed to callers is exactly the operations they need rather than
  every Eigen feature.
- This is the pattern Orekit-Hipparchus and most production
  astrodynamics codebases that use Eigen settle on; it is a known-good
  point in the design space.

## Alternatives considered

- **Blaze for orbital code, Eigen at the MBD seam.** Blaze is comparable
  to Eigen on small fixed-size workloads, marginally faster on some
  benchmarks. Rejected because (a) we still ship Eigen for Pinocchio,
  so we'd be dragging two libraries; (b) every orbital ↔ MBD boundary
  requires conversion; (c) Blaze's smaller community means fewer eyes
  on subtle correctness issues; (d) no quaternion type at Eigen's
  quality.
- **Hand-rolled small-fixed-size math (`Vec3`, `Mat3`, `Mat6` from
  scratch).** Tempting for compile times and error legibility. Rejected
  because the moment we need eigendecomposition or Cholesky on the
  Phase 4 covariance, we'd reimplement Eigen badly. Two type systems,
  conversion at every MBD seam.
- **Armadillo.** LAPACK-wrapping; geared to dynamic-size matrices and
  MATLAB-like ergonomics. Rejected; weak fixed-size optimisation, where
  most Apsis math lives.
- **xtensor.** NumPy-like n-d arrays. Not optimised for the
  small-fixed-size kinematic math that dominates orbital code.
  Rejected.
- **Drop Pinocchio for a custom Featherstone implementation.** Would
  open the linalg field but is a months-long architectural change for
  a problem the design overview already locked. Out of scope.

## Consequences

- Public APIs use `apsis::math::*` aliases consistently. Internal code
  may use `Eigen::*` directly when integrating with Eigen idioms.
- `apsis::math::types.h` is a single header that owns the alias
  surface, easy to audit and to evolve.
- Pinocchio's compile-time hit lands when Phase 3 begins; Phase 1's
  compile times stay small because only Eigen's small-fixed-size
  pieces are included.
- A future migration to wrapper structs (if compile-time errors
  motivate it) is per-alias and transparent to callers.
- Eigen's expression-template aliasing footguns (`a = a * b` for
  3×3 self-multiplication) are a real risk; mitigated by clang-tidy
  rule and a project lint check that flags self-assignment patterns
  on `Mat*` types.
