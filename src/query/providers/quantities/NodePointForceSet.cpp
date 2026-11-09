#include "oor/query/providers/quantities/NodePointForceSet.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/results/TableValues.hpp"

#include <algorithm>
#include <optional>

namespace oor::query
{

NodePointForceKind nodePointForceKindOf(std::string_view label) noexcept
{
    // The labels of the fixed rows, by their neutral names (NodePointForceKind), compared exactly.
    const std::string_view text = base::trimAscii(label);
    if (text == "appliedLoad")
        return NodePointForceKind::AppliedLoad;
    if (text == "singlePointConstrForce")
        return NodePointForceKind::Spc;
    if (text == "multiPointConstrForce")
        return NodePointForceKind::Mpc;
    if (text == "total")
        return NodePointForceKind::Total;
    return NodePointForceKind::Other;
}

std::span<const std::uint32_t> NodePointForceLayout::rowsAt(std::int32_t modelNode) const noexcept
{
    const auto found = rowsOfNode.find(modelNode);
    if (found == rowsOfNode.end())
        return {};
    return found->second;
}

namespace
{

// What a table gives on a thread: the table but its layout, and what its rows are in the file (the layout is linked to
// the model on the calling thread, once for the tables with the same rows).
struct DecodedTable
{
    std::optional<NodePointForceTable> table;
    std::vector<std::int32_t> ids;         // the grid points in the file
    std::vector<std::int32_t> elementIds;  // the elements in the file (0: not an element row)
    std::vector<NodePointForceKind> kinds; // Element for the rows of an element, else by the label
};

DecodedTable decodeTable(const ResultQueryContext& context, const TableRef& ref)
{
    static constexpr std::array<const char*, 6> COMPONENTS = {"f1", "f2", "f3", "m1", "m2", "m3"};
    DecodedTable result;
    {
        const results::ResultTableMeta& meta = ref.table->meta();
        const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
        const TableValues values(*decoded);
        std::array<int, 6> component{};
        bool complete = true;
        for (std::size_t number = 0; number < 6; ++number)
        {
            component[number] = values.component(COMPONENTS[number]);
            complete = complete && component[number] >= 0;
        }
        if (!complete)
            return result;
        const results::ResultText* labels = nullptr;
        for (const results::ResultText& text : decoded->texts)
            if (text.name == "elementName")
                labels = &text;
        NodePointForceTable table;
        table.set = ref.set;
        table.analysis = context.sets()[ref.set].analysis;
        table.subcase = meta.subcase;
        table.designCycle = meta.designCycle;
        table.stepKind = meta.stepKind;
        table.precision = precisionOf(*decoded);
        table.isComplex = values.isComplex();
        table.magnitudePhase = meta.storedFormat == results::ResultFormat::MagnitudePhase;
        table.rowCount = values.rows();
        const bool stepped = meta.stepKind == results::StepKind::Time ||
                             meta.stepKind == results::StepKind::Frequency ||
                             meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
        if (!stepped)
            table.stepKind = results::StepKind::None;
        for (std::size_t step = 0; step < values.steps(); ++step)
        {
            table.steps.push_back(stepped && step < decoded->steps.size() ? decoded->steps[step] : std::nan(""));
            table.eigenvalues.push_back(step < decoded->eigenvalues.size() ? decoded->eigenvalues[step] : std::nan(""));
        }
        result.ids.assign(decoded->ids.begin(), decoded->ids.begin() + static_cast<std::ptrdiff_t>(table.rowCount));
        result.elementIds.resize(table.rowCount);
        result.kinds.resize(table.rowCount);
        // an F-OF-MPC row that gives an ID is the row of one relation, not the row of an element: the
        // kind of each text of the dictionary, once
        std::vector<NodePointForceKind> kindOfCode;
        if (labels != nullptr)
            for (const std::string& text : labels->dictionary)
                kindOfCode.push_back(nodePointForceKindOf(text));
        const auto labelKind = [&](std::size_t row)
        {
            const std::uint32_t code = labels->codes[row];
            return code < kindOfCode.size() ? kindOfCode[code] : NodePointForceKind::Other;
        };
        for (std::size_t row = 0; row < table.rowCount; ++row)
        {
            const std::int32_t element = row < decoded->elementIds.size() ? decoded->elementIds[row] : 0;
            result.elementIds[row] = element;
            if (element != 0 && labels != nullptr && labelKind(row) == NodePointForceKind::Mpc)
                result.kinds[row] = NodePointForceKind::Mpc;
            else
                result.kinds[row] = element > 0         ? NodePointForceKind::Element
                                    : labels != nullptr ? labelKind(row)
                                                        : NodePointForceKind::Other;
        }
        const std::size_t count = values.steps() * table.rowCount * 6;
        table.real.resize(count);
        if (table.isComplex)
            table.imaginary.resize(count);
        for (std::size_t step = 0; step < values.steps(); ++step)
            for (std::size_t row = 0; row < table.rowCount; ++row)
                for (std::size_t number = 0; number < 6; ++number)
                {
                    const std::size_t at = (step * table.rowCount + row) * 6 + number;
                    const auto column = static_cast<std::size_t>(component[number]);
                    table.real[at] = values.real(step, row, column);
                    if (table.isComplex)
                        table.imaginary[at] = values.imag(step, row, column);
                }
        result.table = std::move(table);
    }
    return result;
}

// The rows of a table linked to the model.
std::shared_ptr<const NodePointForceLayout> linkLayout(const ResultQueryContext& context, std::size_t set,
                                                       const DecodedTable& decoded)
{
    auto layout = std::make_shared<NodePointForceLayout>();
    const std::size_t rows = decoded.ids.size();
    layout->node.resize(rows);
    layout->element.resize(rows);
    layout->kind = decoded.kinds;
    for (std::size_t row = 0; row < rows; ++row)
    {
        layout->node[row] = context.modelNodeId(set, decoded.ids[row]);
        if (decoded.kinds[row] == NodePointForceKind::Mpc && decoded.elementIds[row] != 0)
        {
            if (layout->relation.empty())
                layout->relation.assign(rows, 0);
            layout->relation[row] = decoded.elementIds[row];
        }
        else if (decoded.elementIds[row] > 0)
        {
            const ResultQueryContext::ElementCard card = context.elementCard(set, decoded.elementIds[row]);
            layout->element[row] = card.linked ? card.modelId : 0;
            if (card.linked)
                layout->elements.insert(card.modelId);
        }
        if (layout->node[row] > 0)
            layout->rowsOfNode[layout->node[row]].push_back(static_cast<std::uint32_t>(row));
    }
    return layout;
}

} // namespace

std::shared_ptr<const NodePointForceSet> NodePointForceSet::read(const ResultQueryContext& context)
{
    auto set = std::make_shared<NodePointForceSet>();
    std::vector<TableRef> refs = context.resultantTables(results::ResultKind::NodePointForces);
    sortTables(refs);
    // the tables decoded on the threads of the call; a table with the rows of the one before (the same result set, grid
    // points, elements and labels) takes its layout
    DecodedTable before;
    std::shared_ptr<const NodePointForceLayout> beforeLayout;
    makeInOrder<DecodedTable>(
        context, refs.size(), [&](std::size_t index) { return indexedRows(refs[index]); },
        [&](std::size_t index) { return decodeTable(context, refs[index]); },
        [&](std::size_t, DecodedTable&& decoded)
        {
            if (!decoded.table)
                return;
            NodePointForceTable& table = *decoded.table;
            if (!beforeLayout || before.table->set != table.set || before.ids != decoded.ids ||
                before.elementIds != decoded.elementIds || before.kinds != decoded.kinds)
                beforeLayout = linkLayout(context, table.set, decoded);
            table.layout = beforeLayout;
            set->m_tables.push_back(std::move(table));
            before.table.emplace();
            before.table->set = set->m_tables.back().set;
            before.ids = std::move(decoded.ids);
            before.elementIds = std::move(decoded.elementIds);
            before.kinds = std::move(decoded.kinds);
        });
    return set;
}

} // namespace oor::query
