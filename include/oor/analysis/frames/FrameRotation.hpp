#pragma once

#include "oor/analysis/frames/FrameCache.hpp"
#include "oor/analysis/frames/FrameConventions.hpp"
#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/results/attach/ResultAttachment.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace oor::analysis
{

// What a frame change goes to.
struct TargetFrame
{
    FrameTargetKind kind = FrameTargetKind::Basic;
    // Coordinate, Cylindrical, Spherical: the system (a CORD card of the model); Cylindrical and Spherical read its
    // axes as those kinds of system, whatever kind the card is.
    std::int32_t system = 0;
    // Direction: the direction in the basic system, one for all the rows or one for each row (rows of the table, in
    // order). They need not be unit vectors; a zero vector gives NaN.
    std::vector<base::Vec3> directions;
};

// The frame changes of the results of an attached model: the values of a table turned from the
// frame they are stored in to another, row by row.
//
// A frame is a rotation whose columns are its axes in the basic system. A vector v stored in frame S is v' = T^T S v in
// frame T; a second-order tensor (a stress, a strain, the membrane and bending resultants of a shell) is T^T S X S^T T;
// a strain is a tensor with half the engineering shear that is stored. Complex values turn their real and imaginary
// parts alike. A cylindrical or spherical target is evaluated at the grid point (nodal results) or at the centre of the
// element (element results). Only the pairs of the convention table (FrameConventionTable) are offered: any other
// raises UnsupportedFrameError with the reason of its row.
class FrameRotation
{
public:
    // The table in the target frame, as a new table (marked as derived): same rows, steps and precision; the components
    // are those of the frame (`FrameConventions.hpp` says which for each family). Rows whose grid point or element is
    // not linked to a card of the model are NaN. Throws UnsupportedFrameError (the pair is refused, or a component the
    // change needs is not in the table) and StaleAttachmentError (the model changed since the attach). `cache`, when
    // given, keeps the frames of the grid points and elements for the next tables of the same model.
    [[nodiscard]] static std::shared_ptr<results::ResultTable> rotate(
        const model::Model& model, const results::ResultAttachment& attachment, const results::ResultTable& table,
        const TargetFrame& target, base::ThreadPool* pool = nullptr, const results::DecodeContext& context = {},
        FrameCache* cache = nullptr);

    // The table in the target frame, turned when it is read: a selection of it (`select`) selects the rows of `table`
    // first, so that only the rows asked for are decoded and turned, and a table already in the target frame is
    // returned as it is. The same values and errors as `rotate`; the model, the attachment and the cache are kept until
    // the table is read.
    [[nodiscard]] static std::shared_ptr<results::ResultTable> rotateWhenRead(
        std::shared_ptr<const model::Model> model, std::shared_ptr<const results::ResultAttachment> attachment,
        std::shared_ptr<results::ResultTable> table, const TargetFrame& target,
        const results::DecodeContext& context = {}, std::shared_ptr<FrameCache> cache = nullptr);

    // The rotation of the stored frame of every row to the basic system, (rows) matrices in the order of the rows: the
    // table's `frame_matrices()`. NaN for a row whose grid point or element is not linked; a ply row has the material
    // system turned by its ply angle.
    [[nodiscard]] static std::vector<base::Mat3> storedMatrices(const model::Model& model,
                                                                const results::ResultAttachment& attachment,
                                                                const results::ResultTable& table,
                                                                base::ThreadPool* pool = nullptr,
                                                                const results::DecodeContext& context = {});
};

} // namespace oor::analysis
