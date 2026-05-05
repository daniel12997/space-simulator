---
type: source
title: "A Parametric Analysis of Orbital Debris Collision Probability and Maneuver Rate for Space Vehicles"
raw_path: docs/raw/papers/foster-estes-1992-jsc-25898-pc.pdf
source_type: technical-report
reliability: authoritative
ingested: 2026-05-04
authors: [Foster, James Lee Jr.; Estes, Herbert S.]
publication_date: 1992-08
venue: "NASA Johnson Space Center JSC-25898"
---

# Foster & Estes (1992) — JSC-25898 collision probability and maneuver rate

The seminal NASA JSC report defining the **"Foster method" 2D analytic collision-probability (Pc) algorithm** that became the operational standard for ISS and Space Shuttle conjunction assessment, and remains the canonical Pc method in NASA CARA today (see [[sources/newman-2022-cara-best-practices]] for current practice). The report frames Pc as a *maneuver decision criterion* and derives the analytic expression for the integrated probability that two extended objects come within their combined hard-body radius during a close approach, given relative-position covariance.

## What the report does (Contents)

§2 lays out the parameters: state-vector uncertainty for the protected vehicle (§2.1) and tracked debris (§2.2), the Pc maneuver-decision threshold (§2.3), maneuver-frequency analysis (§2.4), and the debris-flux model (§2.5).

§3 develops three computational tools:
- **Monte Carlo Collision Probability Code** (§3.3.1, App. B) — sample relative state from the joint covariance, count close approaches, divide by trial count. Reference truth.
- **Numeric/Analytic 2D Pc** (§3.3.2, App. C) — the **Foster analytic method**: project the joint relative-position covariance onto the **encounter B-plane** (perpendicular to relative velocity at TCA), reduce to a 2D Gaussian integral over a circle of radius equal to combined hard-body radius. Closed-form via numerical integration of a 2D normal pdf.
- **Maneuver Rate Determination Tool** (§3.3.3, App. D) — given a Pc threshold and a flux model, predict expected per-orbit maneuver rate.

## The Foster 2D analytic Pc method

Key assumptions (made explicit by App. C):

1. **Linear relative motion** through the encounter — both objects travel approximately in straight lines during the brief window when they could collide. Excellent approximation for hyperbolic encounters with short conjunction durations (typical of LEO, where relative speeds are 7-15 km/s).
2. **Position covariances are constant** across the encounter window.
3. **Velocity uncertainty** doesn't enter directly — it's absorbed by the linear-motion assumption choosing TCA from the nominal trajectories.
4. **Combined position covariance** at TCA: `C = C_primary + C_secondary` (independence assumed).
5. The two extended bodies are treated as **spheres of combined hard-body radius** `R_HBR = R_p + R_s`.

The integral collapses to a 2D Gaussian over a disc of radius `R_HBR` in the B-plane. Foster's algorithm computes this via series expansion or quadrature.

## Apsis relevance

- **REQ-CAT-005** (conjunction screening with Pc computation) — Foster's 2D analytic method is the canonical algorithm Apsis must implement. Subsystems §6 names "Foster Pc method" specifically.
- **REQ-CAT-008** (maneuver decision criteria) — the Pc threshold approach (typically `Pc > 1e-4`) for triggering maneuvers traces to this report's framing.
- **CDM ingestion** ([[sources/newman-2022-cara-best-practices|CARA best practices]] + CCSDS 508 in `docs/raw/specs/`) — every conjunction data message includes the inputs (relative state, joint covariance, HBR) Foster's method consumes.
- **REQ-MC-005** (Monte Carlo cross-validation) — Apsis can validate its analytic Pc against the Monte Carlo reference described in App. B for catch-all confidence.

## Limits of the Foster method

The 2D linear-motion assumption breaks for:

- **Long-duration encounters** (e.g., GEO co-orbital approaches) where the linear-motion assumption fails; need 3D non-linear methods like Alfano, Patera, or numerical Monte Carlo.
- **Highly elliptical or low-relative-velocity encounters** where the encounter geometry isn't well-described by a single B-plane.

For these, Apsis should fall back to Monte Carlo or 3D methods. [[sources/newman-2022-cara-best-practices]] discusses CARA's fallback hierarchy.

## Cross-references

- [[sources/newman-2022-cara-best-practices]] — current NASA CARA operational practice built on this method.
- [[sources/bombardelli-2015-collision-avoidance]] — analytical maneuver design that consumes Pc as decision input.
- [[sources/hoots-roehrich-1980-spacetrack-report-3]] (in `docs/raw/specs/`) — TLE format that feeds the screening pipeline.
- CCSDS 508.0-B-1 CDM (in `docs/raw/specs/`) — the conjunction data message format that delivers Foster-method inputs.
