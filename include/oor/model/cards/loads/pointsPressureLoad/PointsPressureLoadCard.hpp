#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One PLOAD card: a uniform pressure on the triangle or quadrilateral defined by three or four grid points.
//
// Card layout: PLOAD  SID  P  G1  G2  G3  G4
// G4 is blank (0 here) for a triangle. The pressure acts in the direction given by the right-hand rule over the points
// in the order of the card (a negative P reverses it).
struct PointsPressureLoadCard
{
    base::EntityId id = 0;
    double pressure = 0.0;
    std::array<base::EntityId, 4> nodes{0, 0, 0, 0}; // G1 to G4; nodes[3] is 0 for a triangle

    [[nodiscard]] bool isTriangle() const noexcept { return nodes[3] == 0; }
};

} // namespace oor::model
