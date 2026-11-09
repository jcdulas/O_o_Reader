#include "oor/analysis/adjacency/AdjacencyCache.hpp"

namespace oor::analysis
{

namespace
{

// Making the cache of a model is the one step that two threads reading the same model could do at once.
std::mutex& creationMutex()
{
    static std::mutex mutex;
    return mutex;
}

} // namespace

AdjacencyCache::AdjacencyCache(const AdjacencyCache& other) : TableAttachment(other)
{
    const std::scoped_lock lock(other.m_mutex);
    m_version = other.m_version;
    m_nodeIndex = other.m_nodeIndex;
    m_edgeTable = other.m_edgeTable;
    m_faceTable = other.m_faceTable;
    m_statistics = other.m_statistics;
}

std::unique_ptr<model::TableAttachment> AdjacencyCache::clone() const
{
    return std::make_unique<AdjacencyCache>(*this);
}

AdjacencyCache& AdjacencyCache::of(const model::Model& model)
{
    const std::scoped_lock lock(creationMutex());
    model::AttachmentSlot& slot = model.derivedSlot();
    auto* cache = dynamic_cast<AdjacencyCache*>(slot.get());
    if (cache == nullptr)
    {
        slot.reset(std::make_unique<AdjacencyCache>());
        cache = static_cast<AdjacencyCache*>(slot.get());
    }
    return *cache;
}

const AdjacencyCache* AdjacencyCache::peek(const model::Model& model) noexcept
{
    return dynamic_cast<const AdjacencyCache*>(model.derivedSlot().get());
}

void AdjacencyCache::validate(const model::Model& model)
{
    if (m_version == model.structureVersion())
        return;
    if (m_nodeIndex || m_edgeTable || m_faceTable)
        ++m_statistics.invalidations;
    m_nodeIndex.reset();
    m_edgeTable.reset();
    m_faceTable.reset();
    m_version = model.structureVersion();
}

std::shared_ptr<const NodeToElementIndex> AdjacencyCache::nodeIndex(const model::Model& model, base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    validate(model);
    if (!m_nodeIndex)
    {
        m_nodeIndex = std::make_shared<const NodeToElementIndex>(NodeToElementIndex::build(model, pool));
        ++m_statistics.nodeIndexBuilds;
    }
    return m_nodeIndex;
}

std::shared_ptr<const EdgeTable> AdjacencyCache::edgeTable(const model::Model& model, base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    validate(model);
    if (!m_edgeTable)
    {
        m_edgeTable = std::make_shared<const EdgeTable>(EdgeTable::build(model, pool));
        ++m_statistics.edgeTableBuilds;
    }
    return m_edgeTable;
}

std::shared_ptr<const FaceTable> AdjacencyCache::faceTable(const model::Model& model, base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    validate(model);
    if (!m_faceTable)
    {
        m_faceTable = std::make_shared<const FaceTable>(FaceTable::build(model, pool));
        ++m_statistics.faceTableBuilds;
    }
    return m_faceTable;
}

bool AdjacencyCache::isBuilt(const model::Model& model, AdjacencyKind kind) const
{
    const std::scoped_lock lock(m_mutex);
    if (m_version != model.structureVersion())
        return false;
    switch (kind)
    {
    case AdjacencyKind::Node:
        return m_nodeIndex != nullptr;
    case AdjacencyKind::Edge:
        return m_edgeTable != nullptr;
    case AdjacencyKind::Face:
        return m_faceTable != nullptr;
    }
    return false;
}

AdjacencyCache::Statistics AdjacencyCache::statistics() const
{
    const std::scoped_lock lock(m_mutex);
    return m_statistics;
}

} // namespace oor::analysis
