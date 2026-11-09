#include "oor/query/named/common/PropertyZoning.hpp"

#include <algorithm>
#include <numeric>

namespace oor::query
{

namespace
{

// Union-find over the elements; the smaller position becomes the root, so the roots do not depend on the order of the
// unions.
class Sets
{
public:
    explicit Sets(std::size_t count) : m_parent(count) { std::iota(m_parent.begin(), m_parent.end(), 0U); }
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
        const std::uint32_t left = find(first);
        const std::uint32_t right = find(second);
        if (left != right)
            m_parent[std::max(left, right)] = std::min(left, right);
    }

private:
    std::vector<std::uint32_t> m_parent;
};

} // namespace

std::vector<std::uint32_t> propertyZoneRoots(const NodeLists& nodes, const std::vector<std::int32_t>& properties)
{
    const std::size_t count = properties.size();
    // Sort (node, property, element) and join the elements of each group.
    struct Entry
    {
        std::uint64_t key;
        std::uint32_t element;
        [[nodiscard]] bool operator<(const Entry& other) const noexcept
        {
            return key != other.key ? key < other.key : element < other.element;
        }
    };
    std::vector<Entry> entries;
    entries.reserve(nodes.nodes.size());
    for (std::size_t index = 0; index < count; ++index)
    {
        if (properties[index] == 0)
            continue;
        for (const std::int32_t node : nodes.of(index))
            entries.push_back(Entry{(static_cast<std::uint64_t>(static_cast<std::uint32_t>(node)) << 32U) |
                                        static_cast<std::uint32_t>(properties[index]),
                                    static_cast<std::uint32_t>(index)});
    }
    std::sort(entries.begin(), entries.end());
    Sets sets(count);
    for (std::size_t at = 1; at < entries.size(); ++at)
    {
        if (entries[at].key == entries[at - 1].key)
            sets.join(entries[at - 1].element, entries[at].element);
    }
    std::vector<std::uint32_t> roots(count);
    for (std::size_t index = 0; index < count; ++index)
        roots[index] = sets.find(static_cast<std::uint32_t>(index));
    return roots;
}

} // namespace oor::query
