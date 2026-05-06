// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §3: ICRF <-> ITRS via the CEO pipeline (ADR-001).
//
// Composition (forward, ICRF -> ITRS):
//
//   r_itrs = W * R3(ERA) * Q^T * r_icrf
//
// where:
//   * Q = iauC2ixys(X, Y, s)      — celestial-to-intermediate (CIRS) at TT
//   * R3(ERA) = rotation by Earth Rotation Angle about +z, ERA = iauEra00(UT1)
//   * W = iauPom00(xp, yp, sp)    — polar motion (sp = TIO locator from
//                                     iauSp00, which is small and ignored
//                                     at our tolerances; we set sp = 0).
//
// Velocity transform: the time-varying rotation is dominated by ERA's
// uniform spin at omega_E about z. The contributions of dQ/dt (precession-
// nutation) and dW/dt (polar motion) are both << 1e-9 rad/s and negligible
// at the regression tolerances declared in the Phase 1 plan. We therefore
// model dR/dt = R * [omega_e]_z (cross product), which is the standard
// approximation in flight-dynamics-grade pipelines for short arcs.

#include <atomic>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "apsis/frames/transform.h"
#include "apsis/math/types.h"
#include "apsis/time/convert.h"

extern "C" {
#include "sofa.h"
}

namespace apsis::frames {
namespace {

// Earth's nominal rotation rate (IERS 2010 §1.2). Using a fixed value
// rather than computing it from the LOD is sufficient at our tolerances
// (the LOD-derived correction is sub-microradian per second, contributing
// micrometre-per-second velocity noise — well below the 5 cm/s ISS budget).
constexpr double kOmegaEarth = 7.2921151467064e-5;  // rad/s

std::atomic<double> g_polar_motion_xp{0.0};
std::atomic<double> g_polar_motion_yp{0.0};

// Convert a SOFA 3x3 row-major C array into an Eigen Mat3.
apsis::math::Mat3 sofa_to_eigen(const double m[3][3]) {
  apsis::math::Mat3 e;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      e(i, j) = m[i][j];
    }
  }
  return e;
}

// Compose the full ICRF -> ITRS rotation matrix at TT epoch t.
// Pulled into a helper so both forward and inverse transforms share the
// SOFA bookkeeping.
apsis::math::Mat3 build_icrf_to_itrs(apsis::time::Time<apsis::time::tags::TT> tt) {
  // CIP X, Y and CEO position s at TT (full IAU 2006/2000A, periodic terms
  // included). iauXys06a is the canonical one-call entry point.
  double X{}, Y{}, s{};
  iauXys06a(tt.jd1(), tt.jd2(), &X, &Y, &s);

  // Celestial -> CIRS rotation matrix Q.
  double q_c[3][3];
  iauC2ixys(X, Y, s, q_c);
  const apsis::math::Mat3 Q = sofa_to_eigen(q_c);

  // Earth rotation angle from UT1.
  const auto ut1 = apsis::time::convert<apsis::time::tags::UT1>(tt);
  const double era = iauEra00(ut1.jd1(), ut1.jd2());

  apsis::math::Mat3 R_era = apsis::math::Mat3::Identity();
  const double c = std::cos(era);
  const double sn = std::sin(era);
  // R3(ERA): rotation about +z. The CIO/TIO convention uses the IAU R3(theta)
  // form whose matrix is [[c, s, 0], [-s, c, 0], [0, 0, 1]].
  R_era(0, 0) = c;
  R_era(0, 1) = sn;
  R_era(1, 0) = -sn;
  R_era(1, 1) = c;

  // Polar motion W = iauPom00(xp, yp, sp). sp (TIO locator) is small enough
  // to ignore at v1 tolerances per ADR-001 §"polar motion x_p, y_p".
  const double xp = g_polar_motion_xp.load(std::memory_order_relaxed);
  const double yp = g_polar_motion_yp.load(std::memory_order_relaxed);
  double w_c[3][3];
  iauPom00(xp, yp, /*sp=*/0.0, w_c);
  const apsis::math::Mat3 W = sofa_to_eigen(w_c);

  // Q is celestial->CIRS already; ICRF->ITRS = W * R_era * Q.
  return W * R_era * Q;
}

}  // namespace

void set_default_polar_motion(double xp_rad, double yp_rad) noexcept {
  g_polar_motion_xp.store(xp_rad, std::memory_order_relaxed);
  g_polar_motion_yp.store(yp_rad, std::memory_order_relaxed);
}

double default_polar_motion_xp() noexcept {
  return g_polar_motion_xp.load(std::memory_order_relaxed);
}

double default_polar_motion_yp() noexcept {
  return g_polar_motion_yp.load(std::memory_order_relaxed);
}

template <>
State<tags::ITRS> transform<tags::ITRS, tags::ICRF>(State<tags::ICRF> x,
                                                    apsis::time::Time<apsis::time::tags::TT> tt) {
  const apsis::math::Mat3 R = build_icrf_to_itrs(tt);
  // Velocity: r_itrs_dot = R * r_icrf_dot - omega x r_itrs.
  // Equivalently, in fixed frame: r_itrs_dot = R * (r_icrf_dot - omega_icrf x r_icrf).
  // We compute it via the second form: omega_icrf is omega along the rotation
  // axis projected back to ICRF; in the CEO pipeline that axis is essentially
  // +z in the intermediate frame, which after Q is small-angle from ICRF +z.
  // Use the simpler form r_itrs_dot = R*v_icrf - omega_z x r_itrs in ITRS.
  State<tags::ITRS> y;
  y.r = R * x.r;
  const apsis::math::Vec3 v_rot = R * x.v;
  const apsis::math::Vec3 omega_z(0.0, 0.0, kOmegaEarth);
  y.v = v_rot - omega_z.cross(y.r);
  return y;
}

template <>
State<tags::ICRF> transform<tags::ICRF, tags::ITRS>(State<tags::ITRS> x,
                                                    apsis::time::Time<apsis::time::tags::TT> tt) {
  const apsis::math::Mat3 Rt = build_icrf_to_itrs(tt).transpose();
  State<tags::ICRF> y;
  y.r = Rt * x.r;
  const apsis::math::Vec3 omega_z(0.0, 0.0, kOmegaEarth);
  // Inverse: v_icrf = R^T * (v_itrs + omega_z x r_itrs).
  y.v = Rt * (x.v + omega_z.cross(x.r));
  return y;
}

}  // namespace apsis::frames
