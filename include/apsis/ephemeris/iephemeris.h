// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §4: `IEphemeris` interface — planetary ephemeris seam.
// Returns a body's state in ICRF wrt the Solar-System Barycentre at a TDB
// epoch (SPICE's native time scale).

#pragma once

#include "apsis/frames/frame_tags.h"
#include "apsis/frames/state.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace apsis::ephemeris {

class IEphemeris {
 public:
  IEphemeris() = default;
  IEphemeris(const IEphemeris&) = delete;
  IEphemeris& operator=(const IEphemeris&) = delete;
  IEphemeris(IEphemeris&&) = delete;
  IEphemeris& operator=(IEphemeris&&) = delete;
  virtual ~IEphemeris() = default;

  // NAIF body ID (e.g. 10 = Sun, 301 = Moon, 399 = Earth).
  // Output is in ICRF (J2000-equivalent for SPICE's "J2000" frame,
  // which is what NAIF's planetary kernels use), wrt the
  // Solar-System Barycentre, in metres and metres/second.
  virtual apsis::frames::State<apsis::frames::tags::ICRF>
  state(int body_naif_id, apsis::time::Time<apsis::time::tags::TDB> t) const = 0;
};

}  // namespace apsis::ephemeris
