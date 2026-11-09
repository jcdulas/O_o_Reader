#include "oor/analysis/coordinates/BasicPositionCache.hpp"

#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/cards/coordinate_systems/cylindricalFrame1/CylindricalFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/cylindricalFrame2/CylindricalFrame2Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame1/RectFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame2/RectFrame2Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame3/RectFrame3Table.hpp"
#include "oor/model/cards/coordinate_systems/sphericalFrame1/SphericalFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/sphericalFrame2/SphericalFrame2Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/Model.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// A refresh that recomputes at least this many rows spreads them over the thread pool.
constexpr std::size_t PARALLEL_ROW_COUNT = 20000;
constexpr std::size_t ROWS_PER_CHUNK = 8192;

constexpr std::uint8_t CLEAN = 0;
constexpr std::uint8_t PENDING = 1;
constexpr std::uint8_t IN_PROGRESS = 2;

// The systems that must be recomputed when the systems in `changed` are: the ones that use them as reference system.
// The ones that depend on grid points of the changed systems are found by the caller (they are grid points, not
// systems).
void closeOverReferenceSystems(std::unordered_set<base::EntityId>& changed, const SystemDefinitions& definitions)
{
    bool grew = true;
    while (grew)
    {
        grew = false;
        for (const auto& [cid, definition] : definitions)
        {
            if (definition.form == SystemDefinition::Form::ThreePoints && definition.referenceFrameId != 0 &&
                changed.contains(definition.referenceFrameId) && changed.insert(cid).second)
                grew = true;
        }
    }
}

// A number that changes when a coordinate system card is added, edited, renumbered or deleted: it combines the version
// and the row epoch of the six tables. Comparing it with the one seen last time tells that no system changed without
// reading them.
std::uint64_t systemStamp(const model::Model& model)
{
    std::uint64_t stamp = 0;
    const auto add = [&](const auto* table)
    {
        if (table != nullptr)
            stamp = stamp * 1000003ULL + (table->version() << 1U) + table->rows().epoch() * 31ULL;
        else
            stamp = stamp * 1000003ULL;
    };
    add(model.findTable<model::RectFrame1Table>());
    add(model.findTable<model::CylindricalFrame1Table>());
    add(model.findTable<model::SphericalFrame1Table>());
    add(model.findTable<model::RectFrame3Table>());
    add(model.findTable<model::RectFrame2Table>());
    add(model.findTable<model::CylindricalFrame2Table>());
    add(model.findTable<model::SphericalFrame2Table>());
    return stamp;
}

} // namespace

// Which rows of a refresh are still to be computed: a byte per row when many are, a small map when few are (editing one
// grid point of a million must not allocate a million bytes).
class BasicPositionCache::Pending
{
public:
    Pending(std::size_t rowCount, std::size_t workCount) : m_dense(workCount * 16 >= rowCount)
    {
        if (m_dense)
            m_bytes.assign(rowCount, CLEAN);
    }
    [[nodiscard]] std::uint8_t get(std::uint32_t row) const
    {
        if (m_dense)
            return m_bytes[row];
        const auto found = m_sparse.find(row);
        return found == m_sparse.end() ? CLEAN : found->second;
    }
    void set(std::uint32_t row, std::uint8_t state)
    {
        if (m_dense)
            m_bytes[row] = state;
        else if (state == CLEAN)
            m_sparse.erase(row);
        else
            m_sparse[row] = state;
    }

private:
    bool m_dense;
    std::vector<std::uint8_t> m_bytes;
    std::unordered_map<std::uint32_t, std::uint8_t> m_sparse;
};

BasicPositionCache::BasicPositionCache(const BasicPositionCache& other)
    : TableAttachment(other),
      m_positions(other.m_positions),
      m_keys(other.m_keys),
      m_transform(other.m_transform),
      m_epoch(other.m_epoch),
      m_compactions(other.m_compactions),
      m_aliveCount(other.m_aliveCount),
      m_systemStamp(other.m_systemStamp),
      m_initialised(other.m_initialised),
      m_failed(other.m_failed),
      m_inIdOrder(other.m_inIdOrder),
      m_statistics(other.m_statistics)
{
    // The copy shares the buffers; whoever writes first copies them (makePositionsWritable). The resolver of the copied
    // transform points at the other cache: it is set again at the next refresh.
    m_transform.setNodeResolver(nullptr);
}

std::unique_ptr<model::TableAttachment> BasicPositionCache::clone() const
{
    return std::make_unique<BasicPositionCache>(*this);
}

BasicPositionCache& BasicPositionCache::of(const model::Model& model, base::ThreadPool* pool)
{
    static BasicPositionCache empty;
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    if (grids == nullptr)
        return empty;
    model::AttachmentSlot& slot = grids->attachment();
    auto* cache = dynamic_cast<BasicPositionCache*>(slot.get());
    if (cache == nullptr)
    {
        slot.reset(std::make_unique<BasicPositionCache>());
        cache = static_cast<BasicPositionCache*>(slot.get());
    }
    cache->refresh(model, *grids, pool);
    return *cache;
}

const BasicPositionCache* BasicPositionCache::peek(const model::Model& model) noexcept
{
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    return grids == nullptr ? nullptr : dynamic_cast<const BasicPositionCache*>(grids->attachment().get());
}

void BasicPositionCache::makePositionsWritable()
{
    if (!m_positions)
        m_positions = std::make_shared<std::vector<std::array<double, 3>>>();
    else if (m_positions.use_count() > 1)
        m_positions = std::make_shared<std::vector<std::array<double, 3>>>(*m_positions);
    if (!m_keys)
        m_keys = std::make_shared<std::vector<std::uint32_t>>();
    else if (m_keys.use_count() > 1)
        m_keys = std::make_shared<std::vector<std::uint32_t>>(*m_keys);
}

void BasicPositionCache::computeRow(const model::NodeTable& grids, std::uint32_t row)
{
    const base::EntityId cp = grids.positionFrameId(row);
    const base::Vec3 raw = grids.xyz(row);
    base::Vec3 position = raw;
    if (cp != 0)
    {
        if (!m_transform.hasSystem(cp))
            throw CoordinateSystemError("grid " + std::to_string(grids.idAtRow(row)) + " refers to coordinate system " +
                                        std::to_string(cp) + ", which does not exist");
        position = m_transform.frame(cp).toBasic(raw);
    }
    (*m_positions)[row] = {position.x, position.y, position.z};
}

base::Vec3 BasicPositionCache::resolveNode(const model::Model& model, const model::NodeTable& grids, base::EntityId nid)
{
    const auto location = model.locate(base::IdSpace::Node, nid);
    if (!location || location->table != model::typeIndexOf<model::NodeTable>())
        throw CoordinateSystemError("grid " + std::to_string(nid) + " does not exist");
    const std::uint32_t row = location->row;
    if (m_pending == nullptr || m_pending->get(row) == CLEAN)
        return position(row);
    // A grid point that defines a coordinate system is needed before the points in that system: it is computed now, and
    // a grid point that is reached again while it is being computed is a loop.
    const CoordinateTransform::Visit visiting = m_transform.visit(CoordinateTransform::Thing::Node, nid);
    m_pending->set(row, IN_PROGRESS);
    computeRow(grids, row);
    m_pending->set(row, CLEAN);
    return position(row);
}

void BasicPositionCache::recompute(const model::Model& model, const model::NodeTable& grids,
                                   const std::vector<std::uint32_t>& rows, base::ThreadPool* pool)
{
    Pending pending(grids.rows().rowCount(), rows.size());
    for (const std::uint32_t row : rows)
        pending.set(row, PENDING);
    m_pending = &pending;
    struct Reset
    {
        Pending*& slot;
        ~Reset() { slot = nullptr; }
    } reset{m_pending};
    m_transform.setNodeResolver([this, &model, &grids](base::EntityId nid) { return resolveNode(model, grids, nid); });

    // The systems the rows are in, resolved one by one: resolving one computes the grid points that define it first.
    std::unordered_map<base::EntityId, const CoordinateFrame*> frames;
    base::EntityId last = 0;
    for (const std::uint32_t row : rows)
    {
        const base::EntityId cp = grids.positionFrameId(row);
        if (cp == 0 || cp == last)
            continue;
        last = cp;
        if (frames.contains(cp))
            continue;
        if (!m_transform.hasSystem(cp))
            throw CoordinateSystemError("grid " + std::to_string(grids.idAtRow(row)) + " refers to coordinate system " +
                                        std::to_string(cp) + ", which does not exist");
        frames[cp] = &m_transform.frame(cp);
    }

    // The rows still to do: a row the resolution of a system computed already is clean.
    const auto work = [&](std::size_t begin, std::size_t end)
    {
        base::EntityId lastCp = 0;
        const CoordinateFrame* lastFrame = nullptr;
        for (std::size_t index = begin; index < end; ++index)
        {
            const std::uint32_t row = rows[index];
            if (pending.get(row) != PENDING)
                continue;
            const base::EntityId cp = grids.positionFrameId(row);
            const base::Vec3 raw = grids.xyz(row);
            base::Vec3 position = raw;
            if (cp != 0)
            {
                if (cp != lastCp)
                {
                    lastCp = cp;
                    lastFrame = frames.at(cp);
                }
                position = lastFrame->toBasic(raw);
            }
            (*m_positions)[row] = {position.x, position.y, position.z};
        }
    };
    if (pool != nullptr && pool->workerCount() > 0 && rows.size() >= PARALLEL_ROW_COUNT)
    {
        const std::vector<base::IndexRange> chunks = base::splitIntoChunks(rows.size(), ROWS_PER_CHUNK);
        base::parallelFor(*pool, chunks.size(),
                          [&](std::size_t chunk) { work(chunks[chunk].begin, chunks[chunk].end); });
    }
    else
    {
        work(0, rows.size());
    }
    m_statistics.nodesRecomputed += rows.size();
    m_statistics.lastNodesRecomputed = rows.size();
}

void BasicPositionCache::refresh(const model::Model& model, const model::NodeTable& grids, base::ThreadPool* pool)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    ++m_statistics.refreshes;
    const model::RowBook& rows = grids.rows();
    model::RowChangeLog& log = grids.changes();
    const std::size_t rowCount = rows.rowCount();

    bool full = !m_initialised || m_failed;
    if (!log.enabled())
    {
        log.enable(); // what was edited before this moment was not recorded: start from the whole table
        full = true;
    }
    if (log.everythingChanged())
        full = true;

    // --- 1. the rows the cache holds follow the rows of the table ---------------------------------------------------
    std::vector<std::uint32_t> work;
    bool rowsLeft = false; // rows were deleted or moved: the systems defined by grid points may have lost one
    // Whether the rows are still in ID order, all alive: from scratch, or from this row on when cards were only added.
    const bool orderMayChange = full || rows.epoch() != m_epoch || log.idsChanged();
    std::size_t orderFrom = 0;
    try
    {
        const std::uint64_t compactionsBefore = m_compactions;
        if (full)
        {
            makePositionsWritable();
            m_positions->assign(rowCount, {0.0, 0.0, 0.0});
            m_keys->resize(rowCount);
            work.reserve(rows.aliveCount());
            for (std::uint32_t row = 0; row < rowCount; ++row)
            {
                (*m_keys)[row] = rows.keyAt(row).value;
                if (rows.isAlive(row))
                    work.push_back(row);
            }
        }
        else if (rows.epoch() != m_epoch)
        {
            const std::size_t oldCount = m_keys->size();
            if (log.compactions() == compactionsBefore && rowCount >= oldCount)
            {
                // Cards were added (and perhaps some deleted): the new rows are computed, the old ones stay.
                makePositionsWritable();
                m_positions->resize(rowCount);
                m_keys->resize(rowCount);
                std::size_t addedAlive = 0;
                for (std::size_t row = oldCount; row < rowCount; ++row)
                {
                    (*m_keys)[row] = rows.keyAt(row).value;
                    if (rows.isAlive(row))
                    {
                        work.push_back(static_cast<std::uint32_t>(row));
                        ++addedAlive;
                    }
                }
                rowsLeft = rows.aliveCount() != m_aliveCount + addedAlive;
                if (m_inIdOrder && !log.idsChanged() && !rowsLeft)
                    orderFrom = oldCount; // the old rows were in order and stay so: only the new ones are looked at
            }
            else
            {
                // A compaction (or something we cannot tell): rows are followed by their card keys, which only grow.
                auto positions = std::make_shared<std::vector<std::array<double, 3>>>(rowCount);
                auto keys = std::make_shared<std::vector<std::uint32_t>>(rowCount);
                std::size_t old = 0;
                for (std::uint32_t row = 0; row < rowCount; ++row)
                {
                    const std::uint32_t key = rows.keyAt(row).value;
                    (*keys)[row] = key;
                    while (old < oldCount && (*m_keys)[old] < key)
                        ++old;
                    if (old < oldCount && (*m_keys)[old] == key)
                        (*positions)[row] = (*m_positions)[old];
                    else if (rows.isAlive(row))
                        work.push_back(row);
                }
                m_positions = std::move(positions);
                m_keys = std::move(keys);
                rowsLeft = true;
            }
        }
        if (!full)
        {
            for (const std::uint32_t row : log.rows())
            {
                if (row < rowCount && rows.isAlive(row))
                    work.push_back(row);
            }
        }

        // --- 2. the coordinate systems that changed, and what depends on them
        // -----------------------------------------
        const std::uint64_t stamp = systemStamp(model);
        const bool systemsMayHaveChanged = full || stamp != m_systemStamp;
        SystemDefinitions newDefinitions;
        if (systemsMayHaveChanged)
            newDefinitions = readSystemDefinitions(model);
        const SystemDefinitions& definitions = systemsMayHaveChanged ? newDefinitions : m_transform.definitions();
        std::unordered_set<base::EntityId> changed;
        if (full)
        {
            m_transform.forgetAll();
        }
        else
        {
            if (systemsMayHaveChanged)
            {
                for (const auto& [cid, definition] : definitions)
                {
                    const auto before = m_transform.definitions().find(cid);
                    if (before == m_transform.definitions().end() || !(before->second == definition))
                        changed.insert(cid);
                }
                for (const auto& entry : m_transform.definitions())
                {
                    if (!definitions.contains(entry.first))
                        changed.insert(entry.first);
                }
            }
            // A system defined by grid points follows them: edit one and the system changes.
            std::unordered_map<base::EntityId, std::vector<base::EntityId>> definedBy;
            if (!work.empty() || log.idsChanged() || rowsLeft)
            {
                for (const auto& [cid, definition] : definitions)
                {
                    if (definition.form != SystemDefinition::Form::ThreeGrids)
                        continue;
                    if (log.idsChanged() || rowsLeft)
                        changed.insert(cid);
                    for (const base::EntityId node : definition.nodes)
                        definedBy[node].push_back(cid);
                }
            }
            for (const std::uint32_t row : work)
            {
                const auto found = definedBy.find(grids.idAtRow(row));
                if (found != definedBy.end())
                    changed.insert(found->second.begin(), found->second.end());
            }
            closeOverReferenceSystems(changed, definitions);
            if (!changed.empty())
            {
                // The grid points in a changed system change with it; if one of them defines another system, that one
                // changes in turn, and so on.
                std::vector<std::uint8_t> marked(rowCount, 0);
                for (const std::uint32_t row : work)
                    marked[row] = 1;
                bool grew = true;
                while (grew)
                {
                    grew = false;
                    std::vector<std::uint32_t> added;
                    for (std::uint32_t row = 0; row < rowCount; ++row)
                    {
                        if (marked[row] != 0 || !rows.isAlive(row))
                            continue;
                        const base::EntityId cp = grids.positionFrameId(row);
                        if (cp != 0 && changed.contains(cp))
                        {
                            marked[row] = 1;
                            added.push_back(row);
                        }
                    }
                    for (const std::uint32_t row : added)
                    {
                        const auto found = definedBy.find(grids.idAtRow(row));
                        if (found == definedBy.end())
                            continue;
                        for (const base::EntityId cid : found->second)
                            grew = changed.insert(cid).second || grew;
                    }
                    if (grew)
                        closeOverReferenceSystems(changed, definitions);
                    work.insert(work.end(), added.begin(), added.end());
                }
            }
        }
        const std::uint64_t resolvedBefore = m_transform.resolvedSystemCount();
        if (systemsMayHaveChanged)
            m_transform.setDefinitions(std::move(newDefinitions));
        for (const base::EntityId cid : changed)
            m_transform.forget(cid);
        if (full)
            ++m_statistics.fullRebuilds;

        // --- 3. recompute -------------------------------------------------------------------------------------------
        if (!full) // a full rebuild lists every live row once, in order
        {
            std::sort(work.begin(), work.end());
            work.erase(std::unique(work.begin(), work.end()), work.end());
        }
        m_statistics.lastNodesRecomputed = 0;
        if (!work.empty())
        {
            makePositionsWritable(); // a buffer that a NumPy array shares is copied before it is written
            recompute(model, grids, work, pool);
        }
        m_statistics.systemsResolved += m_transform.resolvedSystemCount() - resolvedBefore;
        if (orderMayChange)
        {
            bool ordered = rows.aliveCount() == rowCount;
            for (std::size_t row = std::max<std::size_t>(orderFrom, 1); ordered && row < rowCount; ++row)
                ordered =
                    grids.idAtRow(static_cast<std::uint32_t>(row - 1)) < grids.idAtRow(static_cast<std::uint32_t>(row));
            m_inIdOrder = ordered;
        }
        m_epoch = rows.epoch();
        m_compactions = log.compactions();
        m_aliveCount = rows.aliveCount();
        m_systemStamp = stamp;
        log.clear();
        m_initialised = true;
        m_failed = false;
    }
    catch (...)
    {
        m_failed = true;
        m_transform.forgetAll();
        throw;
    }
    // The resolver stays bound to this model and table until the next call to of(): the rotation of displacement
    // systems resolves systems that no grid point uses through it.
    m_transform.setNodeResolver([this, &model, &grids](base::EntityId nid) { return resolveNode(model, grids, nid); });
}

} // namespace oor::analysis
