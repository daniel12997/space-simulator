// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §1: thin `using` aliases over Eigen 3.4 templates per ADR-012.
// These aliases form the public domain-typed surface used by the rest of
// Apsis. Internal Eigen idioms are permitted; the alias layer keeps callers
// reading in domain shapes (Vec3, Mat6, Mat36) rather than raw Eigen
// templates. Per ADR-012 §"Consequences", upgrading any single alias to a
// wrapper struct is a localised change that does not break call sites.

#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace apsis::math {

// Position / velocity (3-vec), state (6-vec), and dynamic-size variants for
// covariance / Kalman work. Sizes are spelled with explicit Eigen shapes so
// every consumer sees the same instantiations.
using Vec3 = Eigen::Matrix<double, 3, 1>;
using Vec6 = Eigen::Matrix<double, 6, 1>;
using VecX = Eigen::VectorXd;

// 3x3 rotations / inertia tensors, 6x6 state-transition matrices, and a 3x6
// shape used by `IForceModel::partials` (cols 0..2 = ∂a/∂r, cols 3..5 =
// ∂a/∂v). `Mat66` is an alias for `Mat6` kept for symmetry with `Mat36`.
using Mat3  = Eigen::Matrix<double, 3, 3>;
using Mat6  = Eigen::Matrix<double, 6, 6>;
using Mat36 = Eigen::Matrix<double, 3, 6>;
using Mat66 = Mat6;
using MatX  = Eigen::MatrixXd;

// Quaternion. Eigen's `Quaterniond` already offers the (w, x, y, z) layout
// the rest of the project uses.
using Quat = Eigen::Quaterniond;

}  // namespace apsis::math
