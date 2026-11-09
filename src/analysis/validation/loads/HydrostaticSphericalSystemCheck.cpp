#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/HydrostaticSupport.hpp"
#include "oor/model/cards/loads/hydrostaticCavityLoad/HydrostaticCavityLoadTable.hpp"
#include "oor/model/cards/loads/hydrostaticLoad/HydrostaticLoadTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The z direction of a HYDROC or a HYDROS is measured in the coordinate system of the CID/GRD field, or in the CD
// system of the grid point it names when it is negative; the format says that a spherical system "will result in a
// fatal error". A system that does not exist is `ref.unresolved`; the basic system (0) is rectangular.
class HydrostaticSphericalSystemCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.hydrostatic-spherical-system", base::Severity::Error, "loads",
                                    "A hydrostatic load measures its z direction in a spherical coordinate system."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const SystemDefinitions systems = readSystemDefinitions(model);
        forEachCard<model::HydrostaticCavityLoadTable>(
            model, [&](const model::HydrostaticCavityLoadCard& card, model::CardLocation where)
            { look(model, systems, sink, where, card.frameOrNode); });
        forEachCard<model::HydrostaticLoadTable>(model,
                                                 [&](const model::HydrostaticLoadCard& card, model::CardLocation where)
                                                 { look(model, systems, sink, where, card.frameOrNode); });
    }

private:
    static void look(const model::Model& model, const SystemDefinitions& systems, FindingSink& sink,
                     model::CardLocation where, base::EntityId frameOrNode)
    {
        base::EntityId system = model::hydrostaticSystemOf(frameOrNode);
        std::string owner = "the coordinate system " + std::to_string(system);
        if (const base::EntityId grid = model::hydrostaticGridOf(frameOrNode); grid != 0)
        {
            const auto location = model.locate(base::IdSpace::Node, grid);
            if (!location || !isType<model::NodeTable>(*location))
                return; // a point that is not a grid, or that does not exist: another check says so
            system = cardAt<model::NodeTable>(model, *location).displacementFrameId;
            owner = "the " + std::string("displacementFrameId") + " system " + std::to_string(system) +
                    " of grid point " + std::to_string(grid);
        }
        const auto found = systems.find(system);
        if (found == systems.end() || found->second.kind != CoordinateKind::Spherical)
            return;
        sink.reportCard(where, "frameId/nodeAlt",
                        owner + " is a " + std::string(found->second.cardName) + ", a spherical system",
                        "name a rectangular or cylindrical system");
    }
};

} // namespace

void addHydrostaticSphericalSystemCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<HydrostaticSphericalSystemCheck>());
}

} // namespace oor::analysis
