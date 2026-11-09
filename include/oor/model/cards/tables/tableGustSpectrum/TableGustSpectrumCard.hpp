#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One TABRNDG card: the power spectral density of a gust for an aeroelastic response analysis (referenced by the
// RANDPS cards).
//
// Card layout: TABRNDG  TID  TYPE  L/U  WG
// TYPE is 1 for the von Karman model and 2 for the Dryden one; L/U is the scale of the turbulence divided by the
// velocity (a time), WG the root-mean-square gust velocity. The density is then
//   Sq(w) = 2 WG^2 (L/U) (1 + 2 (p + 1) k^2 (L/U)^2 w^2) / (1 + k^2 (L/U)^2 w^2)^(p + 3/2)
// with p = 1/3 and k = 1.339 for the von Karman model and p = 1/2 and k = 1.0 for the Dryden one.
struct TableGustSpectrumCard
{
    static constexpr std::int32_t VON_KARMAN = 1;
    static constexpr std::int32_t DRYDEN = 2;

    base::EntityId id = 0;
    std::int32_t kind = VON_KARMAN;
    double turbulenceScaleRatio = 0.0; // L/U
    double gustVelocity = 0.0;
};

} // namespace oor::model
