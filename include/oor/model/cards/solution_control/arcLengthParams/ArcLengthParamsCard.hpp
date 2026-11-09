#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// The constraint of the arc-length method of an NLPCI card.
enum class ArcLengthType : std::uint8_t
{
    Cris, // CRIS: Crisfield (the default)
    Riks, // RIKS
    Mriks // MRIKS: modified Riks
};

// One NLPCI card: the arc-length method of a nonlinear static analysis (SOL 106 and 400). Its ID is the ID of the
// NLPARM card it goes with (the NLPARM command of Case Control selects both).
//
// Card layout: NLPCI  ID  TYPE  MINALR  MAXALR  SCALE  (blank)  DESITER  MXINC
struct ArcLengthParamsCard
{
    base::EntityId id = 0;
    ArcLengthType type = ArcLengthType::Cris;
    double minArcLengthRatio = 0.25;   // the smallest ratio between two arc lengths
    double maxArcLengthRatio = 4.0;    // the largest ratio
    double scaleFactor = 0.0;          // the scale factor of the load in the constraint (SOL 106)
    std::int32_t designIteration = 12; // the iterations wanted for convergence
    std::int32_t maxIncrements = 20;   // the largest number of controlled increments of a subcase
};

} // namespace oor::model
