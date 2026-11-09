#pragma once

#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/results/attach/ResultAttachment.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace oor::analysis
{

// The frame the values of a table are written in, from the model they are attached to: the descriptor
// `table.stored_frame` holds. `kind` Unknown comes with the reason.
struct StoredFrameInfo
{
    results::StoredFrame kind = results::StoredFrame::Unknown;
    std::int32_t system = 0; // the system of a Coordinate, Cylindrical or Spherical frame
    std::string reason;      // why the frame is unknown; empty otherwise
    std::string source;      // where the frame comes from: "the table name", "PARAM,OMID", "PSOLID CORDM", ...
};

// PARAM,OMID of a model: whether shell results are written in the material system (format reference, PARAM OMID).
enum class MaterialFrameOutput : std::uint8_t
{
    No,     // absent or NO: the element system
    Yes,    // the material system
    Both,   // both forms are written to the file
    Invalid // another value
};

// How the stored frame of the results of a model are found and when what was found goes stale.
class StoredFrames
{
public:
    [[nodiscard]] static MaterialFrameOutput materialFrameOutput(const model::Model& model);
    // The PID of the element at `location` (shells, solids, rods, tubes, bars, beams, bushes, shear panels); nothing
    // for another card.
    [[nodiscard]] static std::optional<base::EntityId> propertyOf(const model::Model& model,
                                                                  model::CardLocation location);

    // The frame of the values of `table` in an attached model:
    //   nodal tables           the output system CD of each grid point (the BOUG tables: the basic system), from the
    //   name
    //                          of the table; the model gives each grid point its system
    //   shells                 the element system, or the material system when PARAM,OMID is YES (not for CQUADR, which
    //   the
    //                          format reference does not list); BOTH writes two forms that the file does not tell
    //                          apart: unknown
    //   plies of laminates     the ply system
    //   solids                 PSOLID CORDM: 0 the basic system, a rectangular system its own; a cylindrical or
    //   spherical
    //                          system or an element system (CORDM < 0) is unknown
    //   a table made by a frame change   the frame it was changed to
    // Never throws for a frame it does not know: the answer is Unknown with the reason.
    [[nodiscard]] static StoredFrameInfo describe(const model::Model& model,
                                                  const results::ResultAttachment& attachment,
                                                  const results::ResultTable& table,
                                                  const results::DecodeContext& context = {});
    // What the table says without a model: the table name for a nodal table, unknown for the others.
    [[nodiscard]] static StoredFrameInfo describeWithoutModel(const results::ResultTable& table);

    // A number that changes when what decides the frames of the results changes: the displacement systems (CD) of the
    // grid points and the systems they resolve to, and PARAM,OMID. The position of a grid point in a cylindrical or
    // spherical CD is not part of it.
    [[nodiscard]] static std::uint64_t signature(const model::Model& model, base::ThreadPool* pool = nullptr);

    // Throws StaleAttachmentError when the attachment cannot be trusted for a frame change: the model's structure
    // changed since it was attached, or what decides the frames did.
    static void requireCurrent(const model::Model& model, const results::ResultAttachment& attachment,
                               base::ThreadPool* pool = nullptr);
};

// Attaches a result set to a model with the frame signature recorded (what StoredFrames::requireCurrent compares), the
// way the bindings do.
[[nodiscard]] std::shared_ptr<results::ResultAttachment> attachResults(const std::shared_ptr<model::Model>& model,
                                                                       std::shared_ptr<results::ResultFile> file,
                                                                       std::shared_ptr<results::ResultTables> tables,
                                                                       const results::AttachOptions& options = {},
                                                                       const results::DecodeContext& context = {},
                                                                       base::ThreadPool* pool = nullptr);
// Refreshes an attachment and records the frame signature again.
void refreshAttachment(const model::Model& model, results::ResultAttachment& attachment, bool strict = false,
                       const results::DecodeContext& context = {}, base::ThreadPool* pool = nullptr);

} // namespace oor::analysis
