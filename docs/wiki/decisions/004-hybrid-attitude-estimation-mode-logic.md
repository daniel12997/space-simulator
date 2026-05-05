---
type: decision
title: "Attitude estimation uses hybrid mode logic — boot-USQUE plus NIS-monitored MEKF, never single-estimator default"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: [lefferts-1982-mekf, crassidis-2003-ukf-attitude, markley-2003-attitude-error-representations]
components: []
requirements: [REQ-GNC-003, REQ-GNC-014, REQ-GNC-016]
---

## Status

**Accepted** 2026-05-05. Surfaced during the architecture-review grilling on the *Attitude Estimator Family* deepening (audit candidate #5; resolves audit Cluster F). Cited from [[concepts/attitude-estimation-policy]] and from REQUIREMENTS.md REQ-GNC-014 (v0.5).

## Context

Apsis mandates [[concepts/mekf|MEKF]] for attitude estimation (REQ-GNC-003, M). The audit (Cluster F, finding F8.1) flagged that MEKF can fail to converge from large initial attitude errors — [[sources/crassidis-2003-ukf-attitude|Crassidis & Markley 2003]] Figure 3 demonstrates this on a single-magnetometer TRMM simulation: MEKF fails to converge in 8 h while [[concepts/usque|USQUE]] (a UKF-attitude variant the same authors developed) converges in 30 min from the same initial error.

The v0.2 audit response added REQ-GNC-014 (S, "USQUE as acquisition-mode fallback"), but did not specify *when* the fallback is active, *how* the switch happens, or whether MEKF or USQUE is the default. The estimator family was three pieces (MEKF, USQUE, mode logic) scattered across requirements (REQ-GNC-003 + REQ-GNC-014), the [[concepts/mekf|MEKF concept page]] (which mentions USQUE in its last section), and one line in subsystems §5.3.

The question this ADR settles: **what is Apsis's canonical attitude-estimation policy?**

## Decision

Apsis SHALL run a **hybrid mode logic** with both MEKF and USQUE simultaneously available:

1. **Boot in USQUE.** Acquisition phase always starts in USQUE — handles the typical scenario where initial attitude is poorly known.
2. **Switch to MEKF on convergence.** When `||MRP_estimate|| < 10°` AND `trace(P_attitude) < threshold_acquired`, hand off to MEKF (direct covariance copy).
3. **Continuous NIS monitoring of MEKF.** Chi-squared test on innovations: if `NIS_k > χ²_{p=0.99, dof=meas_dim}` for `N=5` consecutive samples, declare MEKF inconsistent.
4. **Revert to USQUE on inconsistency.** Warm-restart USQUE with MEKF's current estimate.
5. **FSM override available.** The broader GNC mode FSM (REQ-GNC-009) may pin the estimator to a specific mode via `pin_mode()`.

USQUE SHALL use the **`(a=1, f=1)` MRP-flavoured GRP** parameterisation ([[concepts/generalized-rodrigues-parameters]]) — algebraically identical to MEKF's MRP error representation, so covariance hand-off is a direct copy with no parameterisation conversion.

The hybrid policy is the **canonical default**. Pinning to a single estimator (always-MEKF or always-USQUE) is a configurable special case, not the default.

## Rationale

- **Covers both startup AND mid-mission divergence.** A simple "boot-USQUE-then-switch" policy handles initialisation but not unexpected divergence (manoeuvre transients, sensor outages, unmodelled torques). Continuous NIS monitoring catches the latter. Both failure modes are real in flight; the hybrid policy is the only option that addresses both.
- **NIS is operational, not academic.** Chi-squared innovation-consistency monitoring has been standard in tracking and navigation for decades; the test runs every measurement step at negligible cost; the threshold is tunable per-mission. There's no honest reason to omit it.
- **Direct hand-off is possible because both filters share parameterisation.** The MRP-flavoured GRP mandate makes the covariance structurally identical between filters. Hand-off is a memcpy of `(q, P_attitude, b, P_bias)`. No conversion bugs, no inflation hacks. This is what makes the hybrid policy *cheap* — the mode-switching machinery is essentially free at runtime.
- **USQUE's compute cost is acceptable for the sub-window where it runs.** USQUE is ~2× MEKF cost; in the acquisition phase (seconds to minutes typically) and in the rare inconsistency-recovery windows, the extra compute is well within Apsis's GNC budget. In nominal operation MEKF runs uncontested.
- **Aligns with documented flight practice** — modern attitude-estimation literature (Markley & Crassidis textbook, NASA GNC handbooks) increasingly recommends multi-estimator policies for missions where single-estimator failure is non-recoverable. Apsis's "flight-dynamics-grade" framing makes the additional engineering investment appropriate.
- **The FSM-override hook keeps mission-engineer control.** Missions that have a clear reason to pin one estimator (deeply-constrained CPU; permanently uncertain attitude requiring USQUE; etc.) lose nothing — `pin_mode()` makes the policy dormant.

## Alternatives considered

**Always-MEKF (no fallback).** Rejected — REQ-GNC-003 was originally written this way; the audit identified it as the load-bearing gap. Documented MEKF failure modes (large initial errors, sparse measurements, recovery from sensor anomalies) leave no robust path back without a fallback. Apsis can't claim flight-dynamics-grade attitude estimation while tolerating the known failure mode.

**Boot-USQUE-then-switch (no NIS monitoring).** Rejected because mid-mission divergence is unaddressed. Saves the NIS test (which is cheap anyway) and a small amount of state-machine logic; gives up the recovery capability that motivates the hybrid in the first place.

**Always-USQUE (no MEKF).** Rejected because MEKF is half the compute and equivalent accuracy in nominal small-angle operation, which is the majority of mission time. Pinning USQUE everywhere is wasteful CPU; some missions have it as a configurable choice (see Consequences) but the default is the hybrid.

**Mission-FSM-driven only (no autonomous policy).** Rejected because estimator-internal evidence (NIS exceedance) doesn't belong in the mission FSM — it conflates two decision layers. The FSM should know about mission phases; the estimator should know about its own consistency. The two compose via `pin_mode()` cleanly.

**Larger NIS persistence threshold (e.g., N=20 instead of N=5).** Considered as a more conservative choice — fewer false-positive switches but slower divergence detection. The N=5 default tracks standard ACS practice and is configurable per-mission; N=5 is a reasonable starting point and missions can adjust.

## Consequences

- **REQ-GNC-014 promoted from S to M** — USQUE is no longer optional; the hybrid policy requires both filters.
- **Two algorithm classes** (`Mekf`, `Usque`) plus one **manager** (`AttitudeEstimator`) plus one **pure-logic policy class** (`AttitudeEstimationPolicy`). Four small modules; the manager is the only one external code touches.
- **Mandatory MRP-flavoured GRP for USQUE.** This is non-trivial to verify (the algebraic identity to MEKF's MRP requires matching the `f=1, a=1` parameterisation choice) but is a one-time check at implementation time.
- **NIS computation per measurement step** — adds one `O(meas_dim²)` cost per measurement update. Negligible compared to the measurement update itself.
- **Test surface increases** — the policy class needs unit tests against synthetic covariance and innovation histories; an end-to-end Monte Carlo regression demonstrating large-initial-error convergence is a CI gate.
- **Mission engineers get a configuration knob set** — six tuning parameters with sensible defaults (acquisition thresholds, NIS test parameters, USQUE scaled-UT parameters). Documented at [[concepts/attitude-estimation-policy]] §"Tuning knobs."
- **Diagnostic surface**: `current_mode()` exposes the active estimator for logging / telemetry. Mission operations centres can monitor mode-switch events as a normal piece of telemetry (a high mode-switch rate signals an estimation problem worth investigating).

## Open items if accepted

- Confirm that the existing [[concepts/mekf|MEKF concept page]] needs updating to remove its current USQUE subsection (replaced by the cross-reference to [[concepts/usque]] and [[concepts/attitude-estimation-policy]]).
- Decide whether `Mekf` and `Usque` are public classes (usable standalone, e.g., for benchmarking) or internal to `AttitudeEstimator`. Recommend public — there's no downside, and standalone use cases (algorithm-vs-algorithm benchmarks, alternative wrappers, custom mode policies) become possible.
- Confirm with future Python-bindings work that the `pin_mode()` hook is exposed cleanly (a Python user should be able to do `est.pin_mode(EstimatorMode.MEKF)` from a scenario script).
- Defer until benchmarking: whether the NIS test should also run when USQUE is active (could be useful to log USQUE consistency for monitoring; current decision is "NIS monitors MEKF only"). Likely Yes after first benchmark.
