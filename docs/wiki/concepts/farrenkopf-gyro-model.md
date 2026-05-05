---
type: concept
canonical_name: "Farrenkopf Gyro Model"
aliases: [Farrenkopf model, gyro noise model, ARW/RRW gyro model]
created: 2026-05-04
updated: 2026-05-04
---

# Farrenkopf Gyro Model

The canonical statistical model of rate-mode gyro / IMU noise used in essentially every spacecraft attitude estimator since 1982. Derived by Farrenkopf in two AIAA papers ([[sources/lefferts-1982-mekf]] refs [31] and [69]) and propagated through the field by [[sources/lefferts-1982-mekf]] §5 (Eqs 48-54), which made it the de facto standard alongside the [[concepts/mekf|MEKF]].

## The model

Rate measurement (per axis):

```
ω = ũ - b - n₁                  (Eq. 48)   measurement = true rate − bias − white noise
db/dt = n₂                       (Eq. 51)   bias drifts as integrated white noise
```

with `n₁` and `n₂` independent zero-mean Gaussian white-noise processes:

```
E[n₁(t) n₁ᵀ(t')] = Q₁ δ(t-t')
E[n₂(t) n₂ᵀ(t')] = Q₂ δ(t-t')
E[n₁(t) n₂ᵀ(t')] = 0
```

The bias `b` is part of the **state** to be estimated, jointly with attitude — not a noise source. The only "noise" in the conventional Kalman sense is the additive white noise `n₁` on the measurement and the white noise `n₂` driving the bias random walk.

## ARW and RRW

The two noise PSDs map directly onto the two parameters that gyro datasheets advertise:

| Symbol | Name | Units | Physical meaning |
|---|---|---|---|
| `√Q₁` | **Angle Random Walk (ARW)** | rad/√s (or deg/√hr) | Angular displacement from integrating measurement noise grows as `√(Q₁ · t)` |
| `√Q₂` | **Rate Random Walk (RRW)** | rad/s/√s (or deg/hr/√hr) | Bias standard deviation grows as `√(Q₂ · t)` |

Allan variance plots (IEEE Std 952, INDEX-only paywalled) decompose gyro noise into ARW (slope -1/2 on log-log Allan-σ plot at short averaging times) and RRW (slope +1/2 at long averaging times) and several other sub-components (bias instability, quantization, exponentially-correlated drift). For Apsis's nominal model, **ARW and RRW are sufficient**; the others can be added as state extensions if a specific gyro requires them.

This maps directly to **REQ-SEN-005** ("gyro/IMU sensors with bias drift (Markov model), Angle Random Walk, and Rate Random Walk per axis") — the per-axis ARW and RRW are the two scalar parameters per gyro.

## Optional Markov bias variant

[[sources/lefferts-1982-mekf]] Eq. 55 gives an alternative bias model:

```
db/dt = -b/τ + n₂              (Eq. 55, exponentially correlated bias)
```

with time constant `τ` (the "Markov" model REQ-SEN-005 mentions). For `τ → ∞` this collapses to the random-walk model (Eq. 51). The exponentially-correlated form is more realistic for real fiber-optic gyros which have a bounded long-term bias drift. Apsis should support both: the random-walk default is simpler and conservative; the Markov form is closer to truth for specific hardware.

## What the model is NOT

The Farrenkopf model captures the dominant gyro noise sources for **rate-mode operation in the small-noise regime**. It does not capture:

- **Scale-factor errors** (gyro reports `α ω` instead of `ω`). Add as additional state if needed; observable from comparison to known-good attitude updates.
- **Misalignments** (gyro axes not perfectly aligned with the spacecraft body frame). Add as additional state; observable from cross-axis correlation.
- **g-sensitivity** (acceleration-induced bias). Observable in spacecraft only during maneuvers; rarely included as state.
- **Quantization** (digital gyro output at finite resolution). Becomes a noise floor; modeled as additive noise with a flat PSD up to the Nyquist of the gyro sample rate.
- **Bias instability** (1/f noise, between RRW and white). Distinct slope on Allan-σ plot. Add as a colored-noise state if hardware exhibits significant 1/f.

For Apsis v1, the four-state Farrenkopf model (3 attitude error + 3 bias) is the default. Scale-factor and misalignment extensions can be added per-mission via the GNC-stack configuration without touching the MEKF core.

## Discrete-time integration of process noise

Continuous-time `Q₁`, `Q₂` integrate over the prediction interval `Δt` to give the process-noise contribution to the discrete covariance update. [[sources/lefferts-1982-mekf]] Appendix B (Eqs B-15..B-17) gives closed-form approximations for the time-independent case. The standard discrete form for the 6-D MEKF process noise is:

```
N̄ ≈ [ Q₁ Δt + ⅓ Q₂ Δt³        ½ Q₂ Δt² ;
       ½ Q₂ Δt²                Q₂ Δt ]
```

(Schematic; exact form depends on whether `Q₁`, `Q₂` are intensities or PSDs and on the sign/coupling conventions.) Compute once per gyro sample; reuse across many MEKF prediction steps.

## Apsis use

- **Sensor model for REQ-SEN-005** — the IMU/gyro sensor evaluates `ω + n₁ + b` at every sample; the truth simulator integrates `db/dt = n₂` independently.
- **Process-noise model for [[concepts/mekf|MEKF]]** — the same `Q₁`, `Q₂` are passed into the filter as the process-noise specification.
- **Calibration-sim parameters** — Apsis Monte Carlo trials should sample `Q₁`, `Q₂` from a distribution per REQ-MC-004 to study estimator robustness against gyro hardware variation.

## References

- **[[sources/lefferts-1982-mekf]]** §5, Eqs 48-54 — the canonical exposition.
- **Farrenkopf 1978** — *Analytic Steady-State Accuracy Solutions for Two Common Spacecraft Attitude Estimators*, J. Guid. Control 1(4):282-284. AIAA paywall.
- **Farrenkopf 1974** — *Generalized Results for Precision Attitude Reference Systems Using Gyros*, AIAA Paper 74-903. AIAA paywall.
- **IEEE Std 952** — Allan-variance characterization of single-axis FOG. INDEX-only paywalled (already cataloged in `docs/raw/INDEX.md`).
