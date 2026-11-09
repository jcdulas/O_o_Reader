#pragma once

#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/storage/TableAttachment.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace oor::model
{
class NodeTable;
class Model;
} // namespace oor::model

namespace oor::analysis
{

// The positions of the grid points in the basic system, kept as one column next to the GRID table (one entry per row of
// the table), and brought up to date when asked for.
//
// A position is recomputed only when something it depends on changed:
//   * editing the coordinates or the CP of a grid point recomputes that grid point (and the systems that use it as one
//   of
//     their three points, if it defines a CORD1x, and the grid points in those systems);
//   * editing a coordinate system card recomputes the grid points that depend on it, directly or through a chain of
//     reference systems (RID) or of grid points that define systems;
//   * adding grid points computes the new ones; deleting and compacting keep the others as they are.
// Nothing else is touched: a cache that nothing invalidated is returned as it is, after a comparison of a few integers.
//
// The cache lives in the GRID table (TableAttachment), so a copy of a model has a cache of its own, in the state of the
// original, sharing its buffers until one of them changes. Which rows were edited is told by the table's RowChangeLog;
// which coordinate systems changed is found by comparing the cards with the ones the cache last used (a model has few
// of them). A refresh that fails (a loop of definitions, a grid point in a system that does not exist) throws, and the
// next call starts again from scratch.
class BasicPositionCache final : public model::TableAttachment
{
public:
    // What the cache has done since it was made: counters, for tests and for the performance notes.
    struct Statistics
    {
        std::uint64_t refreshes = 0;           // calls that looked at the model
        std::uint64_t fullRebuilds = 0;        // refreshes that recomputed every grid point
        std::uint64_t nodesRecomputed = 0;     // positions computed, all refreshes together
        std::uint64_t systemsResolved = 0;     // coordinate systems resolved from their cards
        std::uint64_t lastNodesRecomputed = 0; // positions computed by the latest refresh
    };

    BasicPositionCache() = default;
    BasicPositionCache(const BasicPositionCache& other);
    BasicPositionCache& operator=(const BasicPositionCache&) = delete;

    [[nodiscard]] std::unique_ptr<model::TableAttachment> clone() const override;

    // The cache of a model, up to date. A model without grid points gives an empty cache. `pool` (optional) spreads a
    // large recomputation over threads. Throws CoordinateSystemError.
    //
    // The result is valid until the model is edited: positions() is a view of the cache's buffer. The model needs
    // exclusive access while it is refreshed (a refresh writes in the cache), like any edit; concurrent calls on a
    // model that is not edited are safe.
    static BasicPositionCache& of(const model::Model& model, base::ThreadPool* pool = nullptr);

    // The cache the model has now, as it is, or null when there is none yet (nothing is refreshed: for tests and
    // statistics).
    [[nodiscard]] static const BasicPositionCache* peek(const model::Model& model) noexcept;

    // One (x, y, z) per row of the GRID table (rows of deleted cards hold what they held).
    [[nodiscard]] std::span<const std::array<double, 3>> positions() const noexcept
    {
        return m_positions ? std::span<const std::array<double, 3>>(*m_positions)
                           : std::span<const std::array<double, 3>>();
    }
    [[nodiscard]] base::Vec3 position(std::uint32_t row) const noexcept
    {
        const std::array<double, 3>& value = (*m_positions)[row];
        return base::Vec3{value[0], value[1], value[2]};
    }
    // The buffer itself, for a NumPy view that must outlive the next edit (the cache copies the buffer before it writes
    // in one that is shared).
    [[nodiscard]] std::shared_ptr<const std::vector<std::array<double, 3>>> shareBuffer() const noexcept
    {
        return m_positions;
    }

    // The coordinate systems the cache resolved (the ones of the grid points and of the systems that define them), for
    // the rotation of the displacement systems.
    [[nodiscard]] CoordinateTransform& transform() noexcept { return m_transform; }
    // Whoever resolves a system through transform() after the refresh holds this: the transform keeps what it resolves
    // and is shared by every reader of the model.
    [[nodiscard]] std::mutex& mutex() noexcept { return m_mutex; }

    [[nodiscard]] const Statistics& statistics() const noexcept { return m_statistics; }

    // True when every row of the table is a live card and the rows are in increasing ID order: the buffer is then in
    // the order of the table of the grid points (by ID), and a view of it is the whole answer. Brought up to date with
    // the positions, only when cards were added, deleted, moved or renumbered (the added rows alone when cards were
    // only added).
    [[nodiscard]] bool inIdOrder() const noexcept { return m_inIdOrder; }

private:
    class Pending;

    void refresh(const model::Model& model, const model::NodeTable& grids, base::ThreadPool* pool);
    void recompute(const model::Model& model, const model::NodeTable& grids, const std::vector<std::uint32_t>& rows,
                   base::ThreadPool* pool);
    [[nodiscard]] base::Vec3 resolveNode(const model::Model& model, const model::NodeTable& grids, base::EntityId nid);
    void computeRow(const model::NodeTable& grids, std::uint32_t row);
    void makePositionsWritable();

    std::mutex m_mutex;
    std::shared_ptr<std::vector<std::array<double, 3>>> m_positions;
    std::shared_ptr<std::vector<std::uint32_t>>
        m_keys; // the card key of each row: how rows are followed through a compaction
    CoordinateTransform m_transform;
    std::uint64_t m_epoch = 0;       // the epoch of the table's rows when the cache was last brought up to date
    std::uint64_t m_compactions = 0; // the compactions of the table the cache has followed
    std::size_t m_aliveCount = 0;
    std::uint64_t m_systemStamp = 0; // what the coordinate system tables looked like when the cache last read them
    bool m_initialised = false;
    bool m_failed = false;
    bool m_inIdOrder = false;
    Pending* m_pending = nullptr; // the rows being computed, while a refresh runs
    Statistics m_statistics;
};

} // namespace oor::analysis
