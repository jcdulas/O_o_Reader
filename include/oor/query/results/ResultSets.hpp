#pragma once

#include "oor/analysis/frames/FrameCache.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/named/QueryValue.hpp"
#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/results/attach/ResultAttachment.hpp"
#include "oor/results/core/NamedResult.hpp"
#include "oor/results/core/ResultFile.hpp"
#include "oor/results/core/ResultKind.hpp"
#include "oor/results/core/ResultTable.hpp"
#include "oor/results/core/ResultTables.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace oor::query
{

struct QueryRun;

// One set of results a query reads: the tables of a result file (or built in memory), its label (the `analysis`
// column), and, when the file is attached to a model, the attachment that links the IDs of the file to the cards.
struct ResultSet
{
    std::string analysis;
    std::shared_ptr<results::ResultTables> tables;
    std::shared_ptr<results::ResultAttachment> attachment; // null for a result set that has no model
    std::shared_ptr<results::ResultFile> file;             // null for a result set made in memory
};

// What to do with a row whose quantity no provider could give (the `onMissing` parameter).
enum class OnMissing : std::uint8_t
{
    Raise, // QuantityUnavailableError listing the ways tried
    Null,  // the row stays, with nulls
    Skip   // the row is left out
};

// The parameters common to the queries of the R families, read from the arguments of a call.
struct ResultSelection
{
    std::optional<std::vector<std::int64_t>> subcases;
    std::optional<StepSelection> times;
    std::optional<StepSelection> freqs;
    std::optional<StepSelection> modes;
    std::optional<StepSelection> loadSteps;
    // `frame`: nothing (as stored), a name ("basic", "element", "material", "ply") or a coordinate system ID.
    std::string frameName;
    std::int64_t frameSystem = 0;
    // "" (as stored), "realImag" or "magPhase".
    std::string complexForm;
    std::vector<std::string> results;
    std::vector<std::string> analyses;
    std::optional<std::vector<std::int64_t>> designCycles;
    OnMissing onMissing = OnMissing::Raise;
    bool withSource = false;
    // `envelope`: "" (none), "max", "min" or "absMax": the rows that differ only in the dimensions of `envelopeOver`
    // ("analyses", "subcases", "steps", "designCycles") are replaced by the critical one.
    std::string envelope;
    std::vector<std::string> envelopeOver = {"analyses"};
    // `envelopeValue`: the column the envelope decides by, when not the one the query ranks by.
    std::string envelopeValue;

    [[nodiscard]] bool hasFrame() const noexcept { return !frameName.empty() || frameSystem != 0; }
    [[nodiscard]] static ResultSelection from(const QueryArguments& arguments);
};

// The parameters of the result queries, in their order: what `query_catalog()` reports as the
// `common_parameters` of a query of the R families.
[[nodiscard]] const std::vector<ParameterInfo>& resultParameters();

// A table of a result set, selected as the call asked (subcases, steps, design cycles) and, when a frame was asked,
// rotated to it.
struct TableRef
{
    std::size_t set = 0; // the position of the result set in `ResultQueryContext::sets()`
    std::shared_ptr<results::ResultTable> table;
};

// What a result query works from: the model (when there is one), the result sets that the `results` and `analyses`
// parameters keep, the selection, and the helpers every query of the families R uses to find the tables and to link
// their rows to the cards of the model.
class ResultQueryContext
{
public:
    ResultQueryContext(const QueryRun& run, const QueryInfo& info);

    [[nodiscard]] const ResultSelection& selection() const noexcept { return m_selection; }
    [[nodiscard]] const QueryArguments& arguments() const noexcept { return m_arguments; }
    [[nodiscard]] const std::vector<ResultSet>& sets() const noexcept { return m_sets; }
    [[nodiscard]] ExecutionContext& exec() const noexcept { return m_context; }
    [[nodiscard]] results::DecodeContext decode() const;
    // The decoded rows of a table; with `elementIds` or `pointIds`, only the rows of those elements or grid points:
    // the selection is made before the table is decoded, as `select` makes it, so that a few elements of a large table
    // are not decoded whole (a table already decoded is cut in memory). Every row of an element stays (its corners,
    // fibres, plies, stations). Null when the table has no row of them.
    [[nodiscard]] std::shared_ptr<const results::ResultData> decodeRows(
        const TableRef& ref, const std::optional<std::vector<std::int64_t>>& elementIds,
        const std::optional<std::vector<std::int64_t>>& pointIds = std::nullopt) const;
    [[nodiscard]] const std::string& queryName() const noexcept { return m_name; }

    // The status of what the query has read so far: the least trusted of the tables and named results it asked for
    // ("validated", then "notValidated", then "experimental"); empty when it read none. The accumulator gives it to
    // the rows whose block does not say another.
    [[nodiscard]] const std::string& readStatus() const noexcept { return m_readStatus; }
    void noteStatus(std::string_view status) const;

    // The result sets attached or given, before the `results` and `analyses` filters: the `analysis` column exists when
    // there is more than one.
    [[nodiscard]] std::size_t attachedSetCount() const noexcept { return m_attachedCount; }
    [[nodiscard]] bool hasModel() const noexcept { return m_model != nullptr; }
    [[nodiscard]] const std::shared_ptr<model::Model>& modelPointer() const noexcept { return m_model; }
    // The model; raises a QueryError that names the query and `why` when the query runs on a result file alone.
    const model::Model& requireModel(const std::string& why) const;
    // The attached model of a result set (null when the set is not attached or the model is gone).
    [[nodiscard]] const model::Model* modelOf(std::size_t set) const;

    // The tables of a kind (a non-family kind) or of one element type / variant of a family kind, in the order of the
    // sets and of the file, after the selection. `rotate` false leaves the values in the stored frame whatever `frame`
    // says (the queries of scalar results). A table the selection leaves empty is not in the list.
    [[nodiscard]] std::vector<TableRef> tables(results::ResultKind kind, bool rotate = true,
                                               results::ResultVariant variant = results::ResultVariant::Plain) const;
    [[nodiscard]] std::vector<TableRef> elementTables(
        results::ResultKind kind, std::string_view elementName, bool rotate = true,
        results::ResultVariant variant = results::ResultVariant::Plain) const;
    // The same for a query that adds quantities over several points (a resultant): the values are turned to the basic
    // system, where the moments are taken, and the sum is expressed along the axes of the coordinate system `frame`
    // gives
    // (`resultantSystem`; any other frame is an error: a sum needs a common frame).
    [[nodiscard]] std::vector<TableRef> resultantTables(
        results::ResultKind kind, results::ResultVariant variant = results::ResultVariant::Plain) const;
    [[nodiscard]] std::vector<TableRef> resultantElementTables(
        results::ResultKind kind, std::string_view elementName,
        results::ResultVariant variant = results::ResultVariant::Plain) const;
    // The tables of an element container turned to a named frame whatever `frame` says ("element", "material",
    // "basic"): for the computations that need the values in a known frame (the integration of the forces across a
    // cut).
    [[nodiscard]] std::vector<TableRef> elementTablesIn(
        results::ResultKind kind, std::string_view elementName, const std::string& frameName,
        results::ResultVariant variant = results::ResultVariant::Plain) const;
    // The tables of a non-family kind turned to a named frame whatever `frame` says ("basic", ...): for the
    // computations that need nodal values in a known frame (the effective rods of a shear panel, from the displacements
    // of its grid points).
    [[nodiscard]] std::vector<TableRef> tablesIn(results::ResultKind kind, const std::string& frameName,
                                                 results::ResultVariant variant = results::ResultVariant::Plain) const;
    // The coordinate system a resultant is expressed in (0: basic).
    [[nodiscard]] std::int64_t resultantSystem() const;
    // The container names of a family kind that some set holds a table of, alphabetically.
    [[nodiscard]] std::vector<std::string> elementNames(
        results::ResultKind kind, results::ResultVariant variant = results::ResultVariant::Plain) const;
    // The named results of a kind ("aeroFlutterSummary", "designHistory", ...), with the set they belong to, after the
    // subcase and design cycle selection.
    struct NamedRef
    {
        std::size_t set = 0;
        std::shared_ptr<results::NamedResult> result;
    };
    [[nodiscard]] std::vector<NamedRef> named(std::string_view kind) const;

    // The frame the values of a table are written in: "nodeOutputSystem", "basic", "element", ... as the model says
    // when the table is attached, else as the table name says.
    [[nodiscard]] std::string frameNameOf(const TableRef& ref) const;

    // The card of an element of the file: the property, the card type name ("ShellQuad4"), the ID it has in the
    // model now. Zeros and empty text when the set has no model or the element is not linked.
    struct ElementCard
    {
        std::int32_t pid = 0;
        std::int32_t modelId = 0;
        std::string card;
        bool linked = false;
    };
    [[nodiscard]] ElementCard elementCard(std::size_t set, std::int32_t resultId) const;
    // The grid point of the model a node ID of the file is linked to (0 when none).
    [[nodiscard]] std::int32_t modelNodeId(std::size_t set, std::int32_t resultId) const;

private:
    [[nodiscard]] std::optional<TableRef> prepare(std::size_t set, const std::shared_ptr<results::ResultTable>& table,
                                                  bool rotate, const std::string& frameName,
                                                  std::int64_t frameSystem) const;

    ExecutionContext& m_context;
    QueryArguments m_arguments;
    std::string m_name;
    ResultSelection m_selection;
    std::shared_ptr<model::Model> m_model;
    std::vector<ResultSet> m_sets;
    std::size_t m_attachedCount = 0;
    // guards the cards and the status below: the tables of a query may be read on several threads
    mutable std::mutex m_mutex;
    mutable std::unordered_map<std::uint64_t, ElementCard> m_cards;
    // per model: the frames of its grid points and elements, kept for the tables a frame change turns, whatever the
    // result set (the files of one load case each, attached to one model, share them)
    mutable std::unordered_map<const model::Model*, std::shared_ptr<analysis::FrameCache>> m_frameCaches;
    mutable std::string m_readStatus;
    // the model of each set, asked of its attachment once (its resolver may take the GIL of Python: a query that asks
    // for it per row would wait on it from every thread)
    mutable std::unique_ptr<std::once_flag[]> m_setModelOnce;
    mutable std::vector<std::shared_ptr<model::Model>> m_setModels;
    [[nodiscard]] const std::shared_ptr<model::Model>& attachedModel(std::size_t set) const;
};

// The result sets of a run: those the caller gave (a result file alone), else those attached to the model.
[[nodiscard]] std::vector<ResultSet> collectResultSets(const QueryRun& run);

// A text with the names of the ways tried and why each failed, for the error of a quantity nobody could give.
struct ProviderAttempt
{
    std::string provider;
    bool approximate = false;
    std::int64_t rows = 0; // the rows it supplied
    std::string reason;    // why it supplied none (empty when it supplied some)
};

[[nodiscard]] std::string describeAttempts(const std::string& quantity, const std::vector<ProviderAttempt>& attempts);

} // namespace oor::query
