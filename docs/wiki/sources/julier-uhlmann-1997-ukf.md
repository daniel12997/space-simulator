---
type: source
title: "A New Extension of the Kalman Filter to Nonlinear Systems"
raw_path: docs/raw/papers/julier-uhlmann-1997-ukf.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Julier, Simon J.; Uhlmann, Jeffrey K.]
publication_date: 1997
venue: "Proc. SPIE 3068, Signal Processing, Sensor Fusion, and Target Recognition VI (AeroSense '97)"
doi: 10.1117/12.280797
---

# Julier & Uhlmann (1997) — The original Unscented Kalman Filter paper

The seminal paper introducing the **Unscented Kalman Filter (UKF)** and the underlying **unscented transformation**. Argues that for nonlinear systems, *propagating selected sigma points through the true nonlinearity* yields better mean and covariance estimates than the EKF approach of *linearizing the nonlinearity and propagating the Gaussian analytically*. Introduces the **consistency requirement** for any nonlinear-Gaussian-approximation transformation, and shows the UKF satisfies it while EKF can violate it (causing filter divergence).

## The argument against EKF (§1, §2)

EKF in practice has two well-documented failure modes:

1. **Linearization fragility**: when local linearity is violated (e.g., by large state errors or strongly nonlinear dynamics), the EKF can become highly unstable and diverge.
2. **Jacobian implementation burden**: nontrivial Jacobian derivations introduce bugs and force model-specific code paths that defeat black-box reuse.

Both motivate looking for a nonlinear extension that **doesn't require linearization**.

## The unscented transformation (§2.2)

Given random variable `x` with mean `x̄` and covariance `P_xx`, and a nonlinear function `y = f(x)`, **calculate `ȳ` and `P_yy`** without analytically transforming the distribution.

The **consistency** requirement (Eq. 6):
```
P_yy - E[(y - ȳ)(y - ȳ)ᵀ] ≥ 0
```

If statistics are inconsistent, the KF will under-estimate covariance, weight measurements too heavily, and risk divergence. The UT is *consistent by construction*.

The unscented transform proceeds by:
1. Choose `2n + 1` symmetric sigma points around `x̄` placed at distance √n in covariance-weighted directions.
2. Pass each sigma point through `f(·)` directly.
3. Compute `ȳ` and `P_yy` as weighted sample mean and covariance of the transformed sigma points.

Performance is shown analytically to be **superior to EKF** and **comparable to the second-order Gauss filter** — but with EKF-class computational cost and no Jacobian/Hessian derivation.

## Properties (§2-3)

- **Same first-order accuracy as EKF** for linear systems (UKF reduces to KF for linear `f`).
- **Captures third-order Taylor terms exactly** for symmetric distributions, second-order for any.
- **Black-box compatible** — `f(·)` can be a non-differentiable function or external simulation.
- **Same computational order** as EKF (no exponential explosion as one might fear).

## Apsis relevance

- **REQ-GNC-005** (UKF as alternative orbit estimator) — this is the foundational reference. [[sources/wan-vandermerwe-2000-ukf]] is the modern restated version with scaled UT and parameter-estimation extensions; this paper is the *origin*.
- **REQ-GNC-003 fallback** — UKF for attitude (USQUE, [[sources/crassidis-2003-ukf-attitude]]) inherits the consistency property from this paper.
- **EKF divergence cases** — Apsis should monitor EKF consistency online (e.g., NIS / NEES tests on innovations); when EKF goes inconsistent, fall back to UKF for that segment.
- **Black-box dynamics** — for force models that aren't analytically differentiable (e.g., empirical-table-based atmosphere `ρ(t, h, lat, lon, F10.7, Ap)`), UKF avoids the awkward Jacobian-via-finite-differences step that EKF needs.

## Historical note

Julier & Uhlmann's original sigma-point set is symmetric with `2n + 1` points; later refinements (Wan & van der Merwe's scaled UT, the Square-Root UKF) build on this baseline. The 1997 paper is the conceptual milestone; modern implementations follow Wan-van der Merwe 2000.

## Cross-references

- [[concepts/unscented-kalman-filter]] — the algorithm.
- [[concepts/kalman-filter]] — predecessor framework.
- [[sources/wan-vandermerwe-2000-ukf]] — modern reformulation (scaled UT, augmented state, parameter estimation).
- [[sources/crassidis-2003-ukf-attitude]] — attitude-specific USQUE variant.
