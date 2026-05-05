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
