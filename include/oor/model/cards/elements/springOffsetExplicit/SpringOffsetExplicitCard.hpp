#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CSPR card: a spring with offsets, for the SOL 700 explicit solver only.
//
// Card layout:
//   CSPR  EID  PID  G1  G2
//
// PID names a PSPRMAT, a property card: the reference is a property reference and resolves. All
// four fields are required positive integers. The format reference says nothing about the two grid points being
// different, so the card does not require it.
struct SpringOffsetExplicitCard
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, 2> nodes = {}; // G1 and G2
};

} // namespace oor::model
