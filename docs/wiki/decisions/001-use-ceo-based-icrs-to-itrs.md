---
type: decision
title: "Use CEO-based pipeline (X, Y, s, ERA) for ICRS↔ITRS transformations"
status: proposed
decided: null
supersedes: []
superseded_by: null
sources: [capitaine-2003-iau2000-precession-p03]
components: []
requirements: [REQ-TIME-006, REQ-TIME-008]
---

## Status

**Proposed.** Awaiting human sign-off. Surfaced during ingest of [[sources/capitaine-2003-iau2000-precession-p03]] which observed that Apsis's architecture and subsystems documents specify "IAU 2006/2000A precession-nutation" without choosing between the two equivalent transformation pipelines that the IAU framework offers.

## Context

The IAU 2000/2006 framework provides **two mathematically equivalent** ways to rotate from the inertial GCRS to the terrestrial ITRS, both implemented in SOFA ([[sources/capitaine-2003-iau2000-precession-p03]] §2):

1. **Equinox-based (classical)** — sequential rotations: frame bias **B**, precession **P**(ζ_A, z_A, θ_A), nutation **N**(Δψ, Δε, ε_A), Earth rotation via **GMST** plus the equation-of-equinoxes complementary terms, then polar motion. Backward-compatible with pre-2003 procedures and IAU 1976 software.
2. **CEO-based (modern)** — uses CIP coordinates X, Y in GCRS (which absorb bias + precession + nutation polynomial parts), the CEO-position quantity s, the [[concepts/earth-rotation-angle|Earth Rotation Angle]] θ (linear in UT1), and polar motion. Recommended by IAU 2000 Resolution B1.8 and IERS Conventions 2010 ch. 5.

REQ-TIME-006 mandates "right-handed body-fixed frames using IAU 2006/2000A precession-nutation" but does not specify which pipeline. REQ-TIME-008 requires bidirectional frame transforms; both pipelines satisfy this.

## Decision (proposed)

Apsis SHALL use the **CEO-based pipeline** as the canonical implementation of the GCRS↔ITRS rotation:

- Inputs: CIP X, Y from `iauXy06`, CEO position s from `iauS06`, ERA from `iauEra00`, polar motion x_p, y_p from IERS Bulletin A.
- Composed via `iauC2t06a` (or by composing `iauC2ixys` × R₃(ERA) × `iauPom00` manually for diagnostic transparency).
- The equinox-based pipeline is **not implemented** as a parallel path; if a future requirement demands GMST-based output (e.g. for legacy interoperability), GMST is computed as a derived quantity from ERA + precession via SOFA's `iauGmst06(uta, utb, tta, ttb)`.

## Rationale

- **Linearity of ERA in UT1.** The CEO-based path's time-varying input is a linear function of UT1. The equinox-based path's input (GMST) is a polynomial in *both* TT and UT1 because GMST absorbs precession. Linearity makes high-rate frame evaluation cheaper and avoids subtle bugs at TT/UT1 epoch boundaries. ([[sources/capitaine-2003-iau2000-precession-p03]] §2)
- **No equation of equinoxes.** The CEO-based path has no equation-of-equinoxes term and no special handling for the complementary terms of s. One fewer subtle conventional pitfall.
- **Recommended modern standard.** IAU 2000 Resolution B1.8, IAU 2006 Resolution B2, and IERS Conventions 2010 all preference the CEO-based formulation for new implementations. Pre-2003 software keeps the equinox path for backward compatibility; Apsis is new code with no legacy obligation.
- **SOFA support.** Both pipelines are first-class in SOFA. There is no implementation cost differential.
- **Apsis fits the use-case profile.** Apsis's principle "Hierarchy of inertial frames" (architecture §2) prefers compact transformations applied at frame boundaries. The CEO formulation absorbs bias + precession + nutation polynomial parts into two scalars (X, Y), which fits that style.

## Alternatives considered

**Equinox-based (classical) pipeline.** Rejected as canonical because: (a) GMST is non-linear in UT1, complicating high-rate evaluation; (b) the equation-of-equinoxes complementary terms are an additional source of subtle bugs; (c) no backward-compatibility obligation; (d) sidereal time can still be obtained when needed via `iauGmst06`.

**Implement both pipelines and let users choose.** Rejected for the v1 timeframe — having two parallel transformation paths multiplies test surface, doubles validation effort, and creates a category of bugs where the two paths disagree at the LSB. A single canonical path with sidereal time as a derived quantity is preferred. Could be revisited if a real interop requirement appears.

**Re-implement polynomials in-house rather than delegate to SOFA.** Out of scope of this decision; Apsis already commits to SOFA delegation in architecture §3 Foundation. SOFA implements both pipelines, so the language of this decision is "which pipeline we standardize on", not "which library we use".

## Consequences

- The frame-transformation API exposes ICRS↔ITRS, ICRS↔TIRS, and (intermediate) GCRS↔CIP-frame transforms parameterised by (CIP X, Y, s, ERA, x_p, y_p). GMST is a derived getter.
- Time inputs to frame transforms always include both TT and UT1 (TT for the CIP coordinates' polynomial parts, UT1 for ERA). The two-component time representation (REQ-TIME-002) keeps both at sub-ns precision.
- Frame bias does not appear as an explicit step in the public API — it is absorbed into X, Y and into SOFA's internal evaluation. [[concepts/frame-bias]] documents this for diagnostic transparency.
- IAU 2000A nutation is required (not just polynomial precession); the CIP X, Y full evaluation needs the periodic terms. SOFA's `iauXys06a` does this internally.
- Tests must cover at least one published reference epoch (IERS provides X, Y, s, ERA tables for benchmarking) to validate the SOFA-call composition matches an authoritative external value.

## Open items if accepted

- Pick the canonical SOFA composition routine: `iauC2t06a` (one call) vs explicit `iauC2ixys` × R₃(ERA) × `iauPom00` (three calls) for diagnostic transparency. Likely the latter for v1, with `iauC2t06a` as a sanity-check oracle in tests.
- Confirm with REQ-TIME-006/008 owners that "no equinox-based path" is acceptable. If a hard interop requirement exists (e.g. exporting STK-compatible state vectors that demand GMST), this decision changes scope to "default CEO; equinox available behind a flag".
