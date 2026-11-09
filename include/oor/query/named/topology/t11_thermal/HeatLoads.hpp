#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/operators/ExecutionContext.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

// One heat load on one target (what a row of `heatLoadsTable` says). NaN is a value that does not apply or that the
// geometry of the model cannot give.
struct HeatLoad
{
    base::EntityId sid = 0;
    std::string card;       // the card type: HeatFluxSurfaceLoad1, HeatFluxNodalLoad2, ..., HeatFluxVectorLoad
    std::string targetType; // surfaceElement, element, node
    base::EntityId target = 0;
    bool targetValid = true;
    double flux = std::numeric_limits<double>::quiet_NaN(); // the magnitude on the card (the mean of the corner fluxes
                                                            // of a QBDY2)
    double extent =
        std::numeric_limits<double>::quiet_NaN(); // the area of the surface element, or the volume of the element
    double power = std::numeric_limits<double>::quiet_NaN(); // the heat per unit time the load puts into the target
    base::EntityId control = 0;
    base::Vec3 direction{
        std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN()}; // QVECT: the unit flux direction in the basic system
};

// The heat loads of the model, in the order of their sets (then of the cards): the loads of the sets in `sets`, or of
// every set when `sets` is not given. Throws what the geometry throws (a surface element on a grid point that does not
// exist).
[[nodiscard]] std::vector<HeatLoad> computeHeatLoads(const std::shared_ptr<model::Model>& model,
                                                     ExecutionContext& context,
                                                     const std::optional<std::vector<base::EntityId>>& sets);

} // namespace oor::query
