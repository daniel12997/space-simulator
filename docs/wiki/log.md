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

## [2026-05-04..05] ingest batch | All specs (15 sources) + ICGEM article (1)

Final ingest pass — all remaining items in `docs/raw/specs/` and `docs/raw/articles/`. Sixteen sources ingested:

**Earth orientation, time, and ephemeris (7):**
- `iers-conventions-2010` — IERS Technical Note No. 36 umbrella for the 11-chapter standards bundle. The load-bearing ICRS↔ITRS spec.
- `sofa-2023-earth-attitude-cookbook` — IAU SOFA cookbook for precession-nutation-Earth-rotation-polar-motion implementation.
- `sofa-2023-time-scale-cookbook` — IAU SOFA cookbook for TAI/UTC/UT1/TT/TCG/TCB/TDB conversions.
- `iau-sofa-2023-software-collection-c` — the actual C source distribution.
- `kaplan-2005-usno-circular-179` — pedagogical companion to IERS Conventions.
- `naif-spice-required-reading` — NAIF SPICE Toolkit "Required Reading" docs (umbrella for 28 HTMLs).
- `archinal-2018-iau-wgccre-2015` — IAU WGCCRE 2015 planetary rotation/orientation report.

**TLE / SGP4 / CDM (3):**
- `hoots-roehrich-1980-spacetrack-report-3` — original public NORAD SGP/SGP4/SDP4/SGP8/SDP8 spec.
- `kelso-celestrak-tle-format` — Kelso/CelesTrak TLE and GP-data format documentation (umbrella for 2 HTMLs).
- `ccsds-508-0-b-1-cdm` — CCSDS Blue Book for the Conjunction Data Message format.

**Environment models (3):**
- `us-standard-atmosphere-1976` — time-invariant reference atmosphere; toy/sanity/fallback per REQ-PHY-006.
- `igrf14-2024-coefficients` — 14th generation International Geomagnetic Reference Field per REQ-PHY-011.
- `icgem-gfz-landing-snapshot` — ICGEM gravity-model repository (the article).

**Conjunction (1):**
- `krisko-2019-ordem-3-1-user-guide` — NASA Orbital Debris Engineering Model 3.1 (statistical sub-tracking-threshold debris flux per REQ-CAT-007).

**MBD tooling (1):**
- `urdf-xacro-pinocchio-docs` — URDF XML spec + urdfdom + Xacro + Pinocchio documentation umbrella for the MBD input pipeline.

**One new concept page:**
- `time-scales` — umbrella for the seven astronomical time scales (TAI/UTC/UT1/TT/TCG/TCB/TDB), the conversion graph, and the bug classes Apsis must avoid.

**Items surfaced for human review (no silent spec edits):**
1. **Permanent-tide convention** (zero-tide vs tide-free vs mean-tide) — subsystems §2.5 must explicitly select per IERS Conventions Ch. 6. Mismatch = ~30 cm offset in C₂₀.
2. **Tides and sub-daily polar motion** (IERS Ch. 7-8) — confirm whether REQ-PHY-003 implies inclusion or whether they're optional extensions.
3. **Relativistic acceleration** (IERS Ch. 10) — make explicit in subsystems §2.5 (recommend default-on for gravitational, default-off for Shapiro/light-time except deep-space).
4. **WGS-72 vs WGS-84 constants in SGP4** — STR#3 mandates WGS-72; modern implementations sometimes drift to WGS-84. Subsystems §1 must specify WGS-72.
5. **CCSDS CDM Pc field** — subsystems §6 should clarify that Apsis-computed Pc supersedes the originator-provided value (originator may use different math).
6. **HBR per-spacecraft config** — CDM does not carry HBR; Apsis must maintain a per-spacecraft HBR alongside CDM ingest.
7. **NAIF SPICE thread-safety** — subsystems §1 / §11 must specify serialization or per-thread-instance approach for multi-threaded MC.
8. **ORDEM 3.1 export-controlled** — Apsis distribution must accommodate (recommend optional plug-in, not hard dep).
9. **IGRF generation freshness** — subsystems §2.6 should specify whether Apsis enforces a recent IGRF generation or accepts SV extrapolation past the model's nominal validity (~5 years).
10. **USStd76 vs NRLMSISE-00 vs JB2008 selection precedence** — subsystems §2.4 should document the recommendation per use case (real-time → NRLMSISE-00/JB2008 + space weather; mission-design → USStd76).
11. **URDF flexible-body and slosh extensions** — vanilla URDF doesn't support flex modes; Apsis's Xacro extension layer must add custom URDF-extension elements that the loader recognizes.
12. **Y2056 TLE wraparound** — subsystems §1 should document policy to migrate to OMM (4-digit year) before 2056.

**All 50 raw artifacts now ingested.**
- 32 papers in `docs/raw/papers/`
- 17 specs in `docs/raw/specs/` (some bundled into umbrella source pages: IERS Conventions 11-chapter, NAIF SPICE 28-HTML, URDF/Xacro/Pinocchio 4-HTML, Kelso 2-HTML)
- 1 article in `docs/raw/articles/`

Wiki state: 47 source pages, 23 concept pages, 1 decision page (proposed).

## [2026-05-05] lint | First post-ingest lint pass

Full-wiki lint after the bibliography pipeline reached steady state.

**Fixed during pass:**
- 8 backslash-escaped pipes (`|`) in wikilink-with-alias syntax — global sed-fix.
- 6 references to undefined `composite-rigid-body-algorithm` concept — created the concept page (CRBA met the 2-citation threshold).
- 6 orphan source pages — added inbound wikilinks from canonical sister pages.

**Final state:** 72 pages (47 sources, 24 concepts, 1 decision). 0 broken wikilinks. 0 orphans.

**Outstanding non-lint items deferred to human:**
- decision 001 still in `proposed` status
- 22 spec-edit suggestions logged across earlier batch entries (require human authorship per CLAUDE.md scope guard)
- no component pages yet (no source code yet)
- no synthesis pages yet (corpus is now broad enough — propose if/when human signals interest)
- Pines 1973 / Cunningham 1970 referenced from `concepts/spherical-harmonic-geopotential` but not in corpus (paywalled)

Full report at [[lint-reports/2026-05-05-post-ingest-lint]].

## [2026-05-05] synthesis | Three-document audit (REQUIREMENTS / architecture / subsystems)

Systematic audit of the three authoritative design docs against the wiki corpus, requested by the user. Produced four synthesis pages:

- `synthesis/audit-requirements-2026-05-05` — 1 HIGH, 9 MEDIUM, 19 LOW (29 findings).
- `synthesis/audit-architecture-2026-05-05` — 0 HIGH, 2 MEDIUM, 9 LOW (11 findings).
- `synthesis/audit-subsystems-2026-05-05` — 0 HIGH, 6 MEDIUM, 16 LOW (22 findings).
- `synthesis/audit-summary-2026-05-05` — cross-document summary, finding clusters, and recommended order of operations.

**Single HIGH finding (F2.1):** REQ-PHY-016 (variational-equations partials) is S-priority but blocks M-priority orbit estimation (REQ-GNC-004) and Pc computation (REQ-CAT-009). Recommend promote to M.

**MEDIUM clusters:** frame precision (ICRF/J2000 conflation across all three docs); tides + Pc precision priority mismatch; SGP4 WGS-72 constants spec gap; flex/slosh priority vs "flight-dynamics-grade" framing; MEKF acquisition-mode fallback gap; CAM analytic-optimal vs along-track restriction; CMG/VSCMG terminology confusion.

**LOW cluster:** mostly text fixes — IGRF-13 → IGRF-14, citations of ADR-001 from REQ/arch/subsystems, TCG/TCB time-scale gap, Pinocchio analytical-derivatives credit, body-frame convention choice, CDM HBR config gap.

**No edits made to the three docs themselves** — per CLAUDE.md scope guard, those are deliberate human-or-jointly-authored. Audit reports cite [[concepts/]] and [[sources/]] supporting each finding so the human reviewer has direct backing material.

## [2026-05-05] spec-edit | Three authoritative docs revised to v0.2 + ADR-001 accepted

User reviewed audit summary and authorized "make the fixes". Applied all 62 audit findings across the three authoritative design docs. Three commits (one per doc) plus the ADR acceptance:

**docs/REQUIREMENTS.md (v0.1 → v0.2):**
- HIGH F2.1 resolved: REQ-PHY-016 S → M.
- 9 MEDIUM findings resolved (frame conflation, JB2008 indices, tides priority+convention, SGP4 WGS-72, flex/slosh promotion, MEKF fallback, CAM optimization, REQ-PHY-012 Schwarzschild S → M).
- 19 LOW findings resolved (text/citation fixes, IGRF version, TCG/TCB option, new requirements for VSCMG / RPO MPC / CDM / HBR / nonlinear controllers / OOS items).

**docs/01-architecture.md (v0.1 → v0.2):**
- 2 MEDIUM and 9 LOW findings resolved (frame distinction, Pinocchio analytical-derivatives credit, IGRF-14, WGS-72 pin, JB2008 + CCSDS CDM dependency rows, Phase 3 deliverable expansion).

**docs/02-subsystems.md (v0.1 → v0.2):**
- 6 MEDIUM and 16 LOW findings resolved (frame distinction, tides defaults, VSCMG framework, MEKF MRP/2nd-order/USQUE-fallback, CAM analytic-optimal, CDM ingest section, RPO MPC controller, body-frame convention, mass-rebuild config, etc.).

**ADR-001:** status `proposed` → `accepted` (2026-05-05). Now cited as load-bearing from all three v0.2 docs.

Audit reports retained as historical record; they accurately describe the state at v0.1. Future audits should be quick deltas against v0.2.

## [2026-05-05] deepening | Variational Equations module

Applied the `improve-codebase-architecture` skill — explore + present-candidates + grilling — to the design corpus. Picked candidate #2 (Force-Model Variational Equations) as the first deepening; grilled through 7 design questions (scope, name, interface shape, FD fallback, conformance, Φ propagation placement, doc threading); landed on:

- **Scope:** full chain — per-force partials → A assembly → Φ propagation.
- **Name:** Variational Equations (domain-standard astrodynamics term).
- **Per-force interface:** `partial_dadr`, `partial_dadv`, `partial_dadp` (the last w.r.t. statically-declared estimable parameters via `estimable_parameters()`).
- **FD fallback:** opt-in `FiniteDifferenceJacobian` helper, base-class methods stay pure-virtual.
- **Conformance:** generic harness over registered force models, gates CI; per-force `conformance_grid()` + `conformance_tolerance()`.
- **Φ propagation:** between-measurement (Option C) atop parallel-integrator mechanism (Option B); RK4 for Φ; step matches state-integrator dense output. Φ not part of natural-state vector — see ADR-002.

**Deliverables:**
- New concept page: [[concepts/variational-equations]] (~200 lines, full contract + math + interface + conformance + consumers + numerical conditioning).
- New ADR: [[decisions/002-variational-equations-between-measurements]] (Option C atop Option B, with rationale and Option A rejection).
- REQUIREMENTS.md v0.2 → v0.3: rewording REQ-PHY-016 / REQ-GNC-004 / REQ-CAT-009; new REQ-PHY-020 (conformance gate, M); new REQ-INT-014 (Variational Equations integrator, M).
- 01-architecture.md v0.2 → v0.3: §3 Force model paragraph reworded; integrator list expanded with VE integrator; §6 build-list adds VE subsystem bullet.
- 02-subsystems.md v0.2 → v0.3: §2.1 ForceModel C++ skeleton expanded; new §3.6 Variational Equations integrator; §5.3 EKF and §6.4 Pc roll-forward cross-reference §3.6.

This deepening resolves the audit's HIGH finding F2.1 by promoting variational-equations partials from a one-line aside in REQ-PHY-016 to a first-class named module with its own concept page, ADR, requirement IDs (REQ-PHY-016, REQ-PHY-020, REQ-INT-014), conformance discipline, and explicit consumers (REQ-GNC-004, REQ-CAT-009).

## [2026-05-05] deepening | Long-arc state conditioning module

Second deepening from the architecture-review candidate list (Two-Component Time + Encke). Grilled through 6 design questions; expanded scope from the original 2-piece framing to a 3-pillar module:

- **Pillar 1 — Two-component time** (representation conditioning): tagged at type level (`Time<TimeScale::TT>`, etc.) per ADR-003; auto-rectification on threshold.
- **Pillar 2 — Encke perturbation propagation** (formulation conditioning): wrapper around any base integrator (`EnckePropagator(std::unique_ptr<Integrator> base, ...)`); per-spacecraft engagement; default 1% rectification threshold; absolute-coordinate dense output so downstream VE integrator is Encke-unaware.
- **Pillar 3 — Compensated summation** (accumulation conditioning): both `CompensatedSum<T>` (per-scalar) and `CompensatedAccumulator<V>` (vectorised); VE integrator's Φ accumulation default-on for the latter.

**Deliverables:**
- New concept page: [[concepts/long-arc-state-conditioning]] (~280 lines, three-pillar architecture + composition argument + test surfaces).
- New ADR: [[decisions/003-tagged-time-scale-types]] (tagged Time type at type level; rejection of untagged / runtime-tagged / type-erased alternatives).
- REQUIREMENTS.md v0.3 → v0.4: rewording REQ-TIME-002 / REQ-TIME-009 / REQ-INT-006 / REQ-INT-007; new REQ-TIME-013 (tagged Time).
- 01-architecture.md v0.3 → v0.4: §2 design principle cross-reference; §3 Foundation > Time system gains tagged-type note; §3 Dynamics > Encke clarified as wrapper; §6 build-list adds long-arc-state-conditioning bullet, replacing standalone Encke bullet.
- 02-subsystems.md v0.3 → v0.4: §1.1 cross-reference; §1.2 expanded with auto-rectification + tagged-type sections; §3.3 expanded with vectorised aggregator; §3.4 expanded with wrapper-pattern detail.

This deepening operationalises the architecture's "Conditioning over precision" design principle (§2): three previously-scattered fragments (REQ-TIME-002 + REQ-INT-006 + REQ-INT-007) plus a new compile-time-safety guarantee (REQ-TIME-013) become one coherent named subsystem with one concept page, one ADR, and explicit composition rules. REQ-TIME-009 (mm precision out to 50 AU) now has a single citable mechanism rather than three independent enabling requirements.

## [2026-05-05] deepening | Attitude Estimator Family

Third deepening from the architecture-review candidate list (#5: MEKF + USQUE + mode logic). Resolves audit Cluster F. Grilled through 6 design questions; landed on:

- **Two pages** (Q1): `concepts/usque` (algorithmic — peer of `concepts/mekf`) and `concepts/attitude-estimation-policy` (operational umbrella).
- **Hybrid trigger** (Q3): boot-USQUE + NIS-monitored MEKF; convergence trigger `||MRP|| < 10° AND trace(P) < threshold`; inconsistency trigger `NIS > χ²_{p=0.99}` for `N=5` consecutive samples; mission-FSM override via `pin_mode()`.
- **Direct covariance hand-off** (Q4): mandate matching `(a=1, f=1)` MRP-flavoured GRP for USQUE so covariance is structurally identical to MEKF's MRP; hand-off is direct copy of `(q, P_att, b, P_bias)`.
- **Manager + pure-logic policy architecture** (Q5): `AttitudeEstimator` (manager satisfying existing `Estimator` interface) wraps `Mekf` + `Usque` + `AttitudeEstimationPolicy` (pure logic, testable against synthetic histories).

**Deliverables:**
- New concept page: [[concepts/usque]] (~150 lines, algorithm + GRP parameterisation + sigma-point construction + Crassidis-Markley discrete-process-noise covariance + when USQUE wins).
- New concept page: [[concepts/attitude-estimation-policy]] (~190 lines, hybrid mode logic + manager architecture + hand-off mechanism + tuning knobs + validation invariants).
- New ADR: [[decisions/004-hybrid-attitude-estimation-mode-logic]] (boot-USQUE + NIS-monitored MEKF; rejection of always-MEKF / boot-only-no-NIS / always-USQUE / FSM-only alternatives).
- REQUIREMENTS.md v0.4 → v0.5: REQ-GNC-003 reworded; REQ-GNC-014 promoted from S to M and expanded with hybrid-policy details + GRP parameterisation mandate; new REQ-GNC-016 (AttitudeEstimator manager).
- 01-architecture.md v0.4 → v0.5: §3 GNC stack note on manager pattern + ADR-004 cross-reference.
- 02-subsystems.md v0.4 → v0.5: §5.3 restructured into 5 subsections (5.3.1 MEKF, 5.3.2 USQUE, 5.3.3 AttitudeEstimator manager + policy, 5.3.4 Orbit estimator, 5.3.5 User-swappable estimators), each with code skeleton and cross-references.

This deepening fully resolves audit finding F8.1 (MEKF mandate had no fallback) and resolves audit Cluster F at the structural level — the estimator family is now a named module with operational policy in one place, not three scattered fragments.

Three deepenings landed in this session; five candidates remain (#1 Conjunction Pipeline, #4 GNC Bus, #6 Deterministic MC, #7 Floating-Base Coupling, #8 Fidelity Scope ADR).

## [2026-05-05] deepening | Conjunction Screening Pipeline

Fourth deepening from the architecture-review candidate list (#1: Conjunction Assessment Pipeline, scope-narrowed in Q1 to screening only — CAM deferred to a future deepening). Largest deepening of the session: 14+ stages across 4 phases, two new requirement IDs (REQ-CAT-016, REQ-CAT-017), one promoted optional method (REQ-CAT-015 Patera at S), one new ADR (ADR-005 strategy interface).

Grilled through 6 design questions:
- **Q1 Scope**: split — Screening this session, CAM as a future deepening (different shapes — streaming pipeline vs discrete algorithm).
- **Q2 Name**: `conjunction-screening` (matches NASA CARA terminology; CAM is a downstream peer concept).
- **Q3 Stage decomposition**: 4 phases (catalog state → propagation → pair reduction → per-pair assessment); user pushed back on broad-phase, leading to revised Phase 3 with three layers (orbit-element pre-filter + analytical distance bounds + strategy-pluggable broad-phase).
- **Q4 Composition**: outer `ConjunctionScreeningPipeline` with named inner-stage modules (Variational Equations pattern); broad-phase is a strategy interface; `CatalogStore` is external persistent service.
- **Q5 Pc method**: registry pattern (Foster + Monte Carlo default; Patera registered alternative at S priority); each method declares its own validity predicate; first-valid-method-in-registry-order rule; `pc_method` diagnostic recorded in event.
- **Q6 Doc threading**: REQ-CAT-006 expanded into two pre-filter layers; REQ-CAT-007 reworded for strategy interface; REQ-CAT-008 adds all-local-minima invariant; REQ-CAT-009 expanded for Pc registry; REQ-CAT-010 reworded for full event payload contract; new REQ-CAT-015 (Patera S), REQ-CAT-016 (pipeline interface M), REQ-CAT-017 (broad-phase strategy interface M).

**Deliverables:**
- New concept page: [[concepts/conjunction-screening]] (~340 lines — phase walkthrough, pipeline shape, performance budget, tuning knobs).
- New ADR: [[decisions/005-broad-phase-strategy-pluggable]] (strategy interface for broad-phase; spatial-hash default, sort-and-sweep required alternative, trajectory-tube as future extension; rejection of pinning one method).
- REQUIREMENTS.md v0.5 → v0.6: 5 reworded (REQ-CAT-006 / -007 / -008 / -009 / -010); 3 new (REQ-CAT-015 / -016 / -017).
- 01-architecture.md v0.5 → v0.6: ECS Systems entry expanded; build-list bullet rewritten for the screening pipeline.
- 02-subsystems.md v0.5 → v0.6: §6 introduction added; §6.1-§6.5 restructured to match concept page's 4-phase organisation; broad-phase strategy interface added; Pc registry pattern documented; CAM moved to forward-pointer.

This deepening resolves the audit's first finding-cluster about scattered conjunction-assessment requirements (14 REQ-CAT IDs spanning a single coherent pipeline) into a named module with explicit phase boundaries, strategy seams, and per-stage testability. Sets up CAM as a clean downstream peer concept for a future deepening — events flow from screening to CAM via a documented payload contract.

Four deepenings landed in this session; four candidates remain (#4 GNC Bus, #6 Deterministic MC, #7 Floating-Base Coupling, #8 Fidelity Scope ADR — plus the deferred CAM as effectively a #9).

## [2026-05-05] design-doc refactor + new synthesis page

Refactored `docs/00-design-overview.md` to focus on Apsis-the-system (forward-looking design statements) rather than mixing design content with process/meta-documentation about how the design was developed. Specifically:

- Removed the "Current state" deepenings table and wiki-state inventory (process content).
- Removed the standalone "Reading order" navigation section.
- Replaced `Per REQUIREMENTS §X / 01-architecture.md §Y` cross-references with direct design statements — doc no longer reads as a meta-summary of spec section locations.
- Merged "Patterns from specific deepenings" into the main "Patterns to follow" section.
- Reordered for clarity: What is → Where going → Build sequence → Decisions → Patterns → Out of scope → Risks → Extending.

Process / meta-documentation moved to new `synthesis/development-state-2026-05-05`:
- Corpus inventory (page counts by type)
- Deepenings landed table (with links to concept pages and ADRs per deepening)
- Remaining deepening candidates
- Audit history pointer
- Corpus organisation diagram
- Reading order through the corpus
- Self-supersession pattern documented (in-place update for drift; successor page for major snapshots)

Net effect: design doc is now ~191 rendered lines at 100-char wrap (within the 200 budget); synthesis page is ~95 rendered lines and grows as needed.

Reasoning for the split: the design doc should be a **declarative statement** about what Apsis IS and DECIDES — a stable artifact that doesn't drift as the spec docs get reorganised or as deepenings continue. Process / development-state content is its own concern with its own update cadence and lives where similar artifacts (audit syntheses) already live.

## [2026-05-05] decision-batch | 006–012 phase-0/phase-1 implementation ADRs

Seven decisions landed in one session ahead of writing the Phase 0 and Phase 1 plan documents (`docs/phase-0-plan.md`, `docs/phase-1-plan.md`). Driven by the brainstorm → design → structure → plan workflow: structure outline (`docs/structure.md`) committed Apsis to specific seams without pinning the implementation libraries, languages of those seams, or the bootstrap stack — these ADRs close that gap.

**Tooling tier (006–008, 011):**
- [[decisions/006-cmake-cpm-build-system]] — CMake ≥ 3.25 with CPM.cmake (vendored single header) for upstream fetches. Rejected vcpkg / Conan / Meson / Bazel / submodules.
- [[decisions/007-googletest-test-framework]] — GoogleTest 1.15.2 with gmock. Rejected Catch2 / doctest. Per user preference; gmock equivalence in alternatives is a separate dep.
- [[decisions/008-vendor-sofa-and-cspice]] — both libraries vendored as `external/sofa/` and `external/cspice/` with hand-written CMakeLists; pinned by upstream-tarball SHA-256; CSPICE thread-unsafety contained at the `IEphemeris` seam.
- [[decisions/011-reference-data-shipping]] — two-tier policy: small canonical slices vendored under `data/` (DE440 truncated, EOP slice, EGM2008 deg-20, ISS reference vectors); large artefacts (full DE441, full EGM2008) fetched at configure time gated by `APSIS_FETCH_LARGE_DATA=ON`.

**Architectural tier (009–010, 012):**
- [[decisions/009-hand-rolled-integrator-family]] — DOP853 (adaptive RK), Yoshida-4 (symplectic), Gauss-Jackson 8 (multi-step) all hand-rolled behind one `IIntegrator` seam, all stepping `(state, Φ, dt)` directly to honour the VE contract from [[decisions/002-variational-equations-between-measurements]]. Rejected Boost.Odeint (its observer-pattern step interface fights the VE contract) and SUNDIALS (heavy; non-stiff).
- [[decisions/010-phantom-typed-time-and-state]] — operationalises [[decisions/003-tagged-time-scale-types]] (which decided *that* tagging exists) by deciding *how*: bespoke phantom tag types in `apsis::time::tags` and `apsis::frames::tags` with `Time<Scale>` and `State<Frame>` class templates. Rejected Mp-Units / Boost.Units (overkill; we don't need dimensional analysis), strong-typedef libraries (no compose with two-component representation).
- [[decisions/012-eigen-with-apsis-math-aliases]] — Eigen 3.4 underneath, `apsis::math::{Vec3, Mat3, Mat6, Quat, ...}` `using` aliases at the public API. Pinocchio (Phase 3) forces Eigen into the dep tree; choosing anything else creates a two-library design with conversions at every MBD seam. L3a "alias-only" form chosen with a documented escape hatch to L3b "wrapper struct" if compile-time errors motivate it.

**Cross-references updated:**
- [[concepts/long-arc-state-conditioning]], [[concepts/variational-equations]], [[concepts/gauss-jackson-integration]] gain back-links to ADR-009 as the canonical Apsis implementation of the integrator family.
- [[concepts/time-scales]] gains a back-link to ADR-010 as the canonical Apsis tagging implementation.

**Not landed yet (deferred):**
- Updating `docs/00-design-overview.md` "Design decisions" section from "Five accepted ADRs" to twelve. Substantive design-doc edits are deliberate; flagging for a separate human-or-jointly-authored pass.
- Concept back-links from ADR-012 to [[concepts/pinocchio-library]] — judged too tangential (the ADR cites the concept as a constraint, not as a canonical implementation of it).

These ADRs unblock writing `docs/phase-0-plan.md` (project bootstrap) and `docs/phase-1-plan.md` (propagator core) — the implementation plans that immediately follow this decision batch.

## [2026-05-05] decision + process | 013 Class D classification + SMP/SDP/STP skeleton

Established Apsis's NASA software-classification posture and the Class D process artefacts that NPR 7150.2D mandates for that class.

- **[[decisions/013-class-d-software-classification]]** — Apsis is Class D (mission-support / engineering software) under NPR 7150.2D. Justifies the choice against Class A/B (flight-grade — would invalidate ADRs 005/007/009/012 and add 6–12 months of structural rework for guarantees ground software doesn't need to provide), Class C (overkill — added rigor doesn't change defect-detection outcomes for our workload), and Class E (under-rigorous — strips the test-plan formalism the regression-case discipline depends on). Includes a mandatory-practice mapping table from each Class D requirement to the Apsis artefact that addresses it.
- **`docs/process/software-management-plan.md`** (SMP) — project organisation, lifecycle model (QRSPI iterative loop), configuration management, risk management, tooling, standards-adherence claims.
- **`docs/process/software-development-plan.md`** (SDP) — development environment, project-internal coding standards (frames-first-class, time-scales-first-class, VE contract, strategy-interface-only-at-real-seams, vocabulary discipline, conformance-via-the-interface), source-tree layout, branch / PR / review process, build infrastructure, documentation requirements.
- **`docs/process/software-test-plan.md`** (STP) — test categories (smoke / unit / conformance / regression / sanitizer / bench), coverage targets (≥ 80% statement on first-party code; gcov gate as Phase 1 followup), reference regression cases per phase, requirements-traceability mechanism (test-file frontmatter + lint script as Phase 1 followup), reporting cadence.

**Tracked Phase 1 followups added by this batch:**
- gcov coverage gate in CI with PR-comment delta rendering (STP §3).
- `tools/lint/req_traceability.py` for REQ-* ↔ test mapping (STP §7).
- GitHub branch-protection rules on `main` before first external PR (SDP §4).
- Release process document at Phase 6 completion (SDP §8, SMP §6).
- Reclassification clause: if a derived Apsis component is integrated into flight software, the consuming flight project performs its own classification of that component independently (ADR-013 Consequences).

**Not landed yet (deferred):**
- Updating `CLAUDE.md` "Commit Message Prefixes" with `process:` (or normalising on `meta:`). Skipped to keep this PR scoped to the Class D paperwork itself.
- Updating `docs/00-design-overview.md` "Design decisions" section to mention ADR-013. Substantive design-doc edits remain deliberate human-or-jointly-authored passes.

This batch is process bolt-on — no code touched, no architectural ADRs revised, no existing artefacts moved. The Class D envelope is documentation of the framework Apsis already implements in spirit, not a new development discipline.

## [2026-05-05] ingest + decision-update | Phase 1A Batch C — SphericalHarmonic quality

Phase 1A Batch C of the hardening sprint closed two Phase-1
`SphericalHarmonic` deferrals: the body-fixed rotation gap (issue #11)
and the analytical SH gradient (issue #7). The companion wiki edits:

- **[[sources/orekit-holmes-featherstone-impl]]** — new source page
  citing Orekit's `HolmesFeatherstoneAttractionModel.java`
  (Apache-2.0) as the open-source reference implementation studied
  while porting the Cunningham gradient. Apsis chose the Cunningham
  formulation (M-G §3.2.5) over Holmes-Featherstone because it stays
  inside the existing V/W skeleton; the Orekit file is cited for
  attribution and audit-trail completeness.
- **[[decisions/009-hand-rolled-integrator-family]]** — new "Phase 1A
  Implementation Note — Batch C closures" section appended below the
  Phase 1 note. Documents the C1 (rotation) and C2 (analytical
  gradient) closures, the algorithm choice (Cunningham V/W over
  Holmes-Featherstone / Pines), and the empirical residual the
  conformance gate sees (~1e-10 on J2-only deg=2; ~2.4e-10 on full-
  order deg=8 synthetic; comfortably below the 1e-6 PointMass-sibling
  tolerance).
- **`docs/wiki/index.md`** — sources count 47 → 48; new entry under
  *Force models*.

This is `ingest:` + `decision:` (the source page is an ingest; the
ADR-009 update is a decision-rev). Source code edits in this batch
(`src/force/spherical_harmonic.cc`, `tests/conformance/...`,
`tests/unit/force/...`) are *not* wiki operations — they live on the
implementation-side of the wiki and are tracked by the project's
git history, with `code:` references to follow as `components/`
pages get filled in for the force-model adapters.

## [2026-05-06] ingest | Berry-Healy 2004 GJ8 coefficient generator (with upstream Lisp bug fix)

Created [[sources/berry-healy-2004-gj8-generator]] — Liam Healy's 2005 Common Lisp generator for the named coefficient tables of [[sources/berry-healy-2004-gauss-jackson]] (Tables 3, 4, 5, 6 + Eqs 27/32/44/48/68/70). Sourced from UMD's DRUM repository handle `http://hdl.handle.net/1903/2202`. Reliability tier `primary` (source code by one of the paper's two authors).

Bundle:
- `docs/raw/code/berry-healy-2004-gj8-generator/coefficients.lisp` — Healy's original ANSI Common Lisp source, preserved verbatim as a corpus archive.
- `coefficients.py` — Python port using `fractions.Fraction` for exact-rational arithmetic; function-by-function faithful translation of the Lisp **except** `half_acceleration_in_sum` which carries a corrected diagonal-alignment logic.
- `coefficients-output.txt` — captured stdout (after the fix); canonical reference for downstream C++ transcription.
- `verify.py` — algebraic-identities + paper-cross-check harness; cross-checks at every named anchor.

**Notable finding**: a clear-eyes review of the Python port discovered a 20-year-old latent bug in Healy's Lisp `half-acceleration-in-sum` — the Lisp's `(if (= k j) ...)` adds +1/2 at the wrong column (off-by-one diagonal vs the paper's "current point" diagonal). Effect: the Lisp's Table 5 output disagrees with the paper at 18 of 90 cells. The paper's printed Table 5 was NOT produced by Healy's Lisp — it was either generated by a different code path or hand-shifted at publication. Paper printed values are correct; Lisp generator is wrong.

Resolution: corrected the Python port's `half_acceleration_in_sum`; Lisp file preserved unmodified as historical record. The `verify.py` harness passes at every checked Table 5 anchor including paper-corner cells `(j=±4, k=∓4) = ±19087/89600` and the corrector-current-point `(j=4, k=4) = -19087/89600`.

Use in Apsis: this becomes the source-of-truth for the Phase 1A Batch D'' GJ8 coefficient transcription into C++. The prior implement-cycle attempt at GJ8 (Phase 1A Batch D, deferred under T4) had hit both the "transcription bridge" (PDF-OCR fragile) and "first-principles bridge" (shift-convention disagreement) failure modes. The generator output now provides an unambiguous algebraic reference for the C++ tables, with `verify.py` as the regression test.

## [2026-05-05] decision | Phase 1A Batch D'' (GJ8 re-attempt) closure on ADR-009

Updated [[decisions/009-hand-rolled-integrator-family]] with a Phase 1A Batch D'' closure paragraph documenting:
- Coefficient table transcribed from `docs/raw/code/berry-healy-2004-gj8-generator/coefficients-output.txt` (the verified-fix Python port, paper-cross-checked at every named anchor) into `src/integrate/gj8_coeffs.h`. FNV-1a hash baseline pinned at `0x02AED205E063D443ULL` with `tools/dev/recompute_gj8_hash.sh` for intentional rebaselining.
- `GaussJackson8` adapter behind `IIntegrator` (per ADR-009), with f-and-g starter (Berry-Healy 2004 §5.1, central-difference Phi STM oracle as Phase 1A acceptable approximation).
- Algorithm convention triage: predictor uses half-step first sum; corrector uses TRAPEZOIDAL first sum `s_lead + 0.5 * (ddot_r_lead + ddot_r_slot8_new)`. Identified empirically (constant-acceleration self-consistency does not discriminate trapezoidal from full-step; circular Kepler shows ~16 m/s per-step v error under full-step vs ~4e-12 m/s under trapezoidal).
- Empirical residuals on dev host: Kepler 1-period at h=60s ~1.21e-6 m / ~1.27e-9 m/s; Phi 1-hour at h=60s passes 1e-3 m / 1e-6 m/s gate.
- Both `IntegratorKepler.GaussJackson8` and `IntegratorPhi.GaussJackson8` rejoin the parameterised conformance gate (alongside Dp54, Dop853, Yoshida4).

The original Batch D's TWO bridge failures (PDF-OCR drift; first-principles shift-convention disagreement) do NOT recur — the generator-corpus addition + Lisp-bug fix gave an unambiguous algebraic source. The convention triage (final fix) was the only material Batch D'' debugging effort.
