#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/analysis/validation/coordinates/CoordinateChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

namespace oor::analysis
{

namespace
{

// A grid point given in a cylindrical or spherical system at R = 0: on the axis (or at the origin) the angles mean
// nothing, and the directions of its displacement system are taken at an arbitrary angle.
class NodeOnAxisCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"coord.node-on-axis", base::Severity::Info, "coordinates",
                                    "A grid point is at R = 0 of a cylindrical or spherical coordinate system.", true};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const SystemDefinitions definitions = readSystemDefinitions(context.model());
        forEachCard<model::NodeTable>(
            context.model(),
            [&](const model::NodeCard& grid, model::CardLocation where)
            {
                const auto found = definitions.find(grid.positionFrameId);
                if (found == definitions.end() || found->second.kind == CoordinateKind::Rectangular)
                    return;
                if (grid.xyz.x == 0.0)
                    sink.reportCard(
                        where, "positionFrameId",
                        "R is zero in coordinate system " + std::to_string(grid.positionFrameId) + ": on the axis",
                        "the angles of a point on the axis are arbitrary");
            });
    }
};

} // namespace

void addNodeOnAxisCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<NodeOnAxisCheck>());
}

} // namespace oor::analysis
