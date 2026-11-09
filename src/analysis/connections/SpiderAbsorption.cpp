#include "oor/analysis/connections/SpiderAbsorption.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// Takes the spider off the outside entities of a boundary node.
void removeSpider(BoundaryNode& node, base::EntityId eid)
{
    if (node.connectionCount > 0)
        --node.connectionCount;
    const auto found = std::find(node.connectionElements.begin(), node.connectionElements.end(), eid);
    if (found != node.connectionElements.end())
        node.connectionElements.erase(found);
}

} // namespace

void SpiderAbsorption::apply(std::vector<Boundary>& boundaries, const std::vector<Spider>& spiders,
                             const ConnectionSharing& sharing, const ComponentModel& components, const HeldTest& isHeld)
{
    for (Boundary& boundary : boundaries)
    {
        for (const Spider& spider : spiders)
        {
            if (spider.unit != boundary.component || !spider.isEligible || (isHeld && isHeld(spider.unit, spider)))
                continue;
            const std::vector<base::EntityId> legs = spider.legNodes();
            std::vector<BoundaryNode> kept;
            kept.reserve(boundary.nodes.size());
            for (BoundaryNode& node : boundary.nodes)
            {
                if (std::binary_search(legs.begin(), legs.end(), node.node) && node.isInterface)
                {
                    removeSpider(node, spider.eid);
                    if (node.connectionCount == 0)
                    {
                        node.isInterface = false;
                        if (!node.isRetained)
                            continue;
                    }
                }
                kept.push_back(std::move(node));
            }
            boundary.nodes = std::move(kept);

            // The reference point stands for the legs.
            auto at = std::lower_bound(boundary.nodes.begin(), boundary.nodes.end(), spider.reference,
                                       [](const BoundaryNode& node, base::EntityId id) { return node.node < id; });
            if (at == boundary.nodes.end() || at->node != spider.reference)
            {
                BoundaryNode reference;
                reference.node = spider.reference;
                reference.isInterface = true;
                for (const std::uint32_t connection : sharing.connectionsAt(spider.reference))
                {
                    if (connection == spider.connection)
                        continue;
                    ++reference.connectionCount;
                    const Connection& other = components.connections()[connection];
                    if (other.kind != ConnectionKind::Mpc && other.kind != ConnectionKind::Contact)
                        reference.connectionElements.push_back(other.id);
                }
                std::sort(reference.connectionElements.begin(), reference.connectionElements.end());
                at = boundary.nodes.insert(at, std::move(reference));
            }
            else if (at->isInterface)
                removeSpider(*at, spider.eid); // a reference point of the unit: the spider no longer acts on it
            at->isInterface = true;
            if (at->spiderElement == 0 || spider.eid < at->spiderElement)
                at->spiderElement = spider.eid;
            at->replacedLegs.insert(at->replacedLegs.end(), legs.begin(), legs.end());
            std::sort(at->replacedLegs.begin(), at->replacedLegs.end());
            at->replacedLegs.erase(std::unique(at->replacedLegs.begin(), at->replacedLegs.end()),
                                   at->replacedLegs.end());
        }
    }
}

} // namespace oor::analysis
