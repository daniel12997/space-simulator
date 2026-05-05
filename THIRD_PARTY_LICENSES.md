# Third-Party Licenses

Apsis incorporates the following third-party software. The full text of each
upstream license is reproduced in `LICENSE` of the respective vendored source
tree (after `cmake/fetch_external.cmake` has populated it) or in the upstream
repository for CPM-fetched dependencies.

| Component   | Version  | License                          | Source                                    |
|-------------|----------|----------------------------------|-------------------------------------------|
| Apsis       | 0.0.1    | Apache-2.0                       | this repository, `LICENSE`                |
| IAU SOFA    | 2023-10-11 (C release) | SOFA Software License (BSD-style, with attribution and no-modification-of-name clauses) | https://www.iausofa.org/2023-10-11c |
| JPL CSPICE  | N0067    | NAIF / Caltech open license (BSD-style; no commercial restriction, attribution required) | https://naif.jpl.nasa.gov/naif/toolkit_C.html |
| Eigen       | 3.4.0    | MPL-2.0 (with small portions LGPL/BSD; see Eigen `COPYING.*` files) | https://gitlab.com/libeigen/eigen |
| GoogleTest  | v1.15.2  | BSD-3-Clause                     | https://github.com/google/googletest      |
| CPM.cmake   | v0.42.2  | MIT                              | https://github.com/cpm-cmake/CPM.cmake    |

Notes:

- The SOFA license requires that derivative copies that change SOFA routines
  use a different name; Apsis links the upstream archive unmodified, so the
  attribution-only clauses apply.
- CSPICE redistribution requires the NAIF acknowledgement to appear in
  documentation; Apsis includes it here.
- Eigen's MPL-2.0 is file-level copyleft; static-linking against Eigen does
  not impose copyleft on Apsis sources.

This file is updated whenever an upstream is added, bumped, or removed. New
upstreams must list version, license name, and a stable upstream URL.
