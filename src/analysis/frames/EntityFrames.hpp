#pragma once

#include "oor/analysis/frames/FrameCache.hpp"
#include "oor/analysis/frames/FrameConventions.hpp"
#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/results/attach/ResultAttachment.hpp"
#include "oor/results/core/ResultData.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace oor::analysis::detail
{

// A frame that is a place in the results: the stored frame of a table or the frame a change goes to.
struct FrameRole
{
    results::StoredFrame kind = results::StoredFrame::Basic;
    std::int32_t system = 0; // Coordinate, Cylindrical, Spherical
};

// The frames of the rows of a table, kept once for each grid point or element the rows are about (a shell has two to
// ten rows, a grid point force table several rows a grid point): `entityOfRow` says which.
//
//   stored      the rotation of the stored frame of each entity to the basic system (the columns are the axes); NaN
//   where
//               the entity is not linked to a card of the model
//   target      the same for the frame a change goes to (empty when no target was asked for)
//   point       the position of a grid point, the centre of an element (basic system)
//   passThrough 1 for a point that is not a grid point (a scalar point): its values are copied, never turned
//   plyAngle    per row, in radians: the angle of the ply of the row from the material axis (plies only)
struct EntityFrames
{
    std::vector<std::uint32_t> entityOfRow;
    // the frames of the entities are views of a set the cache may share with the next tables: `owner` keeps it
    std::shared_ptr<const void> owner;
    std::span<const base::Mat3> stored;
    std::span<const base::Mat3> target;
    std::span<const base::Vec3> point;
    std::span<const std::uint8_t> passThrough;
    std::vector<double> plyAngle;
    std::size_t unlinkedEntities = 0;

    [[nodiscard]] std::size_t entityCount() const noexcept { return stored.size(); }
    // The rotation of the stored frame of a row to the basic system: the stored frame of its entity turned by the angle
    // of its ply.
    [[nodiscard]] base::Mat3 storedOfRow(std::size_t row) const noexcept;
    // The same for the frame of a change: the target of the entity, turned by the ply angle when the target is the ply
    // system.
    [[nodiscard]] base::Mat3 targetOfRow(std::size_t row) const noexcept;
    // True when the stored frame, or the target, is the ply system (its rotation depends on the row).
    bool storedIsPly = false;
    bool targetIsPly = false;
    // When a target was asked for and neither frame is the ply system: 1 for an entity whose target is its stored
    // frame within the rounding of their axes, each entry within 1e-14 (a material frame that is the element frame: no
    // MCID, THETA 0, a rectangle), whose change is then the identity; empty otherwise.
    // `targetIsStored`: every entity so, and linked.
    std::span<const std::uint8_t> sameFrame;
    bool targetIsStored = false;
};

// Finds the frames of the rows of a decoded table. `stored` is what StoredFrames::describe said (its kind must be
// known); `target`, when given, is the frame of a change; `cache`, when given, keeps the frames of each entity for the
// next tables of the same model. Throws UnsupportedFrameError when a frame cannot be built (a ply that has no
// laminate, a system the model does not define: CoordinateSystemError). Without `rowsWhenTargetIsStored`,
// `entityOfRow` is left empty when the target is the stored frame of every entity (`targetIsStored`): the caller then
// keeps the values as they are.
[[nodiscard]] EntityFrames buildEntityFrames(const model::Model& model, const results::ResultAttachment& attachment,
                                             const results::ResultTable& table, const results::ResultData& data,
                                             FrameFamily family, const StoredFrameInfo& stored,
                                             const std::optional<FrameRole>& target, base::ThreadPool* pool,
                                             FrameCache* cache = nullptr, bool rowsWhenTargetIsStored = true);

} // namespace oor::analysis::detail
