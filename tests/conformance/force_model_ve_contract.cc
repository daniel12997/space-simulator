// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-PHY-016, REQ-PHY-020
//
// Phase-1 §5 conformance gate: every IForceModel adapter that **claims
// analytical partials** must agree with a central-difference oracle of
// `acceleration` to within an adapter-specific tolerance.
//
// Adapters under test (those with `kAnalyticalPartials == true`):
//   * PointMass        — tolerance 1e-6 (closed-form analytical partials)
//   * ThirdBody        — tolerance 1e-6 (closed-form analytical partials,
//                        Phase 1 — see src/force/third_body.cc)
//
// Adapters **excluded** from this gate (per ADR-009 Phase 1 Implementation
// Note):
//   * SphericalHarmonic — `partials()` is itself a finite-difference
//                         evaluation in Phase 1, pending the Phase 7
//                         Pines analytical-gradient upgrade. Comparing
//                         FD-against-FD would be a tautology; the
//                         adapter declares `kAnalyticalPartials = false`
//                         and is excluded by the loop below.
//
// Per the plan: 32 representative (t, x) points; h = 10 m for position
// columns (chosen to be independent of any adapter's internal h step),
// no velocity perturbation (no velocity-dependent forces in Phase 1 —
// drag/SRP deferred). The plan's force-partials grid is implemented as
// a deterministic LCG over the configuration space below.

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <random>
#include <vector>

#include "apsis/ephemeris/iephemeris.h"
#include "apsis/force/iforce_model.h"
#include "apsis/force/point_mass.h"
#include "apsis/force/spherical_harmonic.h"
#include "apsis/force/third_body.h"

namespace af = apsis::force;
namespace at = apsis::time;
namespace afr = apsis::frames;
namespace ae = apsis::ephemeris;

namespace {

constexpr double kMuEarth = 3.986004418e14;
constexpr double kMuSun = 1.32712440018e20;
constexpr double kAU = 1.495978707e11;

class StubEphem : public ae::IEphemeris {
 public:
  afr::State<afr::tags::ICRF> state(int body, at::Time<at::tags::TDB>) const override {
    afr::State<afr::tags::ICRF> s;
    if (body == 10)
      s.r = apsis::math::Vec3(kAU, 0.0, 0.0);  // Sun
    return s;                                  // others (central-body 399) at origin
  }
};

struct Sample {
  at::Time<at::tags::TT> t;
  afr::State<afr::tags::ICRF> x;
};

std::vector<Sample> make_samples() {
  std::vector<Sample> samples;
  samples.reserve(32);
  // Deterministic LCG: 32 LEO-altitude states, varied across the unit
  // sphere with magnitudes 6.6e6 - 8.0e6 m.
  std::mt19937 rng(0x4151534fu);  // "ASIS" + nybble
  std::uniform_real_distribution<double> mag(6.6e6, 8.0e6);
  std::uniform_real_distribution<double> angle(0.0, 2.0 * M_PI);
  for (int i = 0; i < 32; ++i) {
    Sample s;
    s.t = at::Time<at::tags::TT>{2460676.5 + 30.0 * i, 0.0};
    const double r = mag(rng);
    const double theta = angle(rng);
    const double phi = std::acos(2.0 * std::uniform_real_distribution<double>(0.0, 1.0)(rng) - 1.0);
    s.x.r << r * std::sin(phi) * std::cos(theta), r * std::sin(phi) * std::sin(theta),
        r * std::cos(phi);
    s.x.v << 0.0, 7.5e3, 0.0;  // velocity unused in Phase 1 (no drag)
    samples.push_back(s);
  }
  return samples;
}

apsis::math::Mat36 oracle_partials(const af::IForceModel& model, const Sample& s, double h_pos) {
  apsis::math::Mat36 J = apsis::math::Mat36::Zero();
  for (int i = 0; i < 3; ++i) {
    auto xp = s.x;
    auto xm = s.x;
    xp.r[i] += h_pos;
    xm.r[i] -= h_pos;
    const auto ap = model.acceleration(s.t, xp);
    const auto am = model.acceleration(s.t, xm);
    J.col(i) = (ap - am) / (2.0 * h_pos);
  }
  // Velocity columns: Phase 1 has no velocity-dependent forces, so the
  // oracle returns zeros and the adapter must too.
  return J;
}

// Adapter-agnostic conformance check: max relative error in the position-
// partials block (cols 0..2) must be below `rel_tol`. Velocity columns
// must be exactly zero (Phase 1).
void check_adapter(const char* name, const af::IForceModel& model, double rel_tol_pos,
                   double abs_tol_vel) {
  const auto samples = make_samples();
  for (size_t i = 0; i < samples.size(); ++i) {
    const auto& s = samples[i];
    const auto J_analytic = model.partials(s.t, s.x);
    const auto J_oracle = oracle_partials(model, s, /*h=*/10.0);

    // Position block. Parenthesise block<3,3>(...) so the macro doesn't
    // see the template comma as a macro argument separator.
    const apsis::math::Mat3 J_an_pos = J_analytic.block<3, 3>(0, 0);
    const apsis::math::Mat3 J_or_pos = J_oracle.block<3, 3>(0, 0);
    const double scale = std::max(J_or_pos.norm(), 1e-12);
    const double err = (J_an_pos - J_or_pos).norm();
    EXPECT_LT(err / scale, rel_tol_pos) << name << " sample " << i << " analytic=\n"
                                        << J_an_pos << "\noracle=\n"
                                        << J_or_pos;

    // Velocity block (Phase 1 = zero).
    const apsis::math::Mat3 J_an_vel = J_analytic.block<3, 3>(0, 3);
    EXPECT_LT(J_an_vel.norm(), abs_tol_vel)
        << name << " sample " << i << " velocity columns must be zero in Phase 1";
  }
}

TEST(ForceModelVE, PointMass) {
  static_assert(af::PointMass::kAnalyticalPartials,
                "PointMass must declare analytical partials per ADR-009");
  af::PointMass pm(kMuEarth);
  check_adapter("PointMass", pm, /*rel_tol=*/1e-6, /*abs_tol_vel=*/1e-15);
}

TEST(ForceModelVE, ThirdBodySun) {
  static_assert(af::ThirdBody::kAnalyticalPartials,
                "ThirdBody must declare analytical partials per ADR-009");
  StubEphem ephem;
  af::ThirdBody tb(&ephem, /*central=*/399, /*third=*/10, kMuSun);
  check_adapter("ThirdBody(Sun)", tb, /*rel_tol=*/1e-6, /*abs_tol_vel=*/1e-15);
}

// SphericalHarmonic is intentionally excluded — it declares
// `kAnalyticalPartials = false` because its partials() is itself a
// finite-difference evaluation pending the Phase 7 Pines analytical
// gradient. A static_assert here guards the disclosure: if anyone flips
// the flag without implementing the analytical gradient, this test will
// fail to compile and force them to add a real conformance case below.
static_assert(!af::SphericalHarmonic::kAnalyticalPartials,
              "SphericalHarmonic should declare kAnalyticalPartials=false "
              "until the Phase 7 Pines analytical gradient ships; flipping "
              "the flag must be paired with adding a conformance case here.");

}  // namespace
