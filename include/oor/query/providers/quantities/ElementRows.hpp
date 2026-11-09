#pragma once

#include "oor/base/threading/ParallelFor.hpp"
#include "oor/query/providers/quantities/QuantityProvider.hpp"
#include "oor/query/results/ResultSets.hpp"
#include "oor/query/results/RowAccumulator.hpp"
#include "oor/results/core/ResultLayout.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace oor::query
{

// The rows of one element table (one subcase, one analysis, one element type), made flat: a row for each step and place
// in an element, with the values of the quantity by name (NaN where the table does not have one). What every provider
// of an element quantity gives, whatever it computed it from.
struct ElementFlat
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    results::StepKind stepKind = results::StepKind::None;
    std::string elementName; // the container: "ShellQuad4", "ShellQuad4Composite"
    std::string card;        // the card type, "ShellQuad4"
    std::string frame;       // the frame the values are written in
    std::string status;      // the status of the table the rows were read from ("validated", ...)
    Precision precision = Precision::Double;
    bool magnitudePhase = false; // the table stored its complex values as magnitude and phase
    std::int16_t provider = 0;

    std::vector<std::int32_t> eid;
    std::vector<std::int32_t> node;  // the grid point of the row, 0 at the centre
    std::vector<std::int32_t> layer; // from 1
    std::vector<double> step;
    std::vector<double> eigenvalue;
    std::vector<double> position;
    std::vector<std::string> location;
    std::map<std::string, std::vector<double>> values;     // the values, or their real parts
    std::map<std::string, std::vector<double>> imaginary;  // the imaginary parts of a complex table
    std::map<std::string, std::vector<std::string>> texts; // text labels of the rows (the failure theory of a ply)

    [[nodiscard]] std::size_t rows() const noexcept { return eid.size(); }
    [[nodiscard]] bool isComplex() const noexcept { return !imaginary.empty(); }
    // Keeps the rows `keep` names (increasing positions).
    void keepRows(const std::vector<std::size_t>& keep);
    // A flat of the same table with the labels of the rows `keep` names (increasing positions) and no value.
    [[nodiscard]] ElementFlat rowLabels(const std::vector<std::size_t>& keep) const;
};

// The rows the providers of an element quantity gave, with per-row resolution: a provider's rows that an earlier
// provider already gave (the same subcase, step and element of the same result set) are dropped, so the later ways only
// fill in what is missing. The rows of one provider are never dropped for those of the same provider (a table of centre
// values and one of corner values of the same element are both kept).
class ElementFlatSet
{
public:
    void beginProvider(std::int16_t index, const ProviderInfo& info);
    [[nodiscard]] std::int64_t rowCount() const noexcept { return m_rows; }
    // Adds the rows of `flat` that the set does not have, as rows of the provider that is current; returns how many.
    std::int64_t add(ElementFlat&& flat);
    [[nodiscard]] const std::vector<ElementFlat>& flats() const noexcept { return m_flats; }
    [[nodiscard]] std::vector<ElementFlat>& flats() noexcept { return m_flats; }
    [[nodiscard]] const std::vector<ProviderInfo>& providers() const noexcept { return m_providers; }
    [[nodiscard]] std::int16_t currentProvider() const noexcept { return m_current; }
    // The (set, subcase, design cycle, element type) groups already there, for a provider that only completes some.
    [[nodiscard]] bool hasGroup(std::size_t set, std::int32_t subcase, std::int32_t designCycle,
                                const std::string& elementName) const;
    // True when the element of the group has a row at the step already.
    [[nodiscard]] bool covers(std::size_t set, std::int32_t subcase, std::int32_t designCycle,
                              const std::string& elementName, std::int32_t eid) const;
    // True when the providers before the current one gave a row without a step (a static table) for each of the
    // elements `elementIds` of the group: the rows of a static table of these elements that the current provider would
    // add are all dropped (`add`), so it need not make them.
    [[nodiscard]] bool coversStatic(std::size_t set, std::int32_t subcase, std::int32_t designCycle,
                                    const std::string& elementName, std::span<const std::int32_t> eids) const;

private:
    using GroupKey = std::tuple<std::size_t, std::int32_t, std::int32_t, std::string>;
    struct RowKey
    {
        std::int32_t eid;
        std::uint64_t step;
        bool operator==(const RowKey& other) const noexcept { return eid == other.eid && step == other.step; }
    };
    struct RowKeyHash
    {
        std::size_t operator()(const RowKey& key) const noexcept
        {
            return static_cast<std::size_t>(key.step * 0x9E3779B97F4A7C15ULL) ^
                   (static_cast<std::size_t>(static_cast<std::uint32_t>(key.eid)) * 0xC2B2AE3D27D4EB4FULL);
        }
    };
    struct Group
    {
        std::vector<std::size_t> flats; // the positions in m_flats
    };

    std::vector<ElementFlat> m_flats;
    std::vector<ProviderInfo> m_providers;
    std::map<GroupKey, Group> m_groups;
    std::int16_t m_current = 0;
    std::int64_t m_rows = 0;
};

// The number of the bits of a step value as a key (a step is the same step to the precision of the file).
[[nodiscard]] std::uint64_t stepKey(double step) noexcept;

// What a query of an element quantity asks of its providers.
struct ElementRequest
{
    std::optional<std::vector<std::int64_t>> elementIds;
    std::vector<std::string> types; // card type names ("ShellQuad4"); empty: all
    std::optional<std::vector<std::int64_t>> propertyIds;
    std::string location; // "", "center" or "corners"
    std::optional<std::vector<std::int64_t>> layers;
    // `fastenerForces`: the axis the forces are given along ("" as stored, or "x", "y", "z" of the element system, or
    // a vector in the basic system).
    std::string axisName;
    std::optional<std::array<double, 3>> axisVector;
    // The frame the tables are turned to before the rows are read, whatever the `frame` of the call says ("" the call's
    // own).
    std::string tableFrame;
    // The composite failure theory a margin is asked for ("" the stored index, else the theory of the property).
    std::string theory;
    // The flats of the ply stress tables of the request when the caller made them already (the recomputed failure
    // index reads them instead of the tables); null to read the tables.
    const std::vector<ElementFlat>* plyStresses = nullptr;

    // The request of the arguments `elementIds`, `types`, `propertyIds`, `location` and `layers` of a call.
    [[nodiscard]] static ElementRequest from(const QueryArguments& arguments);
    [[nodiscard]] bool wantsElement(std::int32_t eid) const;
    [[nodiscard]] bool wantsType(const std::string& card) const;
    [[nodiscard]] bool wantsLayer(std::int32_t layer) const;
    // The properties are known only with a model: a request that names some needs one, and an element is wanted when
    // its property is among them.
    [[nodiscard]] bool wantsProperty(std::int32_t pid) const;

private:
    // sorted, each once: a binary search per row
    std::vector<std::int64_t> m_elements;
    std::vector<std::int64_t> m_properties;
    std::vector<std::int64_t> m_layers;
};

// The card type of an element container name ("ShellQuad4Composite" -> "ShellQuad4").
[[nodiscard]] std::string cardOfContainer(const std::string& elementName);

// The element container uses the exact card type ("ShellQuad4" -> "ShellQuad4").
[[nodiscard]] std::string containerOfCard(const std::string& card);

// The sorted elements a request names that the set has no row of, in any provider.
[[nodiscard]] std::vector<std::int32_t> missingElements(const ElementRequest& request, const ElementFlatSet& set);

// Records in the provenance of a table made from the rows of `set` (an envelope, a ranking, a group of them) which
// providers the rows it summarises came from: each provider with the number of rows it gave and the `columns` the
// summary has of them.
class RowAccumulator;
void noteInputs(RowAccumulator& accumulator, const ElementFlatSet& set, const std::vector<std::string>& columns);

// The rows of a table as its index counts them, all its steps: what a query that selects nothing may make of it. A
// table whose rows are not known counts as large.
[[nodiscard]] inline std::size_t indexedRows(const TableRef& ref) noexcept
{
    constexpr std::size_t UNKNOWN = std::size_t{1} << 20U;
    const results::ResultTableMeta& meta = ref.table->meta();
    return meta.rowCountKnown && meta.stepCountKnown ? meta.rowCount * std::max<std::size_t>(meta.stepCount, 1)
                                                     : UNKNOWN;
}

// Makes what each of `count` tables gives (`make(index)`, which decodes and flattens table `index`) on the threads of
// the run, a batch of tables at a time, and hands each to `take(index, made)` on the calling thread in the order of the
// tables: the rows come out as one thread gives them. A batch is of a few tables a thread, fewer when their rows
// (`rows(index)`, as indexedRows counts them) would hold much memory. `make` may run on any thread: it reads the
// context and its table, and writes nothing else (else `parallel` is false: one table at a time).
template <typename Made, typename Rows, typename Make, typename Take>
void makeInOrder(const ResultQueryContext& context, std::size_t count, const Rows& rows, const Make& make,
                 const Take& take, bool parallel = true)
{
    constexpr std::size_t ROWS_IN_FLIGHT = 4'000'000;
    const results::DecodeContext decode = context.decode();
    const std::size_t most = parallel && decode.pool != nullptr ? 8 * (decode.pool->workerCount() + 1) : 1;
    std::vector<Made> made;
    for (std::size_t first = 0; first < count;)
    {
        std::size_t size = 0;
        std::size_t held = 0;
        while (first + size < count && size < most && (size == 0 || held + rows(first + size) <= ROWS_IN_FLIGHT))
            held += rows(first + size++);
        made.assign(size, Made{});
        const auto body = [&](std::size_t index) { made[index] = make(first + index); };
        if (decode.pool != nullptr && size > 1)
            base::parallelFor(*decode.pool, size, body, decode.token);
        else
            for (std::size_t index = 0; index < size; ++index)
                body(index);
        for (std::size_t index = 0; index < size; ++index)
            take(first + index, std::move(made[index]));
        first += size;
    }
}

} // namespace oor::query
