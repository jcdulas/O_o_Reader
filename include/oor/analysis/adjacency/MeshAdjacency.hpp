#pragma once

#include "oor/analysis/adjacency/AdjacencyCache.hpp"
#include "oor/analysis/adjacency/ElementIndexes.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace oor::analysis
{

// The neighbours of many elements in compressed sparse row form: the neighbours of element `i` of the call are
// `values[offsets[i] .. offsets[i + 1])`, as element IDs in increasing order.
struct AdjacencyCsr
{
    std::vector<std::int64_t> offsets;
    std::vector<base::EntityId> values;
};

// The free edges of the shells: the edges used by exactly one shell element (a CTRIA3, CQUAD4, ... or any other
// two-dimensional element), sorted by their nodes. `elements[i]` is the one shell that uses edge `i`. A line element on
// the edge does not make it a closed edge, a solid does not either.
struct FreeEdges
{
    std::vector<std::array<base::EntityId, 2>> nodes;
    std::vector<base::EntityId> elements;
};

// The free faces of the solids: the faces used by exactly one solid element, sorted by their corner nodes. `nodes[i]`
// holds the corners of the face in the order that gives the outward normal (0 last for a triangle), as the owner
// element lists them; `elements[i]` is the owner. A shell on the face does not make it a closed face.
struct FreeFaces
{
    std::vector<std::array<base::EntityId, 4>> nodes;
    std::vector<base::EntityId> elements;
};

// Questions about which elements touch which: the elements that use a node, the neighbours of an element by node, edge
// or face, the same for many elements at once, and the free edges and faces. They all read the tables of the
// AdjacencyCache of the model, built the first time they are needed and kept until the connectivity of the model
// changes.
class MeshAdjacency
{
public:
    // The elements that use a node, by ID. `ofTypes` (card type indexes) restricts the answer to those card types;
    // empty means every element.
    [[nodiscard]] static std::vector<base::EntityId> elementsOfNode(const model::Model& model, base::EntityId node,
                                                                     std::span<const std::uint16_t> ofTypes = {},
                                                                     base::ThreadPool* pool = nullptr);

    // The elements that are neighbours of the element at `location` in the way `kind` says, by ID in increasing order,
    // the element itself left out.
    [[nodiscard]] static std::vector<base::EntityId> adjacentElements(const model::Model& model,
                                                                       model::CardLocation location, AdjacencyKind kind,
                                                                       std::span<const std::uint16_t> ofTypes = {},
                                                                       base::ThreadPool* pool = nullptr);

    // The neighbours of each of `elements`, in the order given, over the pool.
    [[nodiscard]] static AdjacencyCsr adjacency(const model::Model& model,
                                                std::span<const model::CardLocation> elements, AdjacencyKind kind,
                                                std::span<const std::uint16_t> ofTypes = {},
                                                base::ThreadPool* pool = nullptr);

    // The elements that use each of `nodes`, in the order given (an ID that no element uses gives an empty row).
    [[nodiscard]] static AdjacencyCsr nodeElements(const model::Model& model, std::span<const base::EntityId> nodes,
                                                   std::span<const std::uint16_t> ofTypes = {},
                                                   base::ThreadPool* pool = nullptr);

    [[nodiscard]] static FreeEdges freeEdges(const model::Model& model, base::ThreadPool* pool = nullptr);
    [[nodiscard]] static FreeFaces freeFaces(const model::Model& model, base::ThreadPool* pool = nullptr);

    // What the cache of the model built so far (zeros when it has none).
    [[nodiscard]] static AdjacencyCache::Statistics statistics(const model::Model& model);
};

} // namespace oor::analysis
