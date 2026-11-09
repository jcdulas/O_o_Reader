#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// The temperature at one grid point.
struct NodeTemperature
{
    base::EntityId node = 0;  // Gi
    double temperature = 0.0; // Ti

    [[nodiscard]] bool operator==(const NodeTemperature&) const noexcept = default;
};

// One TEMP card: the temperatures of some grid points.
//
// Card layout: TEMP  SID  G1  T1  G2  T2  G3  T3
// The format reference allows one to three points on a card (the SID is shared by as many TEMP cards as needed). This
// library reads and writes the pairs one after the other from field 2 on, as many as the card has, over continuation
// lines if it has more than three.
struct NodeTemperatureLoadCard
{
    base::EntityId id = 0;
    std::vector<NodeTemperature> points;
};

} // namespace oor::model
