#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One RANDPS card: a power spectral density factor of a random analysis (selected by RANDOM in Case Control; several
// RANDPS and RANDT1 cards share a set ID).
//
// Card layout: RANDPS  SID  J  K  X  Y  TID
// The cross spectrum of the loads of subcases J and K is S(F) = (X + i Y) G(F), where G is the table TID (TABRND1 or
// TABRNDG), or 1.0 when TID is blank. For an auto spectrum (J = K) X must be greater than zero and Y zero.
// A blank X or Y is 0.0 (the format reference gives no default).
struct RandomSpectralDensityCard
{
    base::EntityId id = 0;
    std::int32_t subcaseJ = 0; // the subcase of the excited load set
    std::int32_t subcaseK = 0; // the subcase of the applied load set (K >= J)
    double realPart = 0.0;
    double imaginaryPart = 0.0;
    base::EntityId tableId = 0; // the TABRND1 or TABRNDG that gives G(F); 0 is blank
};

} // namespace oor::model
