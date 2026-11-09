#include "oor/analysis/superelements/BoundaryPointNumbering.hpp"

#include <algorithm>
#include <limits>

namespace oor::analysis
{

namespace
{

constexpr std::uint32_t NO_GROUP = std::numeric_limits<std::uint32_t>::max();
constexpr base::EntityId SMALLEST_ID = std::numeric_limits<base::EntityId>::min();

// Union-find over the points: each point starts alone, a link merges two sets (path halving keeps the trees flat).
class DisjointSets
{
public:
    std::uint32_t add()
    {
        m_parent.push_back(static_cast<std::uint32_t>(m_parent.size()));
        return m_parent.back();
    }
    std::uint32_t find(std::uint32_t item)
    {
        while (m_parent[item] != item)
        {
            m_parent[item] = m_parent[m_parent[item]];
            item = m_parent[item];
        }
        return item;
    }
    void join(std::uint32_t first, std::uint32_t second)
    {
        const std::uint32_t a = find(first);
        const std::uint32_t b = find(second);
        if (a != b)
            m_parent[std::max(a, b)] = std::min(a, b);
    }

private:
    std::vector<std::uint32_t> m_parent;
};

} // namespace

BoundaryPointNumbering::BoundaryPointNumbering(const SuperelementCatalogue& catalogue,
                                               std::span<const BoundaryLink> links,
                                               std::span<const SuperelementPoint> lonePoints)
{
    DisjointSets sets;
    const auto indexOf = [&](const SuperelementPoint& point)
    {
        const auto [it, inserted] = m_index.try_emplace(point, 0U);
        if (inserted)
            it->second = sets.add();
        return it->second;
    };
    for (const BoundaryLink& link : links)
        sets.join(indexOf(link.upstream), indexOf(link.joined));
    for (const SuperelementPoint& point : lonePoints)
        (void)indexOf(point);

    // The groups, numbered 0.. in the order of their smallest point.
    std::vector<std::uint32_t> groupOfRoot(m_index.size(), NO_GROUP);
    m_groupOf.assign(m_index.size(), 0);
    for (const auto& [point, item] : m_index)
    {
        const std::uint32_t root = sets.find(item);
        if (groupOfRoot[root] == NO_GROUP)
        {
            groupOfRoot[root] = static_cast<std::uint32_t>(m_members.size());
            m_members.emplace_back();
        }
        m_groupOf[item] = groupOfRoot[root];
        m_members[groupOfRoot[root]].push_back(point); // the map is ordered: the members come ascending
    }

    // the solver's numbering: primaries first, then images, each by number; inside one its points by ID.
    m_numbers.assign(m_members.size(), 0);
    std::vector<base::EntityId> order;
    for (const bool images : {false, true})
    {
        for (const SuperelementEntry& entry : catalogue.entries())
        {
            if (entry.isImage() == images)
                order.push_back(entry.seid);
        }
    }
    for (const base::EntityId seid : order)
    {
        for (auto it = m_index.lower_bound(SuperelementPoint{seid, SMALLEST_ID});
             it != m_index.end() && it->first.seid == seid; ++it)
        {
            std::int32_t& number = m_numbers[m_groupOf[it->second]];
            if (number == 0)
                number = ++m_count;
        }
    }
}

std::optional<std::uint32_t> BoundaryPointNumbering::groupOf(const SuperelementPoint& point) const
{
    const auto found = m_index.find(point);
    if (found == m_index.end())
        return std::nullopt;
    return m_groupOf[found->second];
}

std::vector<base::EntityId> BoundaryPointNumbering::pointsOf(base::EntityId seid) const
{
    std::vector<base::EntityId> points;
    for (auto it = m_index.lower_bound(SuperelementPoint{seid, SMALLEST_ID});
         it != m_index.end() && it->first.seid == seid; ++it)
        points.push_back(it->first.nid);
    return points;
}

std::int32_t BoundaryPointNumbering::numberOf(const SuperelementPoint& point) const
{
    const std::optional<std::uint32_t> group = groupOf(point);
    return group ? m_numbers[*group] : 0;
}

std::optional<base::EntityId> BoundaryPointNumbering::pointIn(const SuperelementPoint& point,
                                                               base::EntityId seid) const
{
    const std::optional<std::uint32_t> group = groupOf(point);
    if (!group)
        return std::nullopt;
    for (const SuperelementPoint& member : m_members[*group])
    {
        if (member.seid == seid)
            return member.nid;
    }
    return std::nullopt;
}

std::vector<SuperelementPoint> BoundaryPointNumbering::membersOf(const SuperelementPoint& point) const
{
    const std::optional<std::uint32_t> group = groupOf(point);
    return group ? m_members[*group] : std::vector<SuperelementPoint>{};
}

} // namespace oor::analysis
