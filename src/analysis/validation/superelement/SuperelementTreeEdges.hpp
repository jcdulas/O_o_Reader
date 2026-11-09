#pragma once

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/superelements/superelementTree/SuperelementTreeTable.hpp"

#include <vector>

namespace oor::analysis
{

// One link of the superelement tree: `upstream` is reduced into `downstream` (0 is the residual structure), and the
// SETREE card that says so. The tree of a deck is the links of all its SETREE cards together.
struct SuperelementTreeEdge
{
    base::EntityId upstream;
    base::EntityId downstream;
    model::CardLocation where;
};

// The links of the SETREE cards of a model, in the order of the cards and of the SEUPi on each.
inline std::vector<SuperelementTreeEdge> superelementTreeEdges(const model::Model& model)
{
    std::vector<SuperelementTreeEdge> edges;
    forEachCard<model::SuperelementTreeTable>(
        model,
        [&](const model::SuperelementTreeCard& card, model::CardLocation where)
        {
            for (const base::EntityId upstream : card.upstream)
                edges.push_back(SuperelementTreeEdge{upstream, card.superelementId, where});
        });
    return edges;
}

} // namespace oor::analysis
