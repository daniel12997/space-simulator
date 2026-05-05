---
type: source
title: "Optimal Impulsive Collision Avoidance in Low Earth Orbit"
raw_path: docs/raw/papers/bombardelli-2015-collision-avoidance.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Bombardelli, Claudio; Hernando-Ayuso, Javier]
publication_date: 2015
venue: "Journal of Guidance, Control, and Dynamics 38(2):217-225"
doi: 10.2514/1.G000742
---

# Bombardelli & Hernando-Ayuso (2015) — Optimal impulsive collision avoidance

A **fully analytical formulation** for optimal impulsive collision-avoidance maneuvers in LEO. Hinges on a **linear relation between applied Δv and b-plane displacement** that lets the optimization collapse to an **eigenvalue problem coupled with a simple nonlinear algebraic equation**. Two objective functions handled: maximum miss-distance (max-Δr) and minimum collision probability (min-Pc). Provides closed-form Δv direction with reduced computational cost vs fully numerical optimization.

## Setup

Two short-term encounter (linear-motion) bodies in LEO; standard Foster-method assumptions hold ([[sources/foster-estes-1992-jsc-25898-pc]]). Pc is computed in the b-plane via the Chan series (Eq. 4) — equivalent to but more numerically convenient than the Foster integral.

The b-plane reference frame `<ξ, η, ζ>` is centered on `S₂`, with `η` along relative velocity (out of plane) and `(ξ, ζ)` spanning the encounter plane. Joint covariance projects onto the `(ξ, ζ)` subspace as the 2×2 matrix `C_ξζ` (Eq. 7).

## The key linear relation (Eq. 9)

After expressing the b-plane displacement induced by an impulsive Δv applied at angular distance Δθ before TCA:

```
r = R K D Δv = M Δv          [b-plane disp linear in Δv]
```

with `R` a rotation, `K` a geometric/orbital-shape matrix, and `D` containing the dimensionless functions `d_rr, d_rθ, d_θr, d_θθ, d_w/h` of `(e₀, θ_c, Δθ)` (Lyapunov-equation-style derivatives).

This linearity is **the central trick**: the maneuver-optimization problem becomes a quadratic form in `Δv`, which under a budget constraint `|Δv| ≤ Δv₀` reduces to a generalized eigenvalue problem.

## Two optimal-maneuver formulations

### Maximum miss-distance (§V)

Maximize `J_r = ξ² + ζ²` subject to `|Δv| ≤ Δv₀`:

```
J_r = Δvᵀ (Mᵀ Q M) Δv,    Q = diag(1, 0, 1)
```

Lagrange multipliers → eigenvalue problem; optimal Δv is the eigenvector of `A = MᵀQM` corresponding to the largest eigenvalue. Maximum miss distance is `Δr_max = √λ₁ Δv₀`.

For direct-hit encounters (initial miss distance = 0), the optimal Δv direction is independent of magnitude (both signs work). For non-direct-hit, sign matters.

### Minimum collision probability (§VI)

Maximize the squared **Mahalanobis distance** in the b-plane (≈ minimum Pc):

```
J_P = (ξ/σ_ξ)² + (ζ/σ_ζ)² - 2ρ_ξζ ξζ/(σ_ξ σ_ζ)
    = Δvᵀ (Mᵀ Q* M) Δv
```

with `Q*` the inverse of the b-plane covariance. Same eigenvalue structure as max-miss, just weighted by `Q*` instead of identity. Solution proceeds analogously.

These two optimization criteria can give **different optimal directions** — a maneuver that maximizes geometric miss can underperform Pc-minimization when the covariance ellipse is highly anisotropic. The paper explicitly compares both on a 2009 Iridium-Cosmos case and a Rapideye4 / nonoperational UoSat2 case.

## Apsis relevance

- **REQ-CAT-008** (collision-avoidance maneuver planning) — this paper is the canonical analytical algorithm for optimal impulsive avoidance Δv. Apsis should implement both max-Δr and min-Pc formulations and let the user pick.
- **Subsystems §6** (CAM planning): specifies "linearized-dynamics-based optimal Δv computation" — this paper *is* that algorithm.
- **REQ-INT-009** (impulsive maneuvers) — the optimal Δv produced here is the input to the impulsive-burn integrator interface.
- **REQ-MC-005** validation — Apsis can compare analytical (this paper) vs full numerical optimization across a Monte Carlo test set; this paper's representative scenarios in §VII are reasonable starting points.

## Limits

- **Short-term encounter assumption** — same as Foster method; breaks for long-duration encounters (typically GEO co-orbital) where relative motion isn't well-approximated as linear during the encounter.
- **Impulsive burn** — finite-burn corrections (gravity loss, finite duration) are not modeled.
- **Single-burn** — multi-burn collision-avoidance sequences require numerical optimization.
- **Two-body Keplerian dynamics** in the linearization (with J₂ as a perturbing-acceleration check in §VII); for very tight CAM scheduling at low altitudes, full force model may matter.

## Cross-references

- [[sources/foster-estes-1992-jsc-25898-pc]] — the Pc formulation that defines `J_P`.
- [[sources/newman-2022-cara-best-practices]] — operational CAM decision context.
