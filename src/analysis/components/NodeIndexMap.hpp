#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

namespace oor::analysis::detail
{

// Node IDs to dense indexes 0 .. n-1 in increasing order of the ID. Dense IDs (the usual model: 1 to N, perhaps with
// gaps) are found by one array lookup and built without a sort; sparse ones by a binary search. -1 for an ID that was
// not given. Private to the component code.
class NodeIndexMap
{
public:
    // `ids` may hold repeats and any order.
    explicit NodeIndexMap(std::span<const base::EntityId> ids)
    {
        base::EntityId largest = 0;
        bool positive = true;
        for (const base::EntityId id : ids)
        {
            if (id <= 0)
                positive = false;
            largest = std::max(largest, id);
        }
        if (positive && !ids.empty() && static_cast<std::size_t>(largest) <= ids.size() * 8 + 1024)
        {
            // Mark, then number in increasing order: no sort.
            m_direct.assign(static_cast<std::size_t>(largest) + 1, -1);
            for (const base::EntityId id : ids)
                m_direct[static_cast<std::size_t>(id)] = 0;
            std::int32_t next = 0;
            m_sorted.reserve(ids.size() / 2 + 16);
            for (std::size_t id = 0; id < m_direct.size(); ++id)
            {
                if (m_direct[id] == 0)
                {
                    m_direct[id] = next++;
                    m_sorted.push_back(static_cast<base::EntityId>(id));
                }
            }
            return;
        }
        m_sorted.assign(ids.begin(), ids.end());
        std::sort(m_sorted.begin(), m_sorted.end());
        m_sorted.erase(std::unique(m_sorted.begin(), m_sorted.end()), m_sorted.end());
    }

    [[nodiscard]] std::size_t size() const noexcept { return m_sorted.size(); }
    [[nodiscard]] std::span<const base::EntityId> ids() const noexcept { return m_sorted; }
    [[nodiscard]] std::int32_t indexOf(base::EntityId id) const noexcept
    {
        if (!m_direct.empty())
        {
            if (id <= 0 || static_cast<std::size_t>(id) >= m_direct.size())
                return -1;
            return m_direct[static_cast<std::size_t>(id)];
        }
        const auto found = std::lower_bound(m_sorted.begin(), m_sorted.end(), id);
        if (found == m_sorted.end() || *found != id)
            return -1;
        return static_cast<std::int32_t>(found - m_sorted.begin());
    }

private:
    std::vector<std::int32_t> m_direct;
    std::vector<base::EntityId> m_sorted;
};

} // namespace oor::analysis::detail
