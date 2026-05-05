// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §3: `State<Frame>` is the canonical (position, velocity) record
// tagged with its reference frame at the type level. Cross-frame mixing
// is a compile error; the only path between frames is the explicit
// `transform<>` function.

#pragma once

#include "apsis/frames/frame_tags.h"
#include "apsis/math/types.h"

namespace apsis::frames {

template <class Frame>
struct State {
  using frame_type = Frame;

  // Position [m] expressed in the parameter frame.
  apsis::math::Vec3 r{apsis::math::Vec3::Zero()};
  // Velocity [m/s] expressed in the parameter frame.
  apsis::math::Vec3 v{apsis::math::Vec3::Zero()};
};

}  // namespace apsis::frames
