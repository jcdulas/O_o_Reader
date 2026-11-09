#pragma once

#include "oor/analysis/adjacency/ElementIndexes.hpp"
#include "oor/analysis/connections/WeldedNodes.hpp"
#include "oor/query/named/common/ShellSet.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace oor::query
{

// An edge of the edge table with the selected shells that have it.
struct ShellEdge
{
    std::uint32_t edge = 0; // the position of the edge in the edge table of the model (edgeId); welded: the first one
    std::uint32_t first = 0;
    std::uint32_t count = 0;      // how many shells, 1 or more
    std::uint32_t tableFirst = 0; // the edges of the edge table it stands for (ShellEdges::tableEdgesOf)
    std::uint32_t tableCount = 1;
};

// What tells two edges apart: the IDs of their end grid points, or the welded grid points they stand on (WeldedNodes:
// the shells on either side of a seam of zero-length rigid elements share an edge; an edge whose two ends are welded
// together is no edge).
enum class EdgeIdentity : std::uint8_t
{
    Nodes,
    Welded
};

// The edges of the selected shells of a model, from the edge table of the adjacency cache, with the shells on each (the
// positions in the ShellSet, in increasing order of element ID). An edge that no selected shell has is left out. An
// edge counts only the shells: a line element or a solid on it does not make it manifold.
class ShellEdges
{
public:
    ShellEdges(const QueryRun& run, const ShellSet& shells, EdgeIdentity identity = EdgeIdentity::Nodes);

    [[nodiscard]] const analysis::EdgeTable& table() const noexcept { return *m_table; }
    [[nodiscard]] std::span<const ShellEdge> edges() const noexcept { return m_edges; }
    [[nodiscard]] std::span<const std::uint32_t> shellsOf(const ShellEdge& edge) const noexcept
    {
        return std::span<const std::uint32_t>(m_shells).subspan(edge.first, edge.count);
    }
    [[nodiscard]] analysis::EdgeKey key(const ShellEdge& edge) const noexcept { return m_table->edges()[edge.edge]; }
    // The positions in the edge table of the edges `edge` stands for: one, or with welded grid points the edges of the
    // seam it joins.
    [[nodiscard]] std::span<const std::uint32_t> tableEdgesOf(const ShellEdge& edge) const noexcept
    {
        return std::span<const std::uint32_t>(m_tableEdges).subspan(edge.tableFirst, edge.tableCount);
    }
    // The welded grid points (empty with EdgeIdentity::Nodes).
    [[nodiscard]] const analysis::WeldedNodes& welds() const noexcept { return *m_welds; }

private:
    void mergeWelded();

    std::shared_ptr<const analysis::EdgeTable> m_table;
    std::shared_ptr<const analysis::WeldedNodes> m_welds;
    std::vector<ShellEdge> m_edges;
    std::vector<std::uint32_t> m_shells;
    std::vector<std::uint32_t> m_tableEdges;
};

} // namespace oor::query
