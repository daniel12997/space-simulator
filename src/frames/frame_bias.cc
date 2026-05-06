// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §3: ICRF <-> J2000 (frame bias only). The bias is a constant
// ~17 mas rotation; we obtain it by evaluating SOFA's iauPmat06 at the J2000
// epoch (jd1 = DJ00, jd2 = 0). At that epoch the precession-nutation
// polynomial parts vanish, leaving the pure bias. ICRF <-> GCRS is identity
// in v1 (handled by the same file as a small adjacent specialisation).

#include <array>

#include <Eigen/Core>

#include "apsis/frames/transform.h"
#include "apsis/math/types.h"

extern "C" {
#include "sofa.h"
#include "sofam.h"  // DJ00 (J2000 reference epoch in JD)
}

namespace apsis::frames {
namespace {

// Cached bias matrix (B such that r_icrf = B^T * r_j2000, equivalently
// r_j2000 = B * r_icrf — sign convention follows SOFA's iauBp06: the
// returned `rb` is the bias from GCRF to mean-of-J2000-equator).
// SOFA's iauBp06 takes `double[3][3]` for each output matrix. We re-declare
// the SOFA entry point we need with a flat `double*` signature to avoid the
// C-array parameter type at the C++ call site; the linker resolves it to the
// same SOFA symbol because the C ABI is identical for passing a contiguous
// 9-double row-major matrix.
extern "C" void sofa_bp06_flat(double date1, double date2, double* rb, double* rp,
                               double* rbp) asm("iauBp06");

apsis::math::Mat3 build_bias_matrix() {
  // iauBp06 returns rb (bias), rp (precession), rbp (combined). At J2000
  // (DJ00, 0) precession is the identity, so rb == rbp; either is fine for
  // our caching purposes. We use iauBp06 directly so the operation is
  // unambiguous regardless of any future SOFA convention drift in iauPmat06.
  std::array<double, 9> rb{};
  std::array<double, 9> rp{};
  std::array<double, 9> rbp{};
  sofa_bp06_flat(DJ00, 0.0, rb.data(), rp.data(), rbp.data());
  apsis::math::Mat3 b;
  for (std::size_t i = 0; i < 3; ++i) {
    for (std::size_t j = 0; j < 3; ++j) {
      b(static_cast<int>(i), static_cast<int>(j)) = rb.at(i * 3 + j);
    }
  }
  return b;
}

const apsis::math::Mat3& bias_icrf_to_j2000() {
  static const apsis::math::Mat3 kB = build_bias_matrix();
  return kB;
}

}  // namespace

template <>
State<tags::J2000> transform<tags::J2000, tags::ICRF>(const State<tags::ICRF>& x,
                                                      apsis::time::Time<apsis::time::tags::TT>) {
  const apsis::math::Mat3& b = bias_icrf_to_j2000();
  State<tags::J2000> y;
  y.r = b * x.r;
  y.v = b * x.v;  // bias is constant -> velocity transforms as a rotation
  return y;
}

template <>
State<tags::ICRF> transform<tags::ICRF, tags::J2000>(const State<tags::J2000>& x,
                                                     apsis::time::Time<apsis::time::tags::TT>) {
  const apsis::math::Mat3 kBt = bias_icrf_to_j2000().transpose();
  State<tags::ICRF> y;
  y.r = kBt * x.r;
  y.v = kBt * x.v;
  return y;
}

// ICRF <-> GCRS: identity in v1. Defined here rather than in icrs_itrs.cc to
// keep all "non-rotating identity-grade" specialisations in one file.
template <>
State<tags::GCRS> transform<tags::GCRS, tags::ICRF>(const State<tags::ICRF>& x,
                                                    apsis::time::Time<apsis::time::tags::TT>) {
  return State<tags::GCRS>{x.r, x.v};
}

template <>
State<tags::ICRF> transform<tags::ICRF, tags::GCRS>(const State<tags::GCRS>& x,
                                                    apsis::time::Time<apsis::time::tags::TT>) {
  return State<tags::ICRF>{x.r, x.v};
}

}  // namespace apsis::frames
