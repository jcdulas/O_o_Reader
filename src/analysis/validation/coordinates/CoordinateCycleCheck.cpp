#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/analysis/validation/coordinates/CoordinateChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <map>
#include <set>

namespace oor::analysis
{

namespace
{

// Coordinate systems that are defined in terms of one another: a CORD2R with RID of a system that has the first as its
// RID, or a CORD1R whose grid points are in a system that is defined by those grid points. The positions that depend on
// them cannot be computed.
class CoordinateCycleCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "coord.cycle", base::Severity::Error, "coordinates",
        "A coordinate system is defined through itself (a loop of RIDs or of grid points)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const SystemDefinitions definitions = readSystemDefinitions(model);
        // The systems each system is defined in terms of.
        std::map<base::EntityId, std::set<base::EntityId>> uses;
        for (const auto& [cid, definition] : definitions)
        {
            std::set<base::EntityId>& targets = uses[cid];
            if (definition.form == SystemDefinition::Form::ThreePoints)
            {
                if (definition.referenceFrameId > 0)
                    targets.insert(definition.referenceFrameId);
                continue;
            }
            for (const base::EntityId node : definition.nodes)
            {
                const std::optional<model::CardLocation> grid = model.locate(base::IdSpace::Node, node);
                if (!grid || !isType<model::NodeTable>(*grid))
                    continue;
                if (const base::EntityId cp =
                        static_cast<const model::NodeTable&>(*model.findTable(grid->table)).positionFrameId(grid->row);
                    cp > 0)
                    targets.insert(cp);
            }
        }
        std::set<base::EntityId> reported;
        for (const auto& entry : uses)
        {
            const base::EntityId cid = entry.first;
            if (!reachesItself(uses, cid) || !reported.insert(cid).second)
                continue;
            if (const std::optional<model::CardLocation> where = model.locate(base::IdSpace::Coordinate, cid))
                sink.reportCard(
                    *where,
                    definitions.at(cid).form == SystemDefinition::Form::ThreePoints ? "referenceFrameId" : "originNode",
                    "coordinate system " + std::to_string(cid) + " is defined through itself",
                    "break the loop: define one of the systems in the basic system");
        }
    }

private:
    static bool reachesItself(const std::map<base::EntityId, std::set<base::EntityId>>& uses, base::EntityId start)
    {
        std::set<base::EntityId> seen;
        std::vector<base::EntityId> pending(uses.at(start).begin(), uses.at(start).end());
        while (!pending.empty())
        {
            const base::EntityId next = pending.back();
            pending.pop_back();
            if (next == start)
                return true;
            const auto found = uses.find(next);
            if (found == uses.end() || !seen.insert(next).second)
                continue;
            pending.insert(pending.end(), found->second.begin(), found->second.end());
        }
        return false;
    }
};

} // namespace

void addCoordinateCycleCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CoordinateCycleCheck>());
}

} // namespace oor::analysis
