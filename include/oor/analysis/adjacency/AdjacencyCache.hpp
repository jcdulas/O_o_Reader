#pragma once

#include "oor/analysis/adjacency/ElementIndexes.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/storage/TableAttachment.hpp"

#include <cstdint>
#include <memory>
#include <mutex>

namespace oor::analysis
{

// What makes two elements neighbours.
enum class AdjacencyKind : std::uint8_t
{
    Node, // they share at least one node (corners, mid-side nodes, scalar points; the orientation node of a bar does
          // not count)
    Edge, // they share an edge: both ends of one side of a shell, an edge of a solid, a line
    Face  // they share a face: the same corners (a shell is its own face, a solid has its faces)
};

// What the model keeps of its connectivity: the node to element index, the edge table and the face table, each built
// the first time something asks for it and kept until the connectivity changes.
//
// The cache lives in the model (Model::derivedSlot), so a copy of a model has a cache of its own, in the state of the
// original: the tables are immutable once built and shared by the two caches until one of them is invalidated. It does
// not watch the model: it remembers Model::structureVersion() at the time it built, and drops the tables when the
// number is not the one of the model any more. That number changes when elements are added, removed or given other
// nodes, when the tables are compacted or the IDs renumbered, and for nothing else; editing the coordinate of a grid
// point, a property, a thickness or the coordinate system of a node does not touch the cache.
class AdjacencyCache final : public model::TableAttachment
{
public:
    // What the cache has built since it was made, for the tests and the performance notes.
    struct Statistics
    {
        std::uint64_t nodeIndexBuilds = 0;
        std::uint64_t edgeTableBuilds = 0;
        std::uint64_t faceTableBuilds = 0;
        std::uint64_t invalidations = 0; // times the cache dropped tables because the connectivity changed
    };

    AdjacencyCache() = default;
    AdjacencyCache(const AdjacencyCache& other);
    AdjacencyCache& operator=(const AdjacencyCache&) = delete;

    [[nodiscard]] std::unique_ptr<model::TableAttachment> clone() const override;

    // The cache of a model (made empty on first use), brought in line with the connectivity of the model.
    [[nodiscard]] static AdjacencyCache& of(const model::Model& model);
    // The cache the model has now, untouched, or null when it has none (for the statistics).
    [[nodiscard]] static const AdjacencyCache* peek(const model::Model& model) noexcept;

    // The tables, built when missing (over `pool`). A table stays valid for whoever holds the pointer, whatever the
    // model does afterwards; it describes the connectivity of the model as it was when it was built.
    [[nodiscard]] std::shared_ptr<const NodeToElementIndex> nodeIndex(const model::Model& model,
                                                                      base::ThreadPool* pool = nullptr);
    [[nodiscard]] std::shared_ptr<const EdgeTable> edgeTable(const model::Model& model,
                                                             base::ThreadPool* pool = nullptr);
    [[nodiscard]] std::shared_ptr<const FaceTable> faceTable(const model::Model& model,
                                                             base::ThreadPool* pool = nullptr);

    // True when the table that `kind` of adjacency reads is built and still describes the model, so that asking for it
    // costs a lookup and no build (a caller that wants to release the Python GIL only for a build asks first).
    [[nodiscard]] bool isBuilt(const model::Model& model, AdjacencyKind kind) const;

    [[nodiscard]] Statistics statistics() const;

private:
    // Drops the tables when the model changed its connectivity since they were built (the mutex is held).
    void validate(const model::Model& model);

    mutable std::mutex m_mutex;
    std::uint64_t m_version = 0;
    std::shared_ptr<const NodeToElementIndex> m_nodeIndex;
    std::shared_ptr<const EdgeTable> m_edgeTable;
    std::shared_ptr<const FaceTable> m_faceTable;
    Statistics m_statistics;
};

} // namespace oor::analysis
