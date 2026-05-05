---
type: decision
title: "Time is a type tagged with its scale; scale mixing is a compile-time error"
status: accepted
decided: 2026-05-05
supersedes: []
superseded_by: null
sources: [sofa-2023-time-scale-cookbook, sofa-2023-earth-attitude-cookbook]
components: []
requirements: [REQ-TIME-002, REQ-TIME-013]
---

## Status

**Accepted** 2026-05-05. Surfaced during the architecture-review grilling on the *Long-arc state conditioning* deepening (candidate #3 in the audit-derived deepening list). Cited from [[concepts/long-arc-state-conditioning]] and from REQUIREMENTS.md REQ-TIME-013 (v0.4).

## Context

Apsis carries time in two-component form `(epoch_jd, offset_seconds)` per REQ-TIME-002. The seven astronomical time scales Apsis supports — TAI, TT, UTC, UT1, TDB, optionally TCG and TCB ([[concepts/time-scales]]) — are not interchangeable:

- TAI vs UTC differs by integer leap seconds (37 as of 2026); ignoring this gives ~37 s position errors at 7.5 km/s = ~280 km of orbital miss.
- TT vs TDB differs by ≤ ~1.6 ms periodic terms; conflating them in JPL DE ephemeris evaluation gives ~12 m position error per ms at LEO velocities.
- UT1 vs UTC differs by `ΔUT1` (~70 s in 2026); using UTC in `iauEra00` produces ~500 m frame-rotation error at the equator.

CLAUDE.md (project-specific guidance) explicitly flags scale-mixing as a *"load-bearing failure mode"*: *"Time scale must be named. TAI / TT / UTC / UT1 / TDB are not interchangeable — when a timestamp or epoch is mentioned, name the scale."* The [[concepts/time-scales]] page lists the canonical bug classes.

The question is whether scale tracking lives at the **comment / convention level** (`utc_now`, `tt_epoch` named variables) or at the **type level** (`UtcTime`, `TtTime` distinct types).

## Decision

Apsis SHALL tag the `Time` type with its scale at the type level. The `Time` type is templated on a `TimeScale` enum and one instantiation exists per supported scale:

```cpp
enum class TimeScale { TAI, TT, UTC, UT1, TDB, TCG, TCB };

template<TimeScale S>
struct Time {
    f64 epoch_jd;
    f64 offset_seconds;
};

using TaiTime = Time<TimeScale::TAI>;
using TtTime  = Time<TimeScale::TT>;
using UtcTime = Time<TimeScale::UTC>;
using Ut1Time = Time<TimeScale::UT1>;
using TdbTime = Time<TimeScale::TDB>;
// optional, for relativistic-strict use:
using TcgTime = Time<TimeScale::TCG>;
using TcbTime = Time<TimeScale::TCB>;
```

Conversions between scales are **explicit free functions** routing through SOFA:

```cpp
TtTime  tt_from_tai(TaiTime);          // SOFA iauTaitt
TtTime  tt_from_tdb(TdbTime, ...);     // SOFA iauTdbtt + iauDtdb
TdbTime tdb_from_tt(TtTime, ...);      // SOFA iauDtdb
UtcTime utc_from_tai(TaiTime);         // SOFA iauTaiutc + iauDat
// ... full graph
```

Mixing scales without an explicit conversion is a **compile-time error**. There is no implicit conversion operator and no scale-erasing `Time` base class.

## Rationale

- **CLAUDE.md scope-guard pressure.** The project explicitly identifies scale mixing as a load-bearing failure mode. Tagging at the type level moves the rule from "discipline maintainers must remember" to "compiler enforces." The latter is much harder to silently violate.
- **The bug class is real and recurring in flight-dynamics codebases.** Practitioners report TT/TDB conflation as one of the top sources of unexplained ~10-20 m discrepancies in OD pipelines; UTC/TAI conflation around leap-second insertion epochs has caused operational anomalies (e.g. some early GLONASS receivers misinterpreted leap-second frames). These bugs survive to production because, with untagged time, the type system gives no warning — it's just two numbers being added.
- **Cost is small.** Templated time slightly complicates downstream API generic code (every API that takes a time has to choose a scale or itself be templated). In practice almost every API has a natural canonical scale (force evaluation in TT, ephemeris query in TDB, frame transform in TT + UT1) and the choice is unambiguous. The compile-time cost is negligible.
- **SOFA precedent.** SOFA's `(jd1, jd2)` API is untagged but its function naming bakes scale into the function names (`iauTaiutc`, `iauDtdb`, etc.). Tagging the inputs / outputs at the Apsis layer recovers the safety SOFA's caller is otherwise responsible for.
- **Auto-rectification composes.** The rectification logic (REQ-TIME-002, [[concepts/long-arc-state-conditioning]]) is per-instance; it operates on `(epoch_jd, offset_seconds)` regardless of scale, so the templated type adds no runtime cost beyond the conversion functions themselves.
- **Aligns with Apsis's broader type-level safety pattern** (frames are tagged at the type level — `Vector3<Frame::ICRF>` vs `Vector3<Frame::ITRF>` — per the architecture's right-handed-throughout discipline; tagging time matches).

## Alternatives considered

**Untagged: a single `Time` struct, scale tracked by naming convention.** Rejected because the bug class it permits is exactly the one CLAUDE.md identifies as load-bearing. Naming-convention discipline degrades over time across teams; type discipline does not. Saves a small amount of type ceremony at the cost of compile-time bug prevention.

**Runtime tag (`Time { f64 epoch_jd; f64 offset_seconds; TimeScale scale; }`).** Rejected because runtime tags add storage cost (one extra word per Time instance, multiplied by the millions of Time instances in a long-arc scenario), need runtime checks at every conversion site, and don't surface bugs at compile time — they degrade silent compile errors into noisier runtime errors. Worst of both worlds.

**Type erasure with virtual dispatch on scale.** Rejected because (a) virtual dispatch defeats inlining of the very-hot conversion paths; (b) inheritance in numerical types is widely considered an antipattern for performance and clarity; (c) no upside over the templated approach.

## Consequences

- The frame-transformation API (per [[decisions/001-use-ceo-based-icrs-to-itrs|ADR-001]]) takes `TtTime` for the precession/nutation polynomial parts and `Ut1Time` for ERA — both inputs are explicitly typed; there's no way to accidentally pass UTC where TT was expected.
- JPL DE ephemeris queries take `TdbTime`; the SOFA-mediated `tdb_from_tt` conversion is required at the call site, surfacing the scale-conversion concern at every point it matters.
- Templates instantiate per-scale, which inflates compile time slightly. Negligible in practice (~7 instantiations).
- **Free-function conversion routines route through SOFA without exception.** No hand-rolled scale conversions anywhere in Apsis. This keeps the conversion logic in one tested place (the SOFA wrappers) and eliminates a class of "almost-right" homemade conversions.
- **Consequence for serialization / I/O** (REQ-INT-007 CCSDS messages, REQ-OBS-002 telemetry): the time scale must be serialised alongside the `(epoch_jd, offset_seconds)` numeric pair. Conventional approach: serialise as `(scale_tag_string, epoch_jd, offset_seconds)`; deserialise into the typed `Time<S>` based on the tag. Type safety is recovered on the deserialised side.
- **Consequence for Python bindings** (REQ-SCN-001): pybind11 doesn't preserve C++ template type tags into Python directly. The Python side gets a single `Time` class with a `.scale` attribute (read-only) and conversion methods (`.to_tt()`, `.to_tdb()`, etc.). Scale-mixing prevention is C++-side; Python users see a unified `Time` with explicit conversions.

## Open items if accepted

- Decide whether the `TimeScale` enum lives in a separate header (lightweight; lots of places need to name a scale) or alongside the `Time` template (compact; one include for time-related types). Likely the former for include-graph hygiene.
- Decide whether to support the optional TCG / TCB instantiations in v1 or defer until a relativistic-strict use case appears. Conversion graph already complete in SOFA; cost is just the type aliases. Recommend include from the start.
- Confirm with future Python-bindings work that the `(scale_tag_string, epoch, offset)` Python representation is acceptable and that the `.to_X()` conversion methods cover the use cases.
