#include "oor/analysis/adjacency/MeshAdjacency.hpp"

#include "oor/base/threading/ParallelFor.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

constexpr std::size_t PARALLEL_ELEMENTS = 2048;
constexpr std::size_t ELEMENTS_PER_CHUNK = 1024;
constexpr base::EntityId MAX_ENTITY_NODE = base::MAX_ENTITY_ID;

// Which card types a call accepts: all of them when the list is empty.
std::vector<std::uint8_t> typeMask(std::span<const std::uint16_t> ofTypes)
{
    std::vector<std::uint8_t> mask;
    if (ofTypes.empty())
        return mask;
    mask.assign(model::Model::typeCount(), 0);
    for (const std::uint16_t type : ofTypes)
    {
        if (type < mask.size())
            mask[type] = 1;
    }
    return mask;
}

// The tables a call needs, built once, and the lists one thread works with. A finder is made for each chunk of
// elements: the tables are shared and read only, the scratch lists are its own.
class NeighbourFinder
{
public:
    NeighbourFinder(const model::Model& model, AdjacencyKind kind, const std::vector<std::uint8_t>& mask,
                    const NodeToElementIndex* nodes, const EdgeTable* edges, const FaceTable* faces)
        : m_kind(kind), m_mask(mask), m_nodes(nodes), m_edges(edges), m_faces(faces)
    {
        m_tables.resize(model::Model::typeCount(), nullptr);
        for (std::size_t type = 1; type < m_tables.size(); ++type)
            m_tables[type] = model.findTable(static_cast<std::uint16_t>(type));
    }

    // The IDs of the neighbours of the element in `self`, increasing, in `out` (replacing what it held).
    void neighbours(model::CardLocation self, std::vector<base::EntityId>& out)
    {
        out.clear();
        const model::CardTable* table = m_tables[self.table];
        if (table == nullptr)
            return;
        const ElementRef own{self.row, self.table};
        switch (m_kind)
        {
        case AdjacencyKind::Node:
            m_nodeIds.clear();
            ElementTopology::nodesOf(*table, self.table, self.row, m_nodeIds);
            for (const base::EntityId node : m_nodeIds)
                append(m_nodes->elementsOf(node), own, out);
            break;
        case AdjacencyKind::Edge:
            m_edgeKeys.clear();
            ElementTopology::edgesOf(*table, self.table, self.row, m_edgeKeys);
            for (const EdgeKey& edge : m_edgeKeys)
                append(m_edges->elementsOf(edge), own, out);
            break;
        case AdjacencyKind::Face:
            m_faceKeys.clear();
            ElementTopology::facesOf(*table, self.table, self.row, m_faceKeys);
            for (const FaceKey& face : m_faceKeys)
                append(m_faces->elementsOf(face), own, out);
            break;
        }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }

    // The IDs of the elements that use a node, increasing.
    void usersOf(base::EntityId node, std::vector<base::EntityId>& out)
    {
        out.clear();
        append(m_nodes->elementsOf(node), ElementRef{0, 0}, out);
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }

private:
    void append(std::span<const ElementRef> refs, const ElementRef& own, std::vector<base::EntityId>& out) const
    {
        for (const ElementRef& ref : refs)
        {
            if (ref == own || (!m_mask.empty() && m_mask[ref.type] == 0))
                continue;
            out.push_back(m_tables[ref.type]->idAt(ref.row));
        }
    }

    AdjacencyKind m_kind;
    const std::vector<std::uint8_t>& m_mask;
    const NodeToElementIndex* m_nodes;
    const EdgeTable* m_edges;
    const FaceTable* m_faces;
    std::vector<const model::CardTable*> m_tables;
    std::vector<base::EntityId> m_nodeIds;
    std::vector<EdgeKey> m_edgeKeys;
    std::vector<FaceKey> m_faceKeys;
};

// The tables that a kind of adjacency reads, held for the length of a call.
struct Tables
{
    std::shared_ptr<const NodeToElementIndex> nodes;
    std::shared_ptr<const EdgeTable> edges;
    std::shared_ptr<const FaceTable> faces;
};

Tables tablesFor(const model::Model& model, AdjacencyKind kind, base::ThreadPool* pool)
{
    AdjacencyCache& cache = AdjacencyCache::of(model);
    Tables tables;
    switch (kind)
    {
    case AdjacencyKind::Node:
        tables.nodes = cache.nodeIndex(model, pool);
        break;
    case AdjacencyKind::Edge:
        tables.edges = cache.edgeTable(model, pool);
        break;
    case AdjacencyKind::Face:
        tables.faces = cache.faceTable(model, pool);
        break;
    }
    return tables;
}

// Runs `body(chunk)` for every chunk of `count` items, on the pool when the call is large.
template <typename Body>
void forEachChunk(std::size_t count, base::ThreadPool* pool, const std::vector<base::IndexRange>& chunks,
                  const Body& body)
{
    if (pool != nullptr && pool->workerCount() > 0 && count >= PARALLEL_ELEMENTS)
        base::parallelFor(*pool, chunks.size(), body);
    else
    {
        for (std::size_t chunk = 0; chunk < chunks.size(); ++chunk)
            body(chunk);
    }
}

// Puts the rows each chunk made one after the other.
AdjacencyCsr assemble(std::size_t count, const std::vector<std::vector<std::int64_t>>& lengths,
                      std::vector<std::vector<base::EntityId>>& values)
{
    AdjacencyCsr result;
    result.offsets.reserve(count + 1);
    result.offsets.push_back(0);
    std::size_t total = 0;
    for (const auto& part : values)
        total += part.size();
    result.values.reserve(total);
    for (std::size_t chunk = 0; chunk < lengths.size(); ++chunk)
    {
        for (const std::int64_t length : lengths[chunk])
            result.offsets.push_back(result.offsets.back() + length);
        result.values.insert(result.values.end(), values[chunk].begin(), values[chunk].end());
        std::vector<base::EntityId>().swap(values[chunk]);
    }
    return result;
}

} // namespace

std::vector<base::EntityId> MeshAdjacency::elementsOfNode(const model::Model& model, base::EntityId node,
                                                           std::span<const std::uint16_t> ofTypes,
                                                           base::ThreadPool* pool)
{
    const Tables tables = tablesFor(model, AdjacencyKind::Node, pool);
    const std::vector<std::uint8_t> mask = typeMask(ofTypes);
    NeighbourFinder finder(model, AdjacencyKind::Node, mask, tables.nodes.get(), nullptr, nullptr);
    std::vector<base::EntityId> users;
    finder.usersOf(node, users);
    return users;
}

std::vector<base::EntityId> MeshAdjacency::adjacentElements(const model::Model& model, model::CardLocation location,
                                                             AdjacencyKind kind, std::span<const std::uint16_t> ofTypes,
                                                             base::ThreadPool* pool)
{
    const Tables tables = tablesFor(model, kind, pool);
    const std::vector<std::uint8_t> mask = typeMask(ofTypes);
    NeighbourFinder finder(model, kind, mask, tables.nodes.get(), tables.edges.get(), tables.faces.get());
    std::vector<base::EntityId> found;
    finder.neighbours(location, found);
    return found;
}

AdjacencyCsr MeshAdjacency::adjacency(const model::Model& model, std::span<const model::CardLocation> elements,
                                      AdjacencyKind kind, std::span<const std::uint16_t> ofTypes,
                                      base::ThreadPool* pool)
{
    const Tables tables = tablesFor(model, kind, pool);
    const std::vector<std::uint8_t> mask = typeMask(ofTypes);
    const std::vector<base::IndexRange> chunks = base::splitIntoChunks(elements.size(), ELEMENTS_PER_CHUNK);
    std::vector<std::vector<std::int64_t>> lengths(chunks.size());
    std::vector<std::vector<base::EntityId>> values(chunks.size());
    forEachChunk(elements.size(), pool, chunks,
                 [&](std::size_t chunk)
                 {
                     NeighbourFinder finder(model, kind, mask, tables.nodes.get(), tables.edges.get(),
                                            tables.faces.get());
                     std::vector<base::EntityId> found;
                     for (std::size_t index = chunks[chunk].begin; index < chunks[chunk].end; ++index)
                     {
                         finder.neighbours(elements[index], found);
                         lengths[chunk].push_back(static_cast<std::int64_t>(found.size()));
                         values[chunk].insert(values[chunk].end(), found.begin(), found.end());
                     }
                 });
    return assemble(elements.size(), lengths, values);
}

AdjacencyCsr MeshAdjacency::nodeElements(const model::Model& model, std::span<const base::EntityId> nodes,
                                         std::span<const std::uint16_t> ofTypes, base::ThreadPool* pool)
{
    const Tables tables = tablesFor(model, AdjacencyKind::Node, pool);
    const std::vector<std::uint8_t> mask = typeMask(ofTypes);
    const std::vector<base::IndexRange> chunks = base::splitIntoChunks(nodes.size(), ELEMENTS_PER_CHUNK);
    std::vector<std::vector<std::int64_t>> lengths(chunks.size());
    std::vector<std::vector<base::EntityId>> values(chunks.size());
    forEachChunk(nodes.size(), pool, chunks,
                 [&](std::size_t chunk)
                 {
                     NeighbourFinder finder(model, AdjacencyKind::Node, mask, tables.nodes.get(), nullptr, nullptr);
                     std::vector<base::EntityId> found;
                     for (std::size_t index = chunks[chunk].begin; index < chunks[chunk].end; ++index)
                     {
                         finder.usersOf(nodes[index], found);
                         lengths[chunk].push_back(static_cast<std::int64_t>(found.size()));
                         values[chunk].insert(values[chunk].end(), found.begin(), found.end());
                     }
                 });
    return assemble(nodes.size(), lengths, values);
}

FreeEdges MeshAdjacency::freeEdges(const model::Model& model, base::ThreadPool* pool)
{
    const std::shared_ptr<const EdgeTable> table = AdjacencyCache::of(model).edgeTable(model, pool);
    std::vector<std::uint8_t> isShell(model::Model::typeCount(), 0);
    for (std::size_t type = 1; type < isShell.size(); ++type)
        isShell[type] = dimensionOf(ElementTopology::shapeOf(static_cast<std::uint16_t>(type))) == 2 ? 1 : 0;
    FreeEdges result;
    for (std::size_t position = 0; position < table->edgeCount(); ++position)
    {
        std::size_t shells = 0;
        ElementRef owner;
        for (const ElementRef& ref : table->elements(position))
        {
            if (isShell[ref.type] != 0)
            {
                ++shells;
                owner = ref;
            }
        }
        if (shells != 1)
            continue;
        const EdgeKey& edge = table->edges()[position];
        result.nodes.push_back({edge.first, edge.second});
        result.elements.push_back(model.findTable(owner.type)->idAt(owner.row));
    }
    return result;
}

FreeFaces MeshAdjacency::freeFaces(const model::Model& model, base::ThreadPool* pool)
{
    const std::shared_ptr<const FaceTable> table = AdjacencyCache::of(model).faceTable(model, pool);
    std::vector<std::uint8_t> isSolid(model::Model::typeCount(), 0);
    for (std::size_t type = 1; type < isSolid.size(); ++type)
        isSolid[type] = dimensionOf(ElementTopology::shapeOf(static_cast<std::uint16_t>(type))) == 3 ? 1 : 0;
    FreeFaces result;
    std::vector<std::array<base::EntityId, 4>> oriented;
    for (std::size_t position = 0; position < table->faceCount(); ++position)
    {
        std::size_t solids = 0;
        ElementRef owner;
        for (const ElementRef& ref : table->elements(position))
        {
            if (isSolid[ref.type] != 0)
            {
                ++solids;
                owner = ref;
            }
        }
        if (solids != 1)
            continue;
        const FaceKey& key = table->faces()[position];
        const model::CardTable& ownerTable = *model.findTable(owner.type);
        oriented.clear();
        ElementTopology::orientedFacesOf(ownerTable, owner.type, owner.row, oriented);
        std::array<base::EntityId, 4> face =
            key.nodes; // the sorted corners if no oriented face matches (it always does)
        for (const auto& candidate : oriented)
        {
            // The key of the oriented face: its nonzero corners, increasing, with 0 last.
            std::array<base::EntityId, 4> sorted = candidate;
            for (base::EntityId& node : sorted)
            {
                if (node == 0)
                    node = MAX_ENTITY_NODE;
            }
            std::sort(sorted.begin(), sorted.end());
            for (base::EntityId& node : sorted)
            {
                if (node == MAX_ENTITY_NODE)
                    node = 0;
            }
            if (sorted == key.nodes)
            {
                face = candidate;
                break;
            }
        }
        result.nodes.push_back(face);
        result.elements.push_back(ownerTable.idAt(owner.row));
    }
    return result;
}

AdjacencyCache::Statistics MeshAdjacency::statistics(const model::Model& model)
{
    const AdjacencyCache* cache = AdjacencyCache::peek(model);
    return cache != nullptr ? cache->statistics() : AdjacencyCache::Statistics{};
}

} // namespace oor::analysis
