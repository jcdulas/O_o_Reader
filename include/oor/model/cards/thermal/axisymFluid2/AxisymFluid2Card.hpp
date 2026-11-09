#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace oor::model
{

// One CFLUID2 card: a fluid element of an axisymmetric fluid model, a thick disk (a body of revolution) whose two outer
// corners are the rings IDF1 and IDF2.
//
// Card layout: CFLUID2  EID  IDF1  IDF2  RHO  B
// IDF1 and IDF2 are RINGFL IDs below 500000 (the RINGFL card is not typed, so they are not references). RHO, the mass
// density, and B, the bulk modulus (0.0 is an incompressible fluid), are blank when the AXIF card gives the values:
// `std::nullopt`.
struct AxisymFluid2Card
{
    static constexpr std::size_t RING_COUNT = 2;

    base::EntityId id = 0;
    std::array<base::EntityId, RING_COUNT> rings = {};
    std::optional<double> density;
    std::optional<double> bulkModulus;
};

} // namespace oor::model
