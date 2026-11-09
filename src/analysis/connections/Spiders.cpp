#include "oor/analysis/connections/Spiders.hpp"

#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/analysis/connections/RigidElementTerms.hpp"
#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidInterpolation/RigidInterpolationTable.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <utility>

namespace oor::analysis
{

std::string_view spiderReasonName(SpiderReason reason) noexcept
{
    switch (reason)
    {
    case SpiderReason::None:
        return "";
    case SpiderReason::Um:
        return "um";
    case SpiderReason::ReferenceIsLeg:
        return "referenceIsLeg";
    case SpiderReason::ReferenceDependentElsewhere:
        return "referenceDependentElsewhere";
    case SpiderReason::LegsOnSeveralUnits:
        return "legsOnSeveralUnits";
    }
    return "";
}

std::vector<base::EntityId> Spider::legNodes() const
{
    std::vector<base::EntityId> nodes;
    for (const SpiderLeg& leg : legs)
        nodes.push_back(leg.node);
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

namespace
{

// The connections each grid point is a dependent point of: the dependent terms of the rigid elements and the first term
// of each MPC equation (format reference MPC: the first degree of freedom of the equation is the dependent one).
std::multimap<base::EntityId, std::uint32_t> dependentPoints(const model::Model& model,
                                                             const ComponentModel& components)
{
    std::multimap<base::EntityId, std::uint32_t> dependents;
    const model::MultiPointConstrTable* mpcs = model.findTable<model::MultiPointConstrTable>();
    for (std::size_t index = 0; index < components.connections().size(); ++index)
    {
        const Connection& connection = components.connections()[index];
        if (connection.kind == ConnectionKind::Rigid)
        {
            for (const RigidTerm& term : RigidElementTerms::of(model, connection.card))
            {
                if (!term.isIndependent)
                    dependents.emplace(term.node, static_cast<std::uint32_t>(index));
            }
        }
        else if (connection.kind == ConnectionKind::Mpc && mpcs != nullptr)
        {
            const model::MultiPointConstrCard card = mpcs->card(connection.card.row);
            if (!card.terms.empty())
                dependents.emplace(card.terms.front().node, static_cast<std::uint32_t>(index));
        }
    }
    return dependents;
}

} // namespace

std::vector<Spider> Spiders::find(const model::Model& model, const ComponentModel& components,
                                  const ConnectionSharing& sharing, const UnitMembership& units, base::ThreadPool* pool)
{
    const std::uint16_t star = model::typeIndexOf<model::RigidBodyStarTable>();
    const std::uint16_t interpolation = model::typeIndexOf<model::RigidInterpolationTable>();
    const std::multimap<base::EntityId, std::uint32_t> dependents = dependentPoints(model, components);
    std::vector<Spider> spiders;
    std::vector<std::vector<model::CardLocation>> cards;
    std::vector<std::int32_t> held;
    std::vector<std::int32_t> here;
    for (std::size_t index = 0; index < components.connections().size(); ++index)
    {
        const Connection& connection = components.connections()[index];
        if (connection.kind != ConnectionKind::Rigid ||
            (connection.card.table != star && connection.card.table != interpolation))
            continue;
        Spider spider;
        spider.card = connection.card;
        spider.eid = connection.id;
        spider.isInterpolation = connection.card.table == interpolation;
        spider.connection = static_cast<std::uint32_t>(index);
        bool hasUm = false;
        for (const RigidTerm& term : RigidElementTerms::of(model, connection.card))
        {
            if (spider.isInterpolation)
            {
                if (term.isIndependent)
                    spider.legs.push_back(SpiderLeg{term.node, term.components, term.weight});
                else if (spider.reference == 0)
                {
                    spider.reference = term.node; // REFGRID comes first among the dependent terms
                    spider.referenceComponents = term.components;
                }
                else
                    hasUm = true;
            }
            else if (term.isIndependent)
            {
                spider.reference = term.node;
                spider.referenceComponents = term.components;
            }
            else
                spider.legs.push_back(SpiderLeg{term.node, term.components});
        }
        const std::vector<base::EntityId> legNodes = spider.legNodes();
        if (hasUm)
            spider.reason = SpiderReason::Um;
        else if (std::binary_search(legNodes.begin(), legNodes.end(), spider.reference))
            spider.reason = SpiderReason::ReferenceIsLeg;
        else
        {
            const auto [first, last] = dependents.equal_range(spider.reference);
            if (std::any_of(first, last, [&](const auto& entry) { return entry.second != index; }))
                spider.reason = SpiderReason::ReferenceDependentElsewhere;
        }
        // The units that hold every leg.
        held.clear();
        for (std::size_t leg = 0; leg < legNodes.size(); ++leg)
        {
            here.clear();
            units.unitsOfNode(legNodes[leg], here);
            std::sort(here.begin(), here.end());
            if (leg == 0)
                held = here;
            else
            {
                std::vector<std::int32_t> both;
                std::set_intersection(held.begin(), held.end(), here.begin(), here.end(), std::back_inserter(both));
                held = std::move(both);
            }
        }
        if (legNodes.empty())
            held.clear();
        spider.blockingConnections = sharing.othersAt(legNodes, spider.connection, &spider.blockingNodes);
        const std::size_t first = spiders.size();
        if (held.empty())
        {
            if (spider.reason == SpiderReason::None)
                spider.reason = SpiderReason::LegsOnSeveralUnits;
            spiders.push_back(spider);
        }
        else
        {
            for (const std::int32_t unit : held)
            {
                spider.unit = unit;
                spiders.push_back(spider);
            }
        }
        for (std::size_t position = first; position < spiders.size(); ++position)
        {
            Spider& entry = spiders[position];
            entry.isSpider = entry.reason == SpiderReason::None;
            entry.isEligible = entry.isSpider && entry.blockingConnections.empty();
        }
        cards.push_back({connection.card});
    }
    // The stamps, one per card.
    const std::vector<std::uint64_t> stamps = RevisionStamp::computeFor(model, cards, pool);
    std::map<std::pair<std::uint16_t, std::uint32_t>, std::uint64_t> stampOf;
    for (std::size_t index = 0; index < cards.size(); ++index)
        stampOf[{cards[index].front().table, cards[index].front().row}] = stamps[index];
    for (Spider& spider : spiders)
        spider.revision = stampOf[{spider.card.table, spider.card.row}];
    std::stable_sort(spiders.begin(), spiders.end(),
                     [](const Spider& left, const Spider& right)
                     {
                         if (left.eid != right.eid)
                             return left.eid < right.eid;
                         return left.unit < right.unit;
                     });
    return spiders;
}

} // namespace oor::analysis
