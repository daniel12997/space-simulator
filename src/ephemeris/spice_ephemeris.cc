// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §4: SpiceEphemeris — the CSPICE seam (ADR-008).
//
// Discipline:
//   * Every CSPICE call (`*_c(`) MUST be inside std::lock_guard<std::mutex>
//     on `spice_mutex_`. The cspice_seam.py CI lint enforces "no CSPICE
//     calls outside src/ephemeris/", but the lock-guard discipline is
//     human-enforced inside this file.
//   * On construction, we promote CSPICE error handling to RETURN mode so
//     a bad kernel path doesn't abort the process; we then check
//     failed_c() and convert to std::runtime_error at the seam.

#include "apsis/ephemeris/spice_ephemeris.h"

#include <stdexcept>
#include <string>

extern "C" {
#include "SpiceUsr.h"
}

namespace apsis::ephemeris {
namespace {

// SPICE J2000-of-date frame name (which is ICRF-aligned to within the bias
// SOFA's iauBp06 returns; for the Phase 1 force model precision, treating
// SPICE "J2000" output as ICRF is exact at the documented kernel quality.
// A separate adapter could compose the J2000->ICRF bias for stricter use).
constexpr const char* kSpiceFrame = "J2000";
constexpr const char* kAbcorr     = "NONE";
constexpr int         kObserverSSB = 0;  // NAIF: 0 = Solar-System Barycentre

// Read CSPICE's pending error message, reset the error stack, and throw.
// Caller must hold spice_mutex_.
[[noreturn]] void throw_spice_error(const std::string& context) {
  constexpr SpiceInt kBuf = 1840;
  SpiceChar msg[kBuf];
  // "LONG" gives the multi-line technical message; this is a developer-
  // facing API so we ship the long form.
  getmsg_c("LONG", kBuf, msg);
  reset_c();
  throw std::runtime_error(context + ": " + msg);
}

// Two-component TDB Julian Date -> ephemeris time (seconds past J2000).
// SPICE's spkezr_c takes ET in seconds past J2000; we never call str2et_c
// because we already have a numeric TDB epoch.
double tdb_to_et_seconds(apsis::time::Time<apsis::time::tags::TDB> t) {
  // J2000 TDB = JD 2451545.0. To preserve precision we subtract the integer
  // portion before scaling.
  constexpr double j2000_jd = 2451545.0;
  return ((t.jd1() - j2000_jd) + t.jd2()) * 86400.0;
}

// km / km-per-second -> m / m-per-second. SPICE units are kilometre-based;
// the rest of Apsis is SI metres throughout.
constexpr double kKmToM = 1000.0;

}  // namespace

SpiceEphemeris::SpiceEphemeris(const std::vector<std::string>& kernel_paths) {
  std::lock_guard<std::mutex> lock(spice_mutex_);

  // Promote CSPICE errors to RETURN mode so we can surface them as C++
  // exceptions instead of having CSPICE call exit() on us.
  erract_c("SET", 0, const_cast<SpiceChar*>("RETURN"));
  errprt_c("SET", 0, const_cast<SpiceChar*>("NONE"));

  for (const auto& path : kernel_paths) {
    furnsh_c(path.c_str());
    if (failed_c()) {
      throw_spice_error("SpiceEphemeris: furnsh_c failed for '" + path + "'");
    }
  }
}

SpiceEphemeris::~SpiceEphemeris() {
  std::lock_guard<std::mutex> lock(spice_mutex_);
  // Best-effort kernel release; if kclear_c flags an error we swallow it
  // since we are in a destructor.
  kclear_c();
  if (failed_c()) {
    reset_c();
  }
}

apsis::frames::State<apsis::frames::tags::ICRF>
SpiceEphemeris::state(int body_naif_id,
                      apsis::time::Time<apsis::time::tags::TDB> t) const {
  std::lock_guard<std::mutex> lock(spice_mutex_);

  const double et = tdb_to_et_seconds(t);

  // spkezr_c takes the target name as a string. NAIF supports numeric IDs
  // either by name table lookup or by passing the integer as a decimal
  // string; the latter avoids any naming-table dependence.
  const std::string target = std::to_string(body_naif_id);
  const std::string observer = std::to_string(kObserverSSB);

  SpiceDouble state6[6] = {0.0};
  SpiceDouble lt = 0.0;
  spkezr_c(target.c_str(), et, kSpiceFrame, kAbcorr, observer.c_str(),
           state6, &lt);
  if (failed_c()) {
    throw_spice_error("SpiceEphemeris::state(body=" + target + ")");
  }

  apsis::frames::State<apsis::frames::tags::ICRF> out;
  out.r << state6[0] * kKmToM, state6[1] * kKmToM, state6[2] * kKmToM;
  out.v << state6[3] * kKmToM, state6[4] * kKmToM, state6[5] * kKmToM;
  return out;
}

}  // namespace apsis::ephemeris
