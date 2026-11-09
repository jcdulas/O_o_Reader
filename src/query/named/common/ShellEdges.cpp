#include "oor/query/named/common/ShellEdges.hpp"

#include "oor/analysis/adjacency/AdjacencyCache.hpp"
#include "oor/analysis/components/ComponentCache.hpp"

#include <algorithm>
#include <tuple>

namespace oor::query
{

ShellEdges::ShellEdges(const QueryRun& run, const ShellSet& shells, EdgeIdentity identity)
{
    const model::Model& model = *run.model;
    m_welds = identity == EdgeIdentity::Welded ? analysis::ComponentCache::of(model).welds(model, run.context.pool())
                                               : std::make_shared<const analysis::WeldedNodes>();
    m_table = analysis::AdjacencyCache::of(model).edgeTable(model, run.context.pool());
    const std::size_t edgeCount = m_table->edgeCount();
    constexpr std::size_t CHUNK = 65536;
    const std::size_t chunks = (edgeCount + CHUNK - 1) / CHUNK;
    struct Part
    {
        std::vector<ShellEdge> edges;
        std::vector<std::uint32_t> shells;
    };
    std::vector<Part> parts(chunks);
    run.context.parallel(edgeCount, CHUNK,
                         [&](base::IndexRange range)
                         {
                             Part& part = parts[range.begin / CHUNK];
                             std::vector<std::uint32_t> found;
                             for (std::size_t edge = range.begin; edge < range.end; ++edge)
                             {
                                 found.clear();
                                 for (const analysis::ElementRef& element : m_table->elements(edge))
                                 {
                                     const std::int64_t index = shells.indexOf(element);
                                     if (index >= 0 && shells.isSelected(static_cast<std::size_t>(index)))
                                         found.push_back(static_cast<std::uint32_t>(index));
                                 }
                                 if (found.empty())
                                     continue;
                                 // Shells in increasing order of element ID (the list is sorted by ID: so by position).
                                 std::sort(found.begin(), found.end());
                                 found.erase(std::unique(found.begin(), found.end()), found.end());
                                 part.edges.push_back(ShellEdge{static_cast<std::uint32_t>(edge),
                                                                static_cast<std::uint32_t>(part.shells.size()),
                                                                static_cast<std::uint32_t>(found.size())});
                                 part.shells.insert(part.shells.end(), found.begin(), found.end());
                             }
                         });
    for (Part& part : parts)
    {
        const auto base = static_cast<std::uint32_t>(m_shells.size());
        for (ShellEdge edge : part.edges)
        {
            edge.first += base;
            edge.tableFirst = static_cast<std::uint32_t>(m_tableEdges.size());
            m_tableEdges.push_back(edge.edge);
            m_edges.push_back(edge);
        }
        m_shells.insert(m_shells.end(), part.shells.begin(), part.shells.end());
    }
    if (!m_welds->empty())
        mergeWelded();
}

void ShellEdges::mergeWelded()
{
    // Each edge by the welded grid points of its ends, smaller first; the edges of a seam then come together.
    struct Entry
    {
        base::EntityId first;
        base::EntityId second;
        std::uint32_t edge; // the position in m_edges
    };
    std::vector<Entry> entries;
    entries.reserve(m_edges.size());
    for (std::size_t index = 0; index < m_edges.size(); ++index)
    {
        const analysis::EdgeKey key = m_table->edges()[m_edges[index].edge];
        const base::EntityId first = m_welds->representative(key.first);
        const base::EntityId second = m_welds->representative(key.second);
        if (first != second) // an edge between two welded grid points is no edge
            entries.push_back(
                Entry{std::min(first, second), std::max(first, second), static_cast<std::uint32_t>(index)});
    }
    std::stable_sort(entries.begin(), entries.end(), [](const Entry& left, const Entry& right)
                     { return std::tie(left.first, left.second) < std::tie(right.first, right.second); });
    std::vector<ShellEdge> edges;
    std::vector<std::uint32_t> shells;
    std::vector<std::uint32_t> tableEdges;
    std::vector<std::uint32_t> found;
    for (std::size_t at = 0; at < entries.size();)
    {
        std::size_t end = at;
        while (end < entries.size() && entries[end].first == entries[at].first &&
               entries[end].second == entries[at].second)
            ++end;
        // the shells of the edges of the seam, in increasing order of element ID
        found.clear();
        ShellEdge merged;
        merged.edge = m_edges[entries[at].edge].edge;
        merged.tableFirst = static_cast<std::uint32_t>(tableEdges.size());
        for (std::size_t item = at; item < end; ++item)
        {
            const ShellEdge& edge = m_edges[entries[item].edge];
            const auto own = shellsOf(edge);
            found.insert(found.end(), own.begin(), own.end());
            tableEdges.push_back(edge.edge);
            merged.edge = std::min(merged.edge, edge.edge);
        }
        std::sort(found.begin(), found.end());
        found.erase(std::unique(found.begin(), found.end()), found.end());
        merged.first = static_cast<std::uint32_t>(shells.size());
        merged.count = static_cast<std::uint32_t>(found.size());
        merged.tableCount = static_cast<std::uint32_t>(end - at);
        shells.insert(shells.end(), found.begin(), found.end());
        edges.push_back(merged);
        at = end;
    }
    // the edges in the order of the edge table, as without welds
    std::vector<std::size_t> order(edges.size());
    for (std::size_t index = 0; index < order.size(); ++index)
        order[index] = index;
    std::sort(order.begin(), order.end(),
              [&](std::size_t left, std::size_t right) { return edges[left].edge < edges[right].edge; });
    m_edges.clear();
    for (const std::size_t index : order)
        m_edges.push_back(edges[index]);
    m_shells = std::move(shells);
    m_tableEdges = std::move(tableEdges);
}

} // namespace oor::query
