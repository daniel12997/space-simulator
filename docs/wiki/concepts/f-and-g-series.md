---
type: concept
canonical_name: "f and g Series"
aliases: [f-and-g series, f and g functions, Lagrange coefficients]
created: 2026-05-04
updated: 2026-05-04
---

# f and g Series

A Taylor expansion of the two-body solution that expresses the position and velocity at time `t₀ + Δt` as a linear combination of the position and velocity at `t₀`:

```
r(t₀ + Δt) = f(Δt) · r₀ + g(Δt) · v₀
v(t₀ + Δt) = ḟ(Δt) · r₀ + ġ(Δt) · v₀
```

The scalar coefficients f, g, ḟ, ġ depend on Δt, on the central body's μ, and on the initial state through the magnitudes |r₀| and r₀ · v₀. They can be expanded as power series in Δt (the "f and g series" proper) or written in closed form using universal variables (Bate, Mueller & White; Vallado Ch. 2 — INDEX-only).

## Why it matters in Apsis

Two distinct uses:

**1. Analytical Keplerian propagation primitive (REQ-INT-004).** A pure two-body propagator is one f-and-g evaluation per requested time. No iteration, no integration. Used during cruise phases (architecture §2, design principle "Analytical where possible, numerical where necessary") and for any computation that needs an "if no perturbations existed, where would this object be" baseline.

**2. Multi-step integrator startup.** [[concepts/gauss-jackson-integration]] needs N+1 backpoints around epoch but the initial value problem only provides one. The standard technique ([[sources/berry-healy-2004-gauss-jackson]] §"Procedure" → "Startup", citing Vallado Ch. 8) is to evaluate the f-and-g series at the surrounding 8 epochs to seed initial guesses, then iterate the SECECE…CE corrector chain to refine to integrator-order accuracy. The two-body solution is *close enough* to the actual perturbed orbit over a single integration window that the iteration converges in 2-3 passes.

## Series form vs universal variables

Two equivalent representations:

- **Power series in Δt**: f = 1 - (μ/2|r₀|³) Δt² + …, g = Δt - (μ/6|r₀|³) Δt³ + …, etc. Truncating to fifth order in Δt is what [[sources/berry-healy-2004-gauss-jackson]] uses for GJ startup. Simple to evaluate, accurate for short Δt only.
- **Universal-variable closed form**: introduces a universal variable χ that handles elliptic, parabolic, and hyperbolic orbits uniformly. Solved via Newton iteration on Kepler's equation in universal form. Accurate across any conic section and any time-of-flight, at the cost of an inner Newton solve.

Apsis should use the power-series form for GJ startup (short Δt, no need for global accuracy) and the universal-variable form for the standalone Keplerian propagator (arbitrary Δt, need closed-form accuracy across all conic types).

## What it is *not*

f and g series is **not** an integrator. It assumes pure two-body motion. Any perturbation (J2, drag, third body, SRP) breaks the equation it solves. Used as a baseline / startup primitive only — never as a propagator for a real spacecraft past the cruise-only design boundary.
