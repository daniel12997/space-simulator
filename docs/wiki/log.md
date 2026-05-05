# Apsis Wiki Log

Append-only chronological record of wiki operations.

## [2026-05-04] ingest | Expressions for IAU 2000 precession quantities (Capitaine, Wallace & Chapront 2003)

Created `sources/capitaine-2003-iau2000-precession-p03`. The paper defines what is now called the **IAU 2006 precession** model (= P03), adopted by IAU GA 2006 Resolution B1. Ingest also created six concept pages from scratch (the wiki was empty): `precession-nutation` (umbrella), `iau-2006-precession`, `celestial-intermediate-pole`, `celestial-ephemeris-origin`, `earth-rotation-angle`, `frame-bias`.

One **proposed** decision page added: `decisions/001-use-ceo-based-icrs-to-itrs` — Apsis's architecture and subsystems docs specify "IAU 2006/2000A precession-nutation" but do not pick between the two equivalent IAU transformation pipelines (equinox-based vs CEO-based). The decision proposes CEO-based as canonical; awaiting human sign-off.

Two items surfaced for human review of the authoritative spec docs (no silent edits):

1. The CCI ↔ CCF transformation pipeline in `docs/02-subsystems.md` §1.3 doesn't enumerate the **frame-bias** step (~17 mas). For the CEO-based pipeline this is fine (bias is absorbed into X, Y); for the equinox-based pipeline it must be an explicit step.
2. `docs/01-architecture.md` §3 Foundation > Frames says "IAU 2006/2000A precession-nutation for ICRF↔ITRF" without specifying which transformation pipeline (equinox vs CEO). See `decisions/001-use-ceo-based-icrs-to-itrs`.

## [2026-05-04] ingest | Implementation of Gauss-Jackson Integration for Orbit Propagation (Berry & Healy 2004)

Created `sources/berry-healy-2004-gauss-jackson`. The reference implementation paper for the GJ8 integrator mandated by REQ-INT-002. Derives Adams / summed-Adams / Störmer-Cowell / Gauss-Jackson from a unified backward-difference operator algebra; tabulates eighth-order ordinate-form coefficients as exact rationals; lays out startup, predictor-corrector iteration patterns, and stability/accuracy benchmarks across ISS and CRRES test orbits.

Two new concept pages: `gauss-jackson-integration` (the method) and `f-and-g-series` (two-body Taylor primitive used both for GJ startup and for the standalone Keplerian propagator under REQ-INT-004 — will be re-cited from a future Keplerian-propagator ingest).

No new decision pages this round. The commit to GJ8 is already in REQ-INT-002, so there's no design decision to extract. Implementation-detail choices (PEC vs PECE iteration mode; restart vs Encke correction at eclipse boundaries) are flagged on the source page for when a `gauss-jackson-integrator` *component* page is later authored.

Empirical finding worth highlighting: "step size has more of an effect on accuracy than the order of the method does, and that higher orders are subject to instability at large step sizes" (Berry & Healy 2004, p. 356). GJ12/14 become unstable at 240s LEO step where GJ8 remains stable — explains why the U.S. space surveillance community standardised on eighth-order specifically.

## [2026-05-04] ingest | The Pinocchio C++ library (Carpentier et al. 2019)

Created `sources/carpentier-2019-pinocchio`. The introductory paper for the Pinocchio rigid-body dynamics library Apsis uses for its multi-body subsystem (architecture build-vs-reuse table; subsystems §4). Describes the model/data separation, the spatial-algebra foundation, the FK/RNEA/CRBA/ABA algorithms, the analytical-derivatives feature (Pinocchio is the first MBD framework with this), and benchmarks against RBDL.

Five new concept pages: `pinocchio-library` (the library + design choices), `spatial-algebra` (Featherstone SE(3)/Motion/Force/Inertia umbrella, will be cross-cited from many future MBD sources), `articulated-body-algorithm` (ABA — forward dynamics, called every integrator step), `recursive-newton-euler-algorithm` (RNEA — inverse dynamics for REQ-MBD-003), `floating-base-dynamics` (the SE(3) free-flyer pattern at the heart of Apsis's spacecraft coupling — REQ-MBD-001, subsystems §4.2).

CRBA concept page deferred — passes the threshold (REQ-MBD-007 mandates mass-matrix queries) but the page would be thin until a specific use site (LQR/MPC controller component) exists.

No new decision pages — "use Pinocchio for MBD" is already in the architecture's build-vs-reuse table; choosing ABA for forward dynamics inside Pinocchio is the only sensible option.

One item surfaced for human review of the authoritative spec docs (no silent edits): **Pinocchio does not apply joint friction or damping torques automatically.** REQ-MBD-006 mandates them; subsystems §4.3 step 3 ("Apply effector forces") should be augmented with one sentence noting that Apsis's integration loop must compute friction/damping from joint velocity + URDF parameters and add them to the external-torque vector before calling ABA. Pinocchio reads `<dynamics damping="..." friction="..."/>` into the Model only as metadata.

## [2026-05-04] ingest | Analytical Derivatives of Rigid Body Dynamics Algorithms (Carpentier & Mansard 2018)

Created `sources/carpentier-2018-rbd-analytical-derivatives`. The derivation paper for Pinocchio's analytical-derivative feature — the load-bearing enabler of REQ-GNC-008 (MPC attitude/orbit controller). Three contributions: analytical RNEA derivatives via spatial-algebra chain-rule (Algorithms 2, 3); the novel identity `∂FD/∂u = -M⁻¹ · ∂ID/∂u` (Eq. 24) that makes ABA-derivatives a free byproduct of RNEA-derivatives; and a direct M⁻¹ algorithm that doesn't materialize M.

One new concept page: `analytical-rbd-derivatives` — the chain-rule structure, the spatial-algebra trick (Eqs 13-14), benchmarks vs finite differences (6× on KUKA, 13× on HyQ, 27× on Atlas), benchmarks vs autodiff+codegen (30-60% faster), and the Apsis MPC cost ballpark.

Back-fill citations into existing pages: `concepts/articulated-body-algorithm` and `concepts/recursive-newton-euler-algorithm` gained "Derivatives" sections referencing the new source and concept; `concepts/pinocchio-library` strengthened the analytical-derivatives entry with the speedup numbers; `sources/carpentier-2019-pinocchio` had its forward "next planned ingest" reference replaced with proper back-links.

No new decision pages — the choice of analytical (over autodiff or finite-differences) is implementation-detail, belongs on a future MPC component page when there's code.

One item surfaced for human review (no silent spec edits): the architecture's build-vs-reuse table at `docs/01-architecture.md` §5 credits Pinocchio for *"Featherstone algorithms, autodiff support"* — undersells the actual capability. **Analytical derivatives are 30-60% faster than autodiff+codegen and numerically exact**; should be the canonical MPC derivative path with autodiff as fallback. Worth one sentence in the arch text.

## [2026-05-04] ingest | Kalman Filtering for Spacecraft Attitude Estimation (Lefferts, Markley & Shuster 1982)

Created `sources/lefferts-1982-mekf` — the seminal MEKF paper. Survey-and-derivation work that reviews the historical development of attitude Kalman filtering, establishes the quaternion as the canonical attitude state via Stuelpnagel's no-go theorem, formalises the Farrenkopf gyro model, and compares three covariance representations to demonstrate the 6×6 multiplicative form is the only one that's both non-singular by construction and numerically robust in finite precision.

Three new concept pages: `mekf` (the algorithm — multiplicative error parameterisation, incremental-update-and-reset pattern, Joseph form, sequential scalar updates), `quaternion-attitude-representation` (Stuelpnagel's theorem, multiplicative composition convention matching rotation-matrix order rather than Hamilton's, the periodic Newton renormalisation step), `farrenkopf-gyro-model` (canonical rate-mode gyro model: ω = ũ - b - n₁; db/dt = n₂; ARW ↔ √Q₁, RRW ↔ √Q₂; Markov-bias variant for finite τ).

Anchors REQ-GNC-003 (MEKF mandate), REQ-SEN-005 (gyro/IMU with bias drift Markov model + ARW + RRW per axis), and the gyro-stellar attitude-determination architecture that REQ-SEN-001/002 + REQ-SEN-005 collectively realise.

`kalman-filter` standalone concept page deferred — folded a brief recap into `mekf`, plan to break it out on the next ingest (Crassidis 2003 UKF) when a second source cross-cites it.

No new decision pages — REQ-GNC-003 is unambiguous about MEKF; the multiplicative-vs-additive choice is what this paper *argues for* and what the requirement *settled on*.

One item surfaced for human review (no silent spec edits): `docs/02-subsystems.md` §5.3 says *"Error parameterization is a 3-vector (small rotation), so the covariance is non-singular."* Correct but slightly underspecified — doesn't make explicit the structural separation between the **estimate** state (4-component quaternion on the unit-norm manifold) and the **covariance** state (3-component vector part of the error quaternion in unconstrained R³). Worth one sentence to head off the easy misreading that "we use a three-parameter rotation representation", which Stuelpnagel proves is impossible globally — and that's precisely *why* MEKF separates estimate from covariance.

## [2026-05-04] ingest | Unscented Filtering for Spacecraft Attitude Estimation (Crassidis & Markley 2003)

Created `sources/crassidis-2003-ukf-attitude` — the canonical USQUE (UnScented QUaternion Estimator) paper. UKF + multiplicative quaternion + GRP error representation; converges from large initial errors where MEKF fails outright (Figure 3 — 8h non-convergence vs 30 min convergence on TRMM single-magnetometer simulation).

Two new concept pages: `kalman-filter` (the umbrella, finally broken out per the deferred plan from the MEKF ingest — now justified by 2 cross-citations) and `unscented-kalman-filter` (the UKF algorithm — sigma points, unscented transform, λ tuning, EKF cost comparison).

Edits to existing pages: `mekf` got a "USQUE" section discussing the UKF variant for attitude as the MEKF fallback; `farrenkopf-gyro-model` back-filled with the Crassidis 2003 citation (same model, σ_v/σ_u letter notation, with the closed-form discrete process-noise covariance in Eq. 42). Deferred `generalized-rodrigues-parameters` and `usque` standalone concept pages — only one source citing each currently; will create when Markley 2003 (in corpus) is ingested.

Two items surfaced for human review (no silent spec edits):
1. **MEKF can fail to converge from large initial errors.** REQ-GNC-003 mandates MEKF without a fallback; subsystems §5.3 doesn't address this. Worth considering USQUE as contingency / safe-mode / acquisition variant. Cost: ~2× MEKF.
2. **The "UKF avoids Jacobians" advantage is reduced for Apsis** because Pinocchio's analytical derivatives are competitive with sigma-point cost. The reason to prefer UKF for orbit estimation (REQ-GNC-005) should be framed as "robustness to nonlinearity," not "Jacobian avoidance." Worth a sentence in subsystems §5.3.

## [2026-05-04] ingest batch | Time/Frames + Force Models clusters (12 sources)

Quick-ingest pass through the time/frames and force-models clusters (per user mandate "keep going until all raw files are done"). Twelve sources:

**Time / frames (7):**
- `mathews-2002-mhb2000-nutation` — IAU 2000A nutation derivation (MHB2000), the official IAU model.
- `wallace-capitaine-2006-iau2006-procedures` — practical procedures for IAU 2006 precession + IAU 2000A nutation, including CEO and equinox-based forms.
- `capitaine-wallace-2008-concise-cio` — concise CIO-based precession-nutation formulations (computational-cost minimization for operational use).
- `charlot-2020-icrf3` — Third realization of the ICRF, defining the inertial axes Apsis treats as the "true" ICRS.
- `folkner-2009-de421` — JPL DE421 ephemeris (predecessor to DE440/DE441).
- `park-2021-de440-de441` — JPL DE440/DE441, **the canonical Apsis ephemeris** per REQ-ENV-001.
- `lyddane-1963-small-eccentricity-inclination` — companion to Brouwer 1959, eliminates e=0 / i=0 singularities via Poincaré canonical variables. Together they form the "Brouwer-Lyddane mean elements" that SGP4 uses.

**Force models (5):**
- `pavlis-2012-egm2008` — EGM2008 Earth gravity to degree 2190; the Apsis Earth-gravity reference per REQ-PHY-003.
- `lemoine-2014-grgm900c` — GRGM900C lunar gravity to degree 900; Apsis lunar reference (predecessor to GRGM1200A which subsystems §2.2 names).
- `picone-2002-nrlmsise-00` — NRLMSISE-00 thermospheric density (REQ-PHY-006 default).
- `bowman-2008-jb2008` — JB2008 thermospheric density (REQ-PHY-008 alternative).
- `brouwer-1959-artificial-satellite-theory` — foundational mean-element analytical satellite theory (Delaunay variables + von Zeipel averaging) underlying SGP4.

**One new concept page**: `spherical-harmonic-geopotential` — the umbrella for both EGM (Earth) and GRGM (Moon) models; threshold met (cited by Pavlis 2012 + Lemoine 2014 + future expected from spec ingests).

**Edits to existing concepts**: `precession-nutation` and `iau-2006-precession` got back-fill citations from Mathews 2002 and Wallace-Capitaine 2006 to triangulate the IAU 2006/2000A construction.

**Items surfaced for human review (no silent spec edits)**:
1. Subsystems §2.2 names "GRGM1200A to 165×165 for low orbits" — current corpus has GRGM900C (Lemoine 2014); methodology unchanged but should grab the GRGM1200A reference paper when convenient. GRGM900C usable in the meantime.
2. JB2008 (REQ-PHY-008) requires four solar indices (F10, S10, M10, Y10) plus Dst — broader space-weather ingestion than NRLMSISE-00 (just F10.7 + Ap). Worth confirming REQ-ENV-005/006 cover the additional indices.
3. Brouwer 1959's series diverge at the **critical inclination** (i ≈ 63.43°). SGP4 implementations use Lyddane's reformulation everywhere; Apsis's mean-element propagator should do the same — worth a note in subsystems §3 if not already there.
4. DE441 covers ±13,200 years from J2000; DE440 ±100 years. Apsis should default to DE440 (smaller, fits in memory) and load DE441 only for long-arc / historical cases.

## [2026-05-04] ingest batch | SGP4 + MBD/flex/slosh + attitude est/control + CA/RPO clusters (14 sources)

Quick-ingest pass through the remaining four paper clusters (per "keep going until all raw files are done"):

**SGP4 cluster (2):**
- `vallado-2006-revisiting-spacetrack-3` — modern reference SGP4 implementation (REQ-INT-005, REQ-CAT-002).
- `vallado-2008-sgp4-orbit-determination` — companion DC fitter for producing TLEs from external ephemerides.

**MBD / flex / slosh cluster (5):**
- `likins-1970-flexible-space-vehicles` — JPL TR 32-1329, foundational hybrid-coordinate method for rigid + flexible spacecraft.
- `mistry-2010-floating-base-inverse-dynamics` — analytically-correct floating-base inverse dynamics via QR/orthogonal decomposition (capture/berthing/landing).
- `abramson-1966-nasa-sp-106-slosh` — NASA SP-106, foundational propellant slosh monograph.
- `dodge-2000-swri-slosh-update` — SwRI 2000 update of SP-106 with low-g and spinning-tank chapters.

**Attitude estimation / control cluster (4):**
- `markley-2003-attitude-error-representations` — taxonomy of attitude error representations + second-order MEKF.
- `julier-uhlmann-1997-ukf` — original UKF paper.
- `wan-vandermerwe-2000-ukf` — modern UKF (scaled UT, augmented state, parameter estimation).
- `schaub-1998-vscmg` — VSCMG feedback control law unifying RW/CMG/VSCMG actuators.

**CA / RPO cluster (3):**
- `foster-estes-1992-jsc-25898-pc` — NASA JSC-25898, the Foster 2D analytic Pc method (REQ-CAT-005 canonical algorithm).
- `bombardelli-2015-collision-avoidance` — analytic optimal impulsive CAM via b-plane eigenvalue formulation (REQ-CAT-008).
- `newman-2022-cara-best-practices` — NASA CARA operational practices grounded in the CA² Handbook.
- `dicairano-2012-mpc-rendezvous` — MPC for spacecraft rendezvous and proximity ops (REQ-GNC-006).

**Two new concept pages:**
- `sgp4` — finally broken out as its own page with the Brouwer-Lyddane / TEME / family relationships.
- `generalized-rodrigues-parameters` — promoted from the deferred list now that Markley 2003 + Crassidis 2003 + Schaub 1998 all cite it.

**Items surfaced for human review (no silent spec edits):**
1. Subsystems §5.3 should specify MRP for the 3-vector covariance state and second-order MEKF as default per Markley 2003; current text just says "3-vector small rotation".
2. REQ-GNC-003 mandates MEKF without a fallback; subsystems §5.3 should consider USQUE as contingency / acquisition-mode variant given MEKF's documented large-error convergence failures.
3. Subsystems §5.4 actuator suite — VSCMG framework subsumes RW, SGCMG, DGCMG; consider implementing the most general case per Schaub 1998 with the others as configurations.
4. Subsystems §6 should distinguish impulsive CAM (Bombardelli analytic) from constrained-trajectory RPO MPC (Di Cairano) — different algorithms for different mission phases.
5. Subsystems §4.4 propellant management — Apsis should explicitly state whether it covers PMD-equipped tanks or only free-surface tanks (Dodge 2000 Ch. 11 is the PMD reference).
6. JB2008 (REQ-PHY-008) needs four solar indices (F10, S10, M10, Y10) plus Dst; broader space-weather ingestion than NRLMSISE-00 (just F10.7 + Ap). Confirm REQ-ENV-005/006 cover these.

All 32 papers now ingested. Specs cluster (18 items) is next.
