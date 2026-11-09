#include "oor/results/core/MemoryResults.hpp"

#include "oor/base/errors/Errors.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <set>
#include <span>
#include <tuple>
#include <utility>

namespace oor::results
{

namespace
{

std::string join(const std::vector<std::string>& names)
{
    std::string text;
    for (const std::string& name : names)
        text += (text.empty() ? "" : ", ") + name;
    return text;
}

bool containsAll(const std::vector<std::string>& list, const std::vector<std::string>& names)
{
    return std::all_of(names.begin(), names.end(), [&](const std::string& name)
                       { return std::find(list.begin(), list.end(), name) != list.end(); });
}

// The layout of the rows of an element type that has a centre and a corner layout: "corner" when a row names a grid
// point, "centre" when none does; empty for the other element types.
std::string rowLayoutOf(const std::string& elementName, std::span<const std::int32_t> nodeIds)
{
    if (!hasCentreAndCornerLayouts(elementName))
        return {};
    const bool corners = std::any_of(nodeIds.begin(), nodeIds.end(), [](std::int32_t node) { return node != 0; });
    return corners ? "corner" : "centre";
}

// The rows of a table that repeat the place of an earlier row: how many, and the first of them in the order given
// (the number of rows when none).
struct RepeatedRows
{
    std::size_t count = 0;
    std::size_t firstRow = 0;
};

// The order of the places of the rows: the ID, the grid point, the element, the layer, the position, then the texts (0
// for what the table does not give).
class PlaceOrder
{
public:
    explicit PlaceOrder(const MemoryTableInput& input) : m_input(input) {}

    [[nodiscard]] bool before(std::size_t left, std::size_t right) const
    {
        if (keyOf(left) != keyOf(right))
            return keyOf(left) < keyOf(right);
        for (const ResultText& text : m_input.texts)
        {
            const std::uint32_t leftCode = left < text.codes.size() ? text.codes[left] : 0;
            const std::uint32_t rightCode = right < text.codes.size() ? text.codes[right] : 0;
            if (leftCode != rightCode)
                return leftCode < rightCode;
        }
        return false;
    }

    // Adds to `repeated` the rows of `rows` (row numbers, sorted here) whose place an earlier row has: the sort is
    // stable, so the first row of a place is the one given first.
    void countRepeated(std::vector<std::size_t>& rows, RepeatedRows& repeated) const
    {
        std::stable_sort(rows.begin(), rows.end(),
                         [this](std::size_t left, std::size_t right) { return before(left, right); });
        for (std::size_t at = 1; at < rows.size(); ++at)
            if (!before(rows[at - 1], rows[at]))
            {
                ++repeated.count;
                repeated.firstRow = std::min(repeated.firstRow, rows[at]);
            }
    }

private:
    [[nodiscard]] std::tuple<std::int32_t, std::int32_t, std::int32_t, std::int32_t, double> keyOf(
        std::size_t row) const
    {
        return std::make_tuple(m_input.ids[row], row < m_input.nodeIds.size() ? m_input.nodeIds[row] : 0,
                               row < m_input.elementIds.size() ? m_input.elementIds[row] : 0,
                               row < m_input.layers.size() ? m_input.layers[row] : 0,
                               row < m_input.positions.size() ? m_input.positions[row] : 0.0);
    }

    const MemoryTableInput& m_input;
};

// Whether the rows `first` to `end` (excluded) have different pairs of grid point and layer, packed in `keys` and
// sorted by insertion (a run of rows of one ID is a few rows: ten for a CQUAD4 at its centre and corners).
bool distinctPairs(const MemoryTableInput& input, std::size_t first, std::size_t end, std::vector<std::uint64_t>& keys)
{
    constexpr std::size_t INSERTION_RUN = 32;
    keys.clear();
    for (std::size_t row = first; row < end; ++row)
    {
        const auto node = static_cast<std::uint32_t>(row < input.nodeIds.size() ? input.nodeIds[row] : 0);
        const auto layer = static_cast<std::uint32_t>(row < input.layers.size() ? input.layers[row] : 0);
        keys.push_back((std::uint64_t{node} << 32U) | layer);
    }
    if (keys.size() > INSERTION_RUN)
    {
        std::sort(keys.begin(), keys.end());
        return std::adjacent_find(keys.begin(), keys.end()) == keys.end();
    }
    for (std::size_t at = 1; at < keys.size(); ++at)
    {
        const std::uint64_t key = keys[at];
        std::size_t to = at;
        for (; to > 0 && keys[to - 1] >= key; --to)
        {
            if (keys[to - 1] == key)
                return false;
            keys[to] = keys[to - 1];
        }
        keys[to] = key;
    }
    return true;
}

// The rows that repeat a place. Rows of strictly increasing IDs (a nodal table, an element table of a row an element)
// cannot repeat; rows of IDs out of order are sorted by their place.
//
// PERFORMANCE: when the IDs do not decrease (the rows of an element together, as every decoder and a companion package
// give them), the places are compared within each run of rows of one ID, in place of a sort of every row: about 100 ns
// a row sorted, so that the 1 664 640 rows of the CQUAD4 corner stresses of a plate of 1 M DOFs took 0.17 s (a
// companion package P11.2). A run whose rows give no element, position or text compares its pairs of grid point and
// layer as integers; a run with a repeated pair, or with more than these, is sorted by the whole place, so that the
// count and the first repeated row are those the sort of every row gives.
RepeatedRows repeatedRows(const MemoryTableInput& input)
{
    const LabelArray<std::int32_t>& ids = input.ids;
    const std::size_t rowCount = ids.size();
    RepeatedRows repeated{0, rowCount};
    if (std::adjacent_find(ids.begin(), ids.end(), std::greater_equal<>()) == ids.end())
        return repeated;
    const PlaceOrder order(input);
    std::vector<std::size_t> rows;
    if (std::adjacent_find(ids.begin(), ids.end(), std::greater<>()) != ids.end())
    {
        rows.resize(rowCount);
        std::iota(rows.begin(), rows.end(), std::size_t{0});
        order.countRepeated(rows, repeated);
        return repeated;
    }
    const bool pairsOnly = input.elementIds.empty() && input.positions.empty() && input.texts.empty();
    std::vector<std::uint64_t> keys;
    for (std::size_t first = 0; first < rowCount;)
    {
        std::size_t end = first + 1;
        while (end < rowCount && ids[end] == ids[first])
            ++end;
        if (end - first > 1 && !(pairsOnly && distinctPairs(input, first, end, keys)))
        {
            rows.resize(end - first);
            std::iota(rows.begin(), rows.end(), first);
            order.countRepeated(rows, repeated);
        }
        first = end;
    }
    return repeated;
}

} // namespace

MemorySetFile::MemorySetFile(std::string label)
{
    m_metadata.format = "memory";
    m_metadata.origin = "memory";
    m_metadata.path = std::move(label);
    m_metadata.complete = true;
}

ResultTables MemorySetFile::tables(const ResultFilter& filter) const
{
    (void)filter;
    return ResultTables();
}

void MemorySetFile::setLabel(const std::string& label)
{
    m_metadata.path = label;
}

const std::vector<std::string>& memoryElementNames(ResultKind kind)
{
    return catalogueElementNames(kind);
}

const std::vector<std::string>& memoryComponents(ResultKind kind, std::string_view elementName, bool complex,
                                                 ResultVariant variant)
{
    (void)variant; // a random variant (PSD, RMS, ...) has the components of the plain table
    return catalogueComponents(kind, elementName, complex);
}

const std::vector<std::vector<std::string>>& memoryComponentLists(ResultKind kind, std::string_view elementName,
                                                                  bool complex)
{
    return catalogueComponentLists(kind, elementName, complex);
}

std::vector<TableLayout> memoryLayouts()
{
    return allTableLayouts();
}

std::vector<std::string> checkMemoryTable(const MemoryTableInput& input)
{
    std::vector<std::string> problems;
    const ResultKindInfo& info = resultKindInfo(input.kind);
    const std::string kindName(info.name);
    const bool complexType = isComplexType(input.type);

    // the container
    if (info.family)
    {
        const std::vector<std::string>& names = memoryElementNames(input.kind);
        if (input.elementName.empty())
            problems.push_back("the " + kindName + " of an element type or variant: give elementType (one of " +
                               join(names) + ")");
        else if (std::find(names.begin(), names.end(), input.elementName) == names.end())
            problems.push_back("'" + input.elementName + "' is not an element type (or variant) of " + kindName +
                               "; the ones this library knows are " + join(names));
    }
    else if (!input.elementName.empty())
    {
        problems.push_back(kindName + " has no element type: elementType is for the stresses, strains, forces, ...");
    }

    // the steps
    const std::int32_t analysisCode = input.analysisCode;
    const StepKind stepKind = input.stepKind != StepKind::None ? input.stepKind : stepKindOf(analysisCode);
    if (input.stepKind != StepKind::None && analysisCode != 0 && stepKindOf(analysisCode) != StepKind::None &&
        stepKindOf(analysisCode) != input.stepKind && !(input.stepKind == StepKind::LoadCombinationSet))
        problems.push_back("the analysis '" + std::string(analysisTypeName(analysisCode)) + "' has steps of kind " +
                           std::string(stepKindName(stepKindOf(analysisCode))) + ", not " +
                           std::string(stepKindName(input.stepKind)));
    const std::size_t stepCount = input.steps.size();
    if (stepCount == 0)
        problems.push_back("a table has at least one step (a static table has one, with the value NaN)");
    // an eigenvalue table (a row a mode) has one step with no value, whatever its analysis
    const bool valued = info.rows != RowKind::Mode && (stepKind == StepKind::Mode || stepKind == StepKind::Frequency ||
                                                       stepKind == StepKind::Time || stepKind == StepKind::LoadFactor);
    if (info.rows == RowKind::Mode && stepCount == 1 && !std::isnan(input.steps.front()))
        problems.push_back("a table of modes (" + kindName + ") has one step with no value (NaN)");
    if (!valued && stepCount > 1)
        problems.push_back("a table with no step values (a static analysis) has one step, not " +
                           std::to_string(stepCount));
    if (valued)
    {
        std::set<double> seen;
        for (const double step : input.steps)
        {
            if (std::isnan(step))
                problems.push_back("a step of kind " + std::string(stepKindName(stepKind)) + " has a value (not NaN)");
            else if (!seen.insert(step).second)
                problems.push_back("the step " + std::to_string(step) + " is given twice");
            if (!problems.empty() && problems.back().rfind("a step of kind", 0) == 0)
                break;
        }
    }
    const auto perStep = [&](const std::vector<double>& values, const char* name)
    {
        if (!values.empty() && values.size() != stepCount)
            problems.push_back(std::string(name) + " has " + std::to_string(values.size()) + " entries for " +
                               std::to_string(stepCount) + " steps");
    };
    perStep(input.eigenvalues, "eigenvalues");
    perStep(input.eigenvaluesImag, "eigenvaluesImag");
    perStep(input.cycles, "cycles");

    // the rows
    const std::size_t rowCount = input.ids.size();
    if (rowCount == 0)
        problems.push_back("a table has at least one row (ids is empty)");
    const auto perRow = [&](std::size_t count, const char* name)
    {
        if (count != 0 && count != rowCount)
            problems.push_back(std::string(name) + " has " + std::to_string(count) + " entries for " +
                               std::to_string(rowCount) + " rows");
    };
    perRow(input.pointTypes.size(), "pointTypes");
    perRow(input.nodeIds.size(), "nodeIds");
    perRow(input.elementIds.size(), "elementIds");
    perRow(input.layers.size(), "layers");
    perRow(input.positions.size(), "positions");
    for (const ResultText& text : input.texts)
        perRow(text.codes.size(), ("the text '" + text.name + "'").c_str());
    for (const ResultText& text : input.texts)
        for (const std::uint32_t code : text.codes)
            if (code >= text.dictionary.size())
            {
                problems.push_back("the text '" + text.name + "' has a code outside its dictionary");
                break;
            }
    // A row of the grid point forces is not a key: a file can list an element several times at one grid point (a
    // CELAS1 between two components of one grid point gives a row a component), and the readers keep them.
    if (rowCount != 0 && input.kind != ResultKind::NodePointForces)
    {
        const RepeatedRows repeated = repeatedRows(input);
        if (repeated.count > 0)
            problems.push_back(std::string(info.element ? "a place of an element is" : "an ID is") +
                               " given more than once: " + std::to_string(input.ids[repeated.firstRow]) + " (" +
                               std::to_string(repeated.count) + " repeated row" + (repeated.count > 1 ? "s" : "") +
                               ")");
    }

    // the components: a part of one of the lists the layouts of the kind can have (the default list, or one of the
    // lists as it is, has nothing to check)
    const std::vector<std::string>& known = memoryComponents(input.kind, input.elementName, complexType, input.variant);
    const std::vector<std::vector<std::string>>& lists =
        memoryComponentLists(input.kind, input.elementName, complexType);
    const std::vector<std::string>& wanted = input.components.empty() ? known : input.components;
    if (wanted.empty())
        problems.push_back("the components of " + kindName + " are not known: give components");
    const bool aWholeList = std::find(lists.begin(), lists.end(), wanted) != lists.end();
    bool allKnown = true;
    for (std::size_t at = 0; at < wanted.size() && !aWholeList; ++at)
    {
        const std::string& name = wanted[at];
        const bool inSomeList = std::any_of(lists.begin(), lists.end(), [&](const std::vector<std::string>& list)
                                            { return std::find(list.begin(), list.end(), name) != list.end(); });
        if (std::find(wanted.begin(), wanted.begin() + static_cast<std::ptrdiff_t>(at), name) !=
            wanted.begin() + static_cast<std::ptrdiff_t>(at))
        {
            problems.push_back("the component '" + name + "' is given twice");
        }
        else if (!lists.empty() && !inSomeList)
        {
            allKnown = false;
            std::string message = "'" + name + "' is not a component of " + kindName +
                                  (input.elementName.empty() ? "" : " " + input.elementName) + "; its components are " +
                                  join(known);
            for (std::size_t other = 1; other < lists.size(); ++other)
                message += (other == 1 ? " (or, in another layout: " : "; ") + join(lists[other]);
            problems.push_back(message + (lists.size() > 1 ? ")" : ""));
        }
    }
    const bool oneLayout =
        aWholeList || std::any_of(lists.begin(), lists.end(),
                                  [&](const std::vector<std::string>& list) { return containsAll(list, wanted); });
    if (allKnown && !lists.empty() && !wanted.empty() && !oneLayout)
        problems.push_back("the components " + join(wanted) + " are not those of one layout of " + kindName +
                           (input.elementName.empty() ? "" : " " + input.elementName) +
                           " (vonMises and maxShear, for instance, are never in one table)");

    // the values
    if (!complexType && input.storedFormat != ResultFormat::Real)
        problems.push_back(
            "a table written as real and imaginary parts or as magnitude and phase holds complex values");
    if (input.storedFormat == ResultFormat::Random || input.storedFormat == ResultFormat::ModalContribution)
        problems.push_back("storedFormat is real, realImaginary or magnitudePhase");
    const std::size_t expected = stepCount * rowCount * wanted.size() * valueTypeSize(input.type);
    if (input.values.size() != expected)
        problems.push_back("the values hold " + std::to_string(input.values.size()) + " bytes where " +
                           std::to_string(stepCount) + " steps x " + std::to_string(rowCount) + " rows x " +
                           std::to_string(wanted.size()) + " components of " + std::string(valueTypeName(input.type)) +
                           " need " + std::to_string(expected));
    if (input.sharedValues &&
        (static_cast<const void*>(input.sharedValues.get()) != static_cast<const void*>(input.values.data()) ||
         reinterpret_cast<std::uintptr_t>(input.values.data()) % alignof(double) != 0))
        problems.push_back("the block of the values to keep does not start at the values, on 8 bytes");
    return problems;
}

std::shared_ptr<ResultTable> makeMemoryTable(MemoryTableInput input)
{
    const std::vector<std::string> problems = checkMemoryTable(input);
    if (!problems.empty())
    {
        std::string message = "cannot make a " + std::string(resultKindName(input.kind)) + " table:";
        for (const std::string& problem : problems)
            message += "\n  " + problem;
        throw base::UsageError(message);
    }
    const ResultKindInfo& info = resultKindInfo(input.kind);
    const bool complexType = isComplexType(input.type);
    auto data = std::make_shared<ResultData>();
    data->type = input.type;
    data->stepCount = input.steps.size();
    data->rowCount = input.ids.size();
    data->components = input.components.empty()
                           ? memoryComponents(input.kind, input.elementName, complexType, input.variant)
                           : std::move(input.components);
    data->componentCount = data->components.size();
    if (input.sharedValues)
    {
        data->storage = std::move(input.sharedValues);
    }
    else
    {
        data->allocate();
        if (!input.values.empty())
            std::memcpy(data->mutableBytes(), input.values.data(), input.values.size());
    }
    data->steps = std::move(input.steps);
    data->eigenvalues = std::move(input.eigenvalues);
    data->eigenvaluesImag = std::move(input.eigenvaluesImag);
    data->cycles = std::move(input.cycles);
    data->ids = std::move(input.ids);
    data->pointTypes = std::move(input.pointTypes);
    data->nodeIds = std::move(input.nodeIds);
    data->elementIds = std::move(input.elementIds);
    data->layers = std::move(input.layers);
    if (info.element && data->layers.empty())
        data->layers.assign(data->rowCount, 1);
    data->positions = std::move(input.positions);
    // a grid point unless the table says otherwise
    if (info.rows == RowKind::GridPoint && data->pointTypes.empty())
        data->pointTypes.assign(data->rowCount, 1);
    data->texts = std::move(input.texts);

    ResultTableMeta meta;
    meta.kind = input.kind;
    meta.variant = input.variant;
    meta.designCycle = input.designCycle;
    meta.superelement = input.superelement;
    meta.subcase = input.subcase;
    meta.loadSet = input.loadSet;
    const StepKind stepKind = input.stepKind != StepKind::None ? input.stepKind : stepKindOf(input.analysisCode);
    std::int32_t analysisCode = input.analysisCode;
    if (analysisCode == 0)
    {
        switch (stepKind)
        {
        case StepKind::Mode:
            analysisCode = 2;
            break;
        case StepKind::Frequency:
            analysisCode = 5;
            break;
        case StepKind::Time:
            analysisCode = 6;
            break;
        case StepKind::LoadFactor:
            analysisCode = 10;
            break;
        default:
            analysisCode = 1;
            break;
        }
    }
    meta.analysisCode = analysisCode;
    meta.stepKind = stepKind == StepKind::None ? StepKind::LoadCombinationSet : stepKind;
    // a table of modes (an eigenvalue table: a row a mode) has one step with no value and no step kind, as the
    // decoders give it
    if (info.rows == RowKind::Mode)
        meta.stepKind = StepKind::None;
    if (info.family)
    {
        if (info.element)
        {
            meta.elementName = input.elementName;
            meta.layout = rowLayoutOf(input.elementName, std::as_const(*data).nodeIds);
        }
        else
        {
            meta.subName = input.elementName;
        }
    }
    // a random variant (PSD, RMS, ...) holds real values, as the decoded random tables say
    meta.valueKind = complexType ? ValueKind::Complex : ValueKind::Real;
    meta.storedFormat =
        complexType && input.storedFormat == ResultFormat::Real ? ResultFormat::RealImaginary : input.storedFormat;
    meta.valueType = input.type;
    meta.wordSize = input.type == ValueType::Float64 || input.type == ValueType::Complex128 ? 8 : 4;
    meta.status = ResultStatus::NotValidated;
    meta.storedFrame = input.storedFrame;
    if (meta.storedFrame == StoredFrame::Unknown && info.nodal)
        meta.storedFrame = StoredFrame::NodeOutputSystem;
    meta.frameSystem = input.frameSystem;
    meta.units = input.units;
    meta.title = input.title;
    meta.subtitle = input.subtitle;
    meta.label = input.label;
    meta.tableName = input.tableName;
    meta.origin = "memory";
    return std::make_shared<ResultTable>(std::move(meta), std::shared_ptr<const ResultData>(std::move(data)));
}

void addTableToResults(ResultTables& tables, std::shared_ptr<ResultTable> table)
{
    const ResultTableMeta& meta = table->meta();
    const ResultKindInfo& info = resultKindInfo(meta.kind);
    if (info.family)
        tables.mutableElementContainer(meta.kind, info.element ? meta.elementName : meta.subName, meta.variant)
            .add(std::move(table));
    else
        tables.mutableContainer(meta.kind, meta.variant).add(std::move(table));
}

void addMemoryTables(ResultTables& tables, MemoryTableIndex& index, std::span<const std::shared_ptr<ResultTable>> batch)
{
    std::set<MemoryTableKey> added;
    for (std::size_t position = 0; position < batch.size(); ++position)
    {
        const MemoryTableKey key = MemoryTableIndex::keyOf(batch[position]->meta());
        const bool inSet = index.contains(key);
        if (inSet || !added.insert(key).second)
            throw base::UsageError(
                std::string(batch.size() > 1 ? "table " + std::to_string(position) + " of the batch: " : "") +
                "the set has a table of " + MemoryTableIndex::describe(key) +
                (inSet ? " already" : " earlier in the batch") +
                ": give another subcase, designCycle, loadSet or tableName, or make another set");
    }
    for (const std::shared_ptr<ResultTable>& table : batch)
    {
        index.insert(MemoryTableIndex::keyOf(table->meta()));
        addTableToResults(tables, table);
    }
}

} // namespace oor::results
