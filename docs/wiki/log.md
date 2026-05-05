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
