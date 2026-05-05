---
type: decision
title: "Vendor IAU SOFA and JPL CSPICE C source under external/"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: [iau-sofa-2023-software-collection-c, naif-spice-required-reading, sofa-2023-time-scale-cookbook, sofa-2023-earth-attitude-cookbook]
components: []
requirements: []
---

## Status

**Accepted** 2026-05-05. Phase 0 prerequisite. Closes the "how do we obtain
SOFA and CSPICE" gap left by [[decisions/001-use-ceo-based-icrs-to-itrs]]
and [[decisions/003-tagged-time-scale-types]] both naming SOFA as the
conversion authority.

## Context

The design overview names SOFA as the only authority for time-scale and
IAU-frame conversions and SPICE as the only authority for ephemerides.
Both ship as C source distributions:

- **IAU SOFA** ([[sources/iau-sofa-2023-software-collection-c]]) — annual C
  release; about 250 source files. No upstream CMake. Distributed as a
  signed tarball from iausofa.org with bespoke licensing terms.
- **JPL CSPICE** ([[sources/naif-spice-required-reading]]) — large C source
  tree (~1500 files). No upstream CMake. Distributed as platform-specific
  tarballs from naif.jpl.nasa.gov. Permissive license; thread-unsafe.

We need a strategy that (a) reproduces builds bit-identically across
machines, (b) doesn't depend on platform package managers (distros vary
wildly on whether either is available), (c) integrates cleanly with the
CMake-based main build (see [[decisions/006-cmake-cpm-build-system]]).

## Decision

- **Both libraries are vendored** under `external/sofa/` and
  `external/cspice/` respectively.
- **Each carries a hand-written `CMakeLists.txt`** that the main build
  consumes via `add_subdirectory()`. Each glob-builds the upstream `src/`
  into a static archive (`apsis_sofa`, `apsis_cspice`) and exposes the
  upstream `include/` as an `INTERFACE` include directory.
- **Upstream sources are not committed binary**; instead, each vendor
  directory carries a `PINNED_VERSION` file with the upstream URL,
  release date, and SHA-256. A `cmake/fetch_external.cmake` script
  downloads and SHA-verifies the tarballs the first time a developer
  configures the build, extracting into the vendored layout.
- **CSPICE thread-unsafety is contained at the `IEphemeris` seam** by a
  process-wide `std::mutex`; no CSPICE call may originate outside that
  seam. Enforced by clang-tidy's `bugprone-*` suite via a custom matcher
  that flags `*_c(` calls outside `src/ephemeris/`.

## Rationale

- Vendoring with verified-fetch beats committing binaries (repo bloat) or
  relying on system packages (distro version drift; SOFA in particular is
  rarely packaged).
- Hand-written `CMakeLists.txt` per upstream is small (~20 lines each) and
  totally tractable. The alternative — patching upstream to add CMake
  support — risks divergence at every annual SOFA release.
- Pinning by upstream-tarball SHA preserves byte-level reproducibility:
  if iausofa.org reissues the 2023 release with a doc fix, our build does
  not silently change.
- Containing CSPICE thread-unsafety at one seam is the only viable
  approach given the design overview's risk note; any thread-unsafety
  leak would corrupt Monte Carlo trial determinism (Phase 5).

## Alternatives considered

- **Distro packages (`apt install libsofa-dev`).** SOFA is not packaged on
  most distros; CSPICE is packaged on a few but at varying versions.
  Rejected; reproducibility lost.
- **CMake `FetchContent` direct from upstream.** Would work, but iausofa
  distributes by signed tarball not git, so `FetchContent_Declare(URL …)`
  is the path — that's what the fetch script ends up being. Same outcome,
  this just makes the fetch step explicit and inspectable.
- **Drop SOFA, write our own IAU 2006 / 2010 implementations.** Rejected;
  the design overview pins SOFA as the only conversion authority, and
  reimplementing nutation tables to SOFA-equivalent precision is months
  of work for zero validation gain.
- **Drop CSPICE, write our own ephemeris reader.** SPK file format is
  documented but the required-reading edge cases (segment types, Type 2
  Chebyshev, frame chains) make this a multi-month effort that would
  miss SPICE features used by mission analysts.
- **Use a third-party CMake wrapper of CSPICE (e.g. `cspice-cmake`).**
  Considered; rejected because it adds a dependency we then track,
  whereas a 20-line `CMakeLists.txt` we own does not.

## Consequences

- Build dependency on network access at first configure (or a populated
  `external/_cache/` directory). CI provides a populated cache image.
- Upstream version bumps are one-line PRs (update SHA + URL in
  `PINNED_VERSION`).
- CSPICE thread-unsafety enforcement adds a `tools/lint/cspice_seam.py`
  CI step that scans for `*_c(` outside the seam directory.
- License notices for both libraries land in `THIRD_PARTY_LICENSES.md`
  alongside the project Apache-2.0 license.
