#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/results/core/ResultData.hpp"
#include "oor/results/core/ResultFilter.hpp"
#include "oor/results/core/ResultKind.hpp"
#include "oor/results/core/ResultLayout.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oor::results
{

class ResultAttachment;

// The frame the values of a table are written in, as far as the file and the attached model say. Which coordinate
// system that is for a given node is the model's to say; until a model is attached it is what the table name
// implies for a nodal table and unknown for an element table. A result made by a frame change (`inFrame`) says the
// frame it was rotated to.
enum class StoredFrame : std::uint8_t
{
    Unknown,
    NodeOutputSystem, // the output ("displacement") coordinate system CD of each grid point (the nodal tables)
    Basic,            // the basic coordinate system, or a result rotated to it
    Element,          // the element coordinate system of each element
    Material,         // the material coordinate system of each shell
    Ply,              // the ply system of each ply of a laminate: the material system turned by the ply angle
    Coordinate,       // the system `frameSystem`, as its card defines it (rectangular, cylindrical or spherical)
    Cylindrical,      // the axes of the system `frameSystem` read as cylindrical axes about its z axis
    Spherical,        // the axes of the system `frameSystem` read as spherical axes about its origin
    Mixed             // not one frame: it depends on the row (solids of properties that have different CORDM)
};

[[nodiscard]] std::string_view storedFrameName(StoredFrame frame) noexcept;

// What a table is, without decoding anything: where it comes from, what it holds, the steps and the number of rows
// the index tells. Fixed once the table is made.
struct ResultTableMeta
{
    ResultKind kind = ResultKind::Displacements;
    // What set of results of the kind it is: the results of an analysis (Plain), a random analysis result (PSD, RMS,
    // ...) or the modal contributions to the stresses.
    ResultVariant variant = ResultVariant::Plain;
    // The design cycle of a SOL 200 run (DESCYC); -1 when the file does not say.
    std::int32_t designCycle = -1;
    // The sub-name of a table of a family of containers that is not an element type: the variant of a grid point stress
    // table ("surface", "volumeDirect", ...); empty otherwise.
    std::string subName;
    // Element tables: the element type, which is also the name of the container ("ShellQuad4"); empty for a nodal table.
    std::string elementName;
    // The name the table has in its file (an archive keeps the name the table was given when it was made); free text,
    // used to tell two tables of the same kind apart.
    std::string tableName;
    // The places of the rows of an element type that has two layouts: "centre" (one row a fibre at the centre) or
    // "corner" (the centre and the corner grid points); empty for the other element types.
    std::string layout;
    std::int32_t subcase = 0;
    std::int32_t analysisCode = 0;
    std::int32_t superelement = -1;
    SortOrder sort = SortOrder::Sort1;
    // The values as the table delivers them: real, or complex (stored as real and imaginary parts: a table written as
    // magnitude and phase is converted when decoded).
    ValueKind valueKind = ValueKind::Real;
    // How the file writes them.
    ResultFormat storedFormat = ResultFormat::Real;
    ValueType valueType = ValueType::Float32;
    StepKind stepKind = StepKind::None;
    std::int32_t loadSet = 0; // the load set of a static table (not a step)
    // The steps, when the index knows them (every SORT1 table; a SORT2 table's steps are inside its records and known
    // after decoding). `stepCount` is the number of steps either way.
    bool stepsKnown = true;
    bool stepCountKnown = true;
    std::size_t stepCount = 0;
    std::vector<double> steps;
    std::vector<double> eigenvalues;     // one per step, or empty
    std::vector<double> eigenvaluesImag; // one per step, or empty
    std::vector<double> cycles;          // one per step, or empty
    // The rows (grid points, or places in elements) of the table; after an ID or layer filter their number is known
    // only once decoded.
    bool rowCountKnown = true;
    std::size_t rowCount = 0;
    std::vector<std::string> components;
    ResultStatus status = ResultStatus::NotValidated;
    StoredFrame storedFrame = StoredFrame::Unknown;
    // The system of a stored frame that names one (Coordinate, Cylindrical, Spherical); 0 otherwise.
    std::int32_t frameSystem = 0;
    // Why the frame is unknown, when it is (what the model lacks or what is not known); empty otherwise.
    std::string frameReason;
    // A table made by an operation on another (a frame change, a derived quantity), never read from the file: what it
    // was made by ("inFrame(basic)", "vonMises"); empty for a table of the file.
    std::string derivation;
    std::string units = "unknown";
    std::string title;
    std::string subtitle;
    std::string label;
    // The precision of the values in the source, in bytes: 4 (single precision) or 8 (double precision).
    int wordSize = 4;
    // "archive" (the HDF5 results archive) or "memory" (a result set made in memory).
    std::string origin = "memory";
};

// How a decode runs: on which threads, whether it can be cancelled and how far it got.
struct DecodeContext
{
    base::ThreadPool* pool = nullptr; // null: on the calling thread
    const base::CancellationToken* token = nullptr;
    base::ProgressCounter* progress = nullptr;
};

// What a source is asked to read: the filter members that the source applies while it reads (the IDs, the layers, and
// the steps of a SORT2 table), and the components to keep, in order.
struct LoadRequest
{
    ResultFilter filter;
    std::vector<std::string> components;
};

// Where the values of a table come from and how to read them (the datasets of an archive). A source is immutable and
// can be asked again and again.
class ResultTableSource
{
public:
    virtual ~ResultTableSource() = default;
    // Reads the selected data. Throws EmptySelectionError when the request selects nothing, IoError when the file is
    // damaged, OperationCancelled when the token says so.
    [[nodiscard]] virtual std::shared_ptr<const ResultData> load(const LoadRequest& request,
                                                                 const DecodeContext& context) const = 0;
    // True when the steps are inside every record of the file (SORT2): they cannot be dropped before reading.
    [[nodiscard]] virtual bool stepsInData() const noexcept = 0;
    // The source of the steps at `positions` (increasing) only; a source whose steps are inside the data returns null.
    [[nodiscard]] virtual std::shared_ptr<const ResultTableSource> withSteps(
        std::span<const std::uint32_t> positions) const = 0;
};

// One table of results of one subcase and analysis: every step of it, for every grid point (or later element) that
// the file holds, as a block of values of shape (steps, rows, components) with the labels of each axis.
//
// Decoding is lazy. A table is made from the index and decodes nothing: its description (`meta()`) comes from the
// index. The first call that needs the values or the row IDs (`data()`) decodes them, on the threads of the context,
// with the filter the table carries applied while reading, and caches the result: later calls return the same block.
// `select` gives a table of a subset: lazy when the table is not decoded yet (the subset is what will be read), a copy
// of the subset when it is.
class ResultTable
{
public:
    // A table that reads its values from `source` when asked.
    ResultTable(ResultTableMeta meta, std::shared_ptr<const ResultTableSource> source);
    // A table that holds its values already.
    ResultTable(ResultTableMeta meta, std::shared_ptr<const ResultData> data);

    ResultTable(const ResultTable&) = delete;
    ResultTable& operator=(const ResultTable&) = delete;

    [[nodiscard]] const ResultTableMeta& meta() const noexcept { return m_meta; }
    [[nodiscard]] bool isDecoded() const;
    // The decoded values and labels; decodes the table the first time. Thread-safe: a second thread asking while the
    // first decodes waits for it.
    [[nodiscard]] std::shared_ptr<const ResultData> data(const DecodeContext& context = {}) const;

    // The number of steps, rows and components now: from the index when it knows them, else by decoding.
    [[nodiscard]] std::size_t stepCount(const DecodeContext& context = {}) const;
    [[nodiscard]] std::size_t rowCount(const DecodeContext& context = {}) const;
    [[nodiscard]] std::size_t componentCount() const noexcept { return m_meta.components.size(); }
    // The steps (decodes a SORT2 table).
    [[nodiscard]] std::vector<double> steps(const DecodeContext& context = {}) const;

    // The table of the subset the filter keeps (its subcases and tables members are not looked at). Throws
    // EmptySelectionError when nothing is selected, which is known at once for the steps and the components and, for
    // IDs, when the table is decoded or when two ID lists meet.
    [[nodiscard]] std::shared_ptr<ResultTable> select(const ResultFilter& filter) const;

    // Makes the table apply `filter` (its IDs, element IDs, layers and, for a SORT2 table, its steps) when it decodes:
    // for the code that builds tables, before the table is shared.
    void setPending(ResultFilter filter);
    // The filters waiting for the decode (IDs, and the steps of a SORT2 table), for tests.
    [[nodiscard]] std::size_t pendingFilterCount() const noexcept { return m_pending.size(); }

    // The attachment (the link to a model) this table was attached with, or null: set by ResultAttachment, passed on to
    // the tables `select` makes and to the tables an operation derives. A weak link: the model keeps the attachment.
    void setAttachment(std::weak_ptr<ResultAttachment> attachment);
    [[nodiscard]] std::shared_ptr<ResultAttachment> attachment() const;

private:
    ResultTableMeta m_meta;
    std::shared_ptr<const ResultTableSource> m_source;
    std::vector<ResultFilter> m_pending;
    mutable std::mutex m_mutex;
    mutable std::shared_ptr<const ResultData> m_data;
    std::weak_ptr<ResultAttachment> m_attachment;
};

// A copy of the subset of `data` that `filter` keeps (steps, ids or element ids and layers, components). Throws
// EmptySelectionError when it selects nothing.
[[nodiscard]] std::shared_ptr<const ResultData> subsetResultData(const ResultData& data, const ResultFilter& filter,
                                                                 StepKind stepKind);
// The same, `data` itself when the filter keeps all of it.
[[nodiscard]] std::shared_ptr<const ResultData> subsetResultData(const std::shared_ptr<const ResultData>& data,
                                                                 const ResultFilter& filter, StepKind stepKind);

} // namespace oor::results
