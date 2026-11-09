#pragma once

#include "oor/results/core/MemoryTableIndex.hpp"
#include "oor/results/core/ResultCatalogue.hpp"
#include "oor/results/core/ResultFile.hpp"
#include "oor/results/core/ResultKind.hpp"
#include "oor/results/core/ResultTable.hpp"
#include "oor/results/core/ResultTables.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::results
{

// Results made in memory: a companion package hands its results to this package as arrays. The
// table it makes is an ordinary `ResultTable` that already holds its values, in an ordinary `ResultTables`: attaching,
// filters, frame changes, derived quantities, queries and exports work as for a table decoded from a file; the only
// differences are that `origin` is "memory", the status is `notValidated`, and the table says the frame its values are
// in (a program that makes results knows it; a decoder has to infer it).

// The "file" of a set of results made in memory: it has no content, metadata that say so, and the label of the set as
// its path, so that the default `analysis` label of the set is that label. `ResultAttachment::attach` takes it like any
// file. Its tables are held by the ResultTables of the set, not by the file: `tables` gives none.
class MemorySetFile final : public ResultFile
{
public:
    explicit MemorySetFile(std::string label);

    [[nodiscard]] std::string_view format() const noexcept override { return "memory"; }
    [[nodiscard]] const ResultFileMetadata& metadata() const noexcept override { return m_metadata; }
    [[nodiscard]] const std::vector<base::Diagnostic>& diagnostics() const noexcept override { return m_diagnostics; }
    [[nodiscard]] ResultTables tables(const ResultFilter& filter) const override;
    void setLabel(const std::string& label);

private:
    ResultFileMetadata m_metadata;
    std::vector<base::Diagnostic> m_diagnostics;
};

// What a table is told: the kind, the subcase and analysis, the steps, the rows, the components and the values.
struct MemoryTableInput
{
    ResultKind kind = ResultKind::Displacements;
    ResultVariant variant = ResultVariant::Plain;
    // The container of a family kind: the element type of stresses, strains, forces, failure indices, strain energy,
    // heat flux and heat flows ("ShellQuad4"), the variant of grid point stresses ("surface"); empty for the other
    // kinds.
    std::string elementName;
    std::int32_t subcase = 1;
    // The analysis code (1 static, 2 modes, 5 frequency, 6 transient, 10 non-linear static, ...); what the step kind
    // says when 0.
    std::int32_t analysisCode = 0;
    // The kind of the steps; `None` takes it from the analysis code. A static analysis has one step with no value.
    StepKind stepKind = StepKind::None;
    std::int32_t designCycle = -1;
    std::int32_t superelement = -1;
    // The load set of a static table; the mode number of a shape of a buckling analysis (a table a shape, with one step
    // and its eigenvalue).
    std::int32_t loadSet = 0;

    // The steps: one value each (NaN for the single step of a static table). `eigenvalues`, `eigenvaluesImag` and
    // `cycles` are one a step, or empty.
    std::vector<double> steps;
    std::vector<double> eigenvalues;
    std::vector<double> eigenvaluesImag;
    std::vector<double> cycles;

    // The rows: the grid point or element of each (`ids`) and what a row of that kind has besides. An element table has
    // a layer for each row (1 when none is given). Each is the table's to keep: a vector, or the block of a companion
    // package kept without a copy (`LabelArray::kept`), which the table then keeps as its labels.
    LabelArray<std::int32_t> ids;
    LabelArray<std::int32_t> pointTypes;
    LabelArray<std::int32_t> nodeIds;
    LabelArray<std::int32_t> elementIds;
    LabelArray<std::int32_t> layers;
    LabelArray<double> positions;
    std::vector<ResultText> texts;

    // The components in the order the values hold them (empty: every component of the kind, in its order).
    std::vector<std::string> components;
    // The values of shape (steps, rows, components) in C order, as bytes of `type`.
    ValueType type = ValueType::Float64;
    std::span<const std::byte> values;
    // The block that holds `values`, kept by the table as its own in place of a copy (empty: the values are copied):
    // it starts at `values.data()`, on 8 bytes, and holds the values unchanged for as long as it lives. The buffer of a
    // companion package taken without copy (`MemoryResults.add_table(copy=False)`), which its deleter gives back.
    std::shared_ptr<double[]> sharedValues;
    // How a complex value is written (real and imaginary parts, or magnitude and phase: the values given are those of
    // the table, this only says what they mean).
    ResultFormat storedFormat = ResultFormat::Real;

    StoredFrame storedFrame = StoredFrame::Unknown;
    std::int32_t frameSystem = 0;
    std::string units = "unknown";
    std::string tableName;
    std::string title;
    std::string subtitle;
    std::string label;
};

// The element types (container names) a kind has, and the components a table of that kind (and element type, real or
// complex) has by default: thin names over the result catalogue (ResultCatalogue.hpp), kept for the code that makes
// tables in memory. The references stay valid for the life of the process.
[[nodiscard]] const std::vector<std::string>& memoryElementNames(ResultKind kind);
[[nodiscard]] const std::vector<std::string>& memoryComponents(ResultKind kind, std::string_view elementName,
                                                               bool complex,
                                                               ResultVariant variant = ResultVariant::Plain);
// Every list of components a table of the kind (and element type) can have: the default first, then the alternatives.
// The components given to `makeMemoryTable` are a part of one of them, in any order.
[[nodiscard]] const std::vector<std::vector<std::string>>& memoryComponentLists(ResultKind kind,
                                                                                std::string_view elementName,
                                                                                bool complex);
// Every layout of the catalogue (allTableLayouts), as a copy.
[[nodiscard]] std::vector<TableLayout> memoryLayouts();

// The problems with the input (an empty list when it is right): a kind with no such element type, a component the kind
// does not have, rows that are repeated, arrays that do not have one entry per row or step, a number of bytes that is
// not steps x rows x components values of the type, a step kind that does not go with the analysis code, a shared
// block that does not start at the values or on 8 bytes. The repeated
// places cost a time linear in the rows when the IDs do not decrease (the rows of an element together, as every decoder
// gives them), n log n otherwise.
[[nodiscard]] std::vector<std::string> checkMemoryTable(const MemoryTableInput& input);

// Makes the table, taking the arrays of `input` (pass it moved: its row labels are not copied again). Throws
// UsageError with every problem `checkMemoryTable` finds. The values are copied, unless `sharedValues` holds them: the
// table then keeps that block as its values.
[[nodiscard]] std::shared_ptr<ResultTable> makeMemoryTable(MemoryTableInput input);

// Puts a table in its container (by kind, element type or variant, variant of results).
void addTableToResults(ResultTables& tables, std::shared_ptr<ResultTable> table);

// Adds tables made in memory to a set whose keys `index` holds, all or none: throws UsageError, before adding any, when
// a table has the key of a table of the set or of an earlier table of the batch (the message names the key and the
// place of the table in the batch).
void addMemoryTables(ResultTables& tables, MemoryTableIndex& index,
                     std::span<const std::shared_ptr<ResultTable>> batch);

} // namespace oor::results
