#include "oor/analysis/connections/WeldedNodes.hpp"

#include "oor/analysis/connections/RigidElementTerms.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBar/RigidBarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBar1/RigidBar1Table.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyMulti/RigidBodyMultiTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidJoint/RigidJointTable.hpp"
#include "oor/model/cards/rigid_elements/rigidTriaPlate/RigidTriaPlateTable.hpp"
#include "oor/model/cards/rigid_elements/rigidTriaPlate1/RigidTriaPlate1Table.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>
#include <optional>

namespace oor::analysis
{

namespace
{

constexpr std::uint8_t ALL_SIX = 0b111111;

// The components of a field (123456, 13, ...) as bits: component k is bit k - 1.
std::uint8_t componentBits(std::int32_t components) noexcept
{
    std::uint8_t bits = 0;
    for (; components > 0; components /= 10)
    {
        const std::int32_t digit = components % 10;
        if (digit >= 1 && digit <= 6)
            bits = static_cast<std::uint8_t>(bits | (1U << (digit - 1)));
    }
    return bits;
}

std::uint64_t locationKey(model::CardLocation location) noexcept
{
    return (static_cast<std::uint64_t>(location.table) << 32) | location.row;
}

// The rigid elements that tie their grid points rigidly.
bool tiesRigidly(std::uint16_t table) noexcept
{
    using namespace oor::model;
    return table == typeIndexOf<RigidBodyStarTable>() || table == typeIndexOf<RigidBodyMultiTable>() ||
           table == typeIndexOf<RigidBarTable>() || table == typeIndexOf<RigidBar1Table>() ||
           table == typeIndexOf<RigidJointTable>() || table == typeIndexOf<RigidTriaPlateTable>() ||
           table == typeIndexOf<RigidTriaPlate1Table>();
}

// The positions of the grid points in the basic system, by grid point ID.
class NodePositions
{
public:
    explicit NodePositions(const model::Model& model)
        : m_model(model), m_nodeType(model::typeIndexOf<model::NodeTable>())
    {
    }

    // The distance between two grid points, or nothing when one of them is not a placed grid point.
    [[nodiscard]] std::optional<double> distance(base::EntityId first, base::EntityId second)
    {
        fetch();
        const std::array<double, 3>* a = position(first);
        const std::array<double, 3>* b = position(second);
        if (a == nullptr || b == nullptr)
            return std::nullopt;
        const double dx = (*a)[0] - (*b)[0];
        const double dy = (*a)[1] - (*b)[1];
        const double dz = (*a)[2] - (*b)[2];
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

private:
    // The positions, computed the first time a distance is asked: a model without rigid element asks none, and the
    // components of a mesh no longer pay a refresh of every position for its welds.
    void fetch()
    {
        if (m_fetched)
            return;
        m_fetched = true;
        if (m_model.findTable(m_nodeType) == nullptr)
            return;
        try
        {
            m_positions = BasicPositionCache::of(m_model).positions();
        }
        catch (const base::OorError&)
        {
            m_positions = {}; // a broken coordinate system: no grid point can be placed, so none is welded
        }
    }

    [[nodiscard]] const std::array<double, 3>* position(base::EntityId node) const noexcept
    {
        const auto location = m_model.locate(base::IdSpace::Node, node);
        if (!location || location->table != m_nodeType || location->row >= m_positions.size())
            return nullptr;
        return &m_positions[location->row];
    }

    const model::Model& m_model;
    std::uint16_t m_nodeType;
    bool m_fetched = false;
    std::span<const std::array<double, 3>> m_positions;
};

} // namespace

WeldedNodes WeldedNodes::build(const model::Model& model, const ModelEntities& entities,
                               std::span<const base::EntityId> excluded)
{
    WeldedNodes result;
    NodePositions positions(model);
    // The components each pair of coincident grid points is tied in, all elements together, and the elements whose
    // ties are all between coincident grid points.
    std::map<std::pair<base::EntityId, base::EntityId>, std::uint8_t> tied;
    std::vector<std::pair<model::CardLocation, std::vector<std::pair<base::EntityId, base::EntityId>>>> candidates;
    for (const ModelEntity& entity : entities.entities())
    {
        if (entity.isEquation || entity.role != ElementRole::Rigid || !tiesRigidly(entity.card.table))
            continue;
        if (std::binary_search(excluded.begin(), excluded.end(), entity.id))
            continue; // declared a connection: it ties two parts, it does not make them one
        const std::vector<RigidTerm> terms = RigidElementTerms::of(model, entity.card);
        // The grid point that holds each independent component.
        std::array<base::EntityId, 6> holder{};
        for (const RigidTerm& term : terms)
        {
            if (!term.isIndependent)
                continue;
            const std::uint8_t bits = componentBits(term.components);
            for (std::size_t component = 0; component < 6; ++component)
            {
                if ((bits & (1U << component)) != 0 && holder[component] == 0)
                    holder[component] = term.node;
            }
        }
        std::vector<std::pair<base::EntityId, base::EntityId>> pairs;
        bool zeroLength = true;
        for (const RigidTerm& term : terms)
        {
            if (term.isIndependent)
                continue;
            const std::uint8_t bits = componentBits(term.components);
            for (std::size_t component = 0; component < 6; ++component)
            {
                const base::EntityId other = holder[component];
                if ((bits & (1U << component)) == 0 || other == 0 || other == term.node)
                    continue;
                const std::optional<double> distance = positions.distance(term.node, other);
                if (!distance || *distance > WELD_TOLERANCE)
                {
                    zeroLength = false;
                    continue;
                }
                const auto pair = std::minmax(term.node, other);
                tied[pair] = static_cast<std::uint8_t>(tied[pair] | (1U << component));
                pairs.emplace_back(pair.first, pair.second);
            }
        }
        if (zeroLength && !pairs.empty())
            candidates.emplace_back(entity.card, std::move(pairs));
    }

    // The welded sets: the pairs tied in all six components, joined; each set stands for its smallest grid point.
    std::vector<base::EntityId> ids;
    for (const auto& [pair, bits] : tied)
    {
        if (bits == ALL_SIX)
        {
            ids.push_back(pair.first);
            ids.push_back(pair.second);
        }
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    std::vector<std::size_t> parent(ids.size());
    std::iota(parent.begin(), parent.end(), std::size_t{0});
    const auto indexOf = [&](base::EntityId node)
    { return static_cast<std::size_t>(std::lower_bound(ids.begin(), ids.end(), node) - ids.begin()); };
    const auto find = [&](std::size_t item)
    {
        while (parent[item] != item)
        {
            parent[item] = parent[parent[item]];
            item = parent[item];
        }
        return item;
    };
    for (const auto& [pair, bits] : tied)
    {
        if (bits != ALL_SIX)
            continue;
        // the smaller position becomes the root, so the root of a set is its smallest grid point
        const std::size_t left = find(indexOf(pair.first));
        const std::size_t right = find(indexOf(pair.second));
        if (left != right)
            parent[std::max(left, right)] = std::min(left, right);
    }
    result.m_nodes.reserve(ids.size());
    for (std::size_t index = 0; index < ids.size(); ++index)
        result.m_nodes.emplace_back(ids[index], ids[find(index)]);

    // A rigid element welds when every pair it ties ends in one welded set.
    for (const auto& [card, pairs] : candidates)
    {
        const bool welds =
            std::all_of(pairs.begin(), pairs.end(),
                        [&](const std::pair<base::EntityId, base::EntityId>& pair)
                        {
                            // two distinct grid points are welded when one grid point stands for both
                            return result.representative(pair.first) == result.representative(pair.second);
                        });
        if (welds)
            result.m_welds.push_back(locationKey(card));
    }
    std::sort(result.m_welds.begin(), result.m_welds.end());
    return result;
}

base::EntityId WeldedNodes::representative(base::EntityId node) const noexcept
{
    const auto found = std::lower_bound(m_nodes.begin(), m_nodes.end(), node,
                                        [](const std::pair<base::EntityId, base::EntityId>& entry, base::EntityId id)
                                        { return entry.first < id; });
    return found != m_nodes.end() && found->first == node ? found->second : node;
}

bool WeldedNodes::isWeld(model::CardLocation element) const noexcept
{
    return std::binary_search(m_welds.begin(), m_welds.end(), locationKey(element));
}

} // namespace oor::analysis
