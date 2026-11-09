#include "oor/analysis/adjacency/ElementIndexes.hpp"

#include "GroupBuilder.hpp"

namespace oor::analysis
{

namespace
{

bool hasNodes(std::uint16_t type) noexcept
{
    return ElementTopology::supports(type);
}

bool hasEdges(std::uint16_t type) noexcept
{
    return dimensionOf(ElementTopology::shapeOf(type)) >= 1;
}

bool hasFaces(std::uint16_t type) noexcept
{
    return dimensionOf(ElementTopology::shapeOf(type)) >= 2;
}

} // namespace

std::span<const ElementRef> NodeToElementIndex::elementsOf(base::EntityId node) const noexcept
{
    if (!m_direct.empty())
    {
        if (node <= 0 || static_cast<std::size_t>(node) >= m_direct.size() ||
            m_direct[static_cast<std::size_t>(node)] < 0)
            return {};
        return m_groups.elements(static_cast<std::size_t>(m_direct[static_cast<std::size_t>(node)]));
    }
    return m_groups.elementsOf(node);
}

NodeToElementIndex NodeToElementIndex::build(const model::Model& model, base::ThreadPool* pool)
{
    using detail::Entry;
    std::vector<Entry<base::EntityId>> entries =
        detail::collectEntries<base::EntityId>(model, pool, hasNodes,
                                                [](const model::CardTable& table, std::uint16_t type, std::uint32_t row,
                                                   std::vector<Entry<base::EntityId>>& out)
                                                {
                                                    thread_local std::vector<base::EntityId> nodes;
                                                    nodes.clear();
                                                    ElementTopology::nodesOf(table, type, row, nodes);
                                                    const std::uint64_t ref = detail::packRef(type, row);
                                                    for (const base::EntityId node : nodes)
                                                        out.push_back(Entry<base::EntityId>{node, ref});
                                                });
    detail::parallelSort(entries, pool, detail::entryLess<base::EntityId>);
    NodeToElementIndex index;
    index.m_groups = GroupedIndex<base::EntityId>::fromSorted(entries);
    const std::span<const base::EntityId> ids = index.m_groups.keys();
    // Dense IDs (the usual model: 1 to N, perhaps with gaps) are found by one array lookup; sparse ones by a binary
    // search.
    if (!ids.empty() && ids.front() > 0 && static_cast<std::size_t>(ids.back()) <= ids.size() * 16 + 1024)
    {
        index.m_direct.assign(static_cast<std::size_t>(ids.back()) + 1, -1);
        for (std::size_t position = 0; position < ids.size(); ++position)
            index.m_direct[static_cast<std::size_t>(ids[position])] = static_cast<std::int32_t>(position);
    }
    return index;
}

EdgeTable EdgeTable::build(const model::Model& model, base::ThreadPool* pool)
{
    using detail::Entry;
    std::vector<Entry<EdgeKey>> entries = detail::collectEntries<EdgeKey>(
        model, pool, hasEdges,
        [](const model::CardTable& table, std::uint16_t type, std::uint32_t row, std::vector<Entry<EdgeKey>>& out)
        {
            thread_local std::vector<EdgeKey> edges;
            edges.clear();
            ElementTopology::edgesOf(table, type, row, edges);
            const std::uint64_t ref = detail::packRef(type, row);
            for (const EdgeKey& edge : edges)
                out.push_back(Entry<EdgeKey>{edge, ref});
        });
    detail::parallelSort(entries, pool, detail::entryLess<EdgeKey>);
    EdgeTable result;
    result.m_groups = GroupedIndex<EdgeKey>::fromSorted(entries);
    return result;
}

FaceTable FaceTable::build(const model::Model& model, base::ThreadPool* pool)
{
    using detail::Entry;
    std::vector<Entry<FaceKey>> entries = detail::collectEntries<FaceKey>(
        model, pool, hasFaces,
        [](const model::CardTable& table, std::uint16_t type, std::uint32_t row, std::vector<Entry<FaceKey>>& out)
        {
            thread_local std::vector<FaceKey> faces;
            faces.clear();
            ElementTopology::facesOf(table, type, row, faces);
            const std::uint64_t ref = detail::packRef(type, row);
            for (const FaceKey& face : faces)
                out.push_back(Entry<FaceKey>{face, ref});
        });
    detail::parallelSort(entries, pool, detail::entryLess<FaceKey>);
    FaceTable result;
    result.m_groups = GroupedIndex<FaceKey>::fromSorted(entries);
    return result;
}

} // namespace oor::analysis
