#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>

namespace oor::model
{

// One QHBDY card: a uniform heat flux into a set of grid points.
//
// Card layout: QHBDY  SID  FLAG  Q0  AF  G1  G2  G3  G4
//                    G5   G6    G7  G8
// FLAG is the type of face: POINT (1 grid point), LINE or REV (2), AREA3 (3), AREA4 (4), AREA6 (4 to 6) or AREA8 (5 to
// 8). AF, the area factor, may be blank. The grid points need not
// belong to a surface element; `nodes` has them, 0 where a point is absent.
struct HeatFluxNodeSetLoadCard
{
    static constexpr std::size_t NODE_COUNT = 8;

    base::EntityId id = 0;
    std::string faceKind = "AreaQuad4";
    double heatFlux = 0.0;
    std::optional<double> area;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
};

} // namespace oor::model
