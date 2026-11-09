#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace oor::model
{

// One CFLUID3 card: a fluid element of an axisymmetric fluid model, a body of revolution whose outer corners are the
// rings IDF1 to IDF3.
//
// Card layout: CFLUID3  EID  IDF1  IDF2  IDF3  RHO  B
// IDF1 to IDF3 are RINGFL IDs below 500000 (the RINGFL card is not typed, so they are not references). RHO, the mass
// density, and B, the bulk modulus (0.0 is an incompressible fluid), are blank when the AXIF card gives the values:
// `std::nullopt`.
struct AxisymFluid3Card
{
    static constexpr std::size_t RING_COUNT = 3;

    base::EntityId id = 0;
    std::array<base::EntityId, RING_COUNT> rings = {};
    std::optional<double> density;
    std::optional<double> bulkModulus;
};

} // namespace oor::model
