#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/references/ReferenceChecks.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

namespace oor::analysis
{

namespace
{

// A grid point that no card names: no element, no load, no constraint, no coordinate system. (A grid point that sits
// strictly inside a THRU range of a card is not seen as named: the card only names the two ends.)
class OrphanNodeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"ref.orphan-node", base::Severity::Info, "references",
                                    "A grid point that no card refers to.", true};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const ReferenceCensus& census = context.references();
        forEachCard<model::NodeTable>(context.model(),
                                      [&](const model::NodeCard& grid, model::CardLocation location)
                                      {
                                          if (!census.nodes.contains(grid.id))
                                              sink.reportCard(location, "id", "no card refers to this grid point",
                                                              "delete the grid point if it is not needed");
                                      });
    }
};

} // namespace

void addOrphanNodeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<OrphanNodeCheck>());
}

} // namespace oor::analysis
