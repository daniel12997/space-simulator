// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: Coefficient table for the Dormand-Prince 5(4) embedded RK
// method (Hairer-Norsett-Wanner "Solving Ordinary Differential Equations I"
// 2nd ed., Table 5.1). This is the integrator that ships behind the
// IIntegrator seam in Phase 1; the full DP8(5,3) table lands in Phase 7.

#pragma once

#include <array>

namespace apsis::integrate::dp54 {

// 7 stages (the FSAL formulation: stage 7 of step n = stage 1 of step n+1).
inline constexpr int kStages = 7;

// c[i] — abscissae (time fractions of step).
inline constexpr std::array<double, 7> kC = {
    0.0,
    1.0 / 5.0,
    3.0 / 10.0,
    4.0 / 5.0,
    8.0 / 9.0,
    1.0,
    1.0,
};

// a[i][j] — Butcher matrix (lower-triangular). a[0] unused.
inline constexpr std::array<std::array<double, 6>, 7> kA = {{
    {0.0,                  0.0,                 0.0,                 0.0,                  0.0,                0.0},
    {1.0 / 5.0,            0.0,                 0.0,                 0.0,                  0.0,                0.0},
    {3.0 / 40.0,           9.0 / 40.0,          0.0,                 0.0,                  0.0,                0.0},
    {44.0 / 45.0,         -56.0 / 15.0,         32.0 / 9.0,          0.0,                  0.0,                0.0},
    {19372.0 / 6561.0,    -25360.0 / 2187.0,    64448.0 / 6561.0,   -212.0 / 729.0,        0.0,                0.0},
    {9017.0 / 3168.0,     -355.0 / 33.0,        46732.0 / 5247.0,    49.0 / 176.0,        -5103.0 / 18656.0,   0.0},
    {35.0 / 384.0,         0.0,                 500.0 / 1113.0,      125.0 / 192.0,       -2187.0 / 6784.0,    11.0 / 84.0},
}};

// b[i] — 5th-order solution weights.
inline constexpr std::array<double, 7> kB5 = {
    35.0 / 384.0, 0.0, 500.0 / 1113.0, 125.0 / 192.0, -2187.0 / 6784.0, 11.0 / 84.0, 0.0,
};

// e[i] — embedded-error weights (b5 - b4).
inline constexpr std::array<double, 7> kE = {
    71.0 / 57600.0,
    0.0,
    -71.0 / 16695.0,
    71.0 / 1920.0,
    -17253.0 / 339200.0,
    22.0 / 525.0,
    -1.0 / 40.0,
};

}  // namespace apsis::integrate::dp54
