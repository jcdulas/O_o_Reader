#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r3_forces/ForceQueries.hpp"
#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

// Grid-point force source names use the neutral result element type.
constexpr std::array<std::pair<std::string_view, std::string_view>, 6> LINE_TYPES{{{"Bar2", "Bar2"},
                                                                                   {"Beam2", "Beam2"},
                                                                                   {"CurvedBeam2", "bendElem"},
                                                                                   {"Rod2", "Rod2"},
                                                                                   {"Rod2InlineSection", "directRod2"},
                                                                                   {"Tube2", "Tube2"}}};
std::string forceName(const std::string& card)
{
    for (const auto& [type, source] : LINE_TYPES)
        if (card == type)
            return std::string(source);
    return card;
}
std::string cardTypeOfForceName(const std::string& source)
{
    for (const auto& [type, name] : LINE_TYPES)
        if (source == name)
            return std::string(type);
    return source;
}
const std::set<std::string>& lineNames()
{
    static const std::set<std::string> NAMES = {"Bar2", "Beam2",      "bendElem",
                                                "Rod2", "directRod2", "Tube2"};
    return NAMES;
}

// barEndLoads: what the one-dimensional elements push on their end grid points, from the grid point forces.
class BarEndLoadsQuery final : public ResultQuery
{
public:
    BarEndLoadsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const ElementRequest request = ElementRequest::from(context.arguments());
        std::set<std::string> wantedNames;
        for (const std::string& card : request.types)
            wantedNames.insert(forceName(card));
        std::vector<TableRef> tables = context.tables(results::ResultKind::NodePointForces);
        sortTables(tables);
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("grid point force table of the result file", false);
        ResultModelIndex index(context);
        bool anyTable = !tables.empty();
        for (const TableRef& ref : tables)
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
            if (!decoded)
                continue;
            const TableValues values(*decoded);
            const results::ResultText* names = nullptr;
            for (const results::ResultText& text : decoded->texts)
                if (text.name == "elementName")
                    names = &text;
            if (names == nullptr || decoded->elementIds.empty())
                continue;
            int component[6];
            const char* const columns[6] = {"f1", "f2", "f3", "m1", "m2", "m3"};
            bool complete = true;
            for (int number = 0; number < 6; ++number)
            {
                component[number] = values.component(columns[number]);
                complete = complete && component[number] >= 0;
            }
            if (!complete)
                continue;
            std::vector<std::size_t> kept;
            for (std::size_t row = 0; row < values.rows(); ++row)
            {
                const std::int32_t element = decoded->elementIds[row];
                const std::string& name = names->at(row);
                if (element <= 0 || !lineNames().contains(name) || !request.wantsElement(element))
                    continue;
                if (!wantedNames.empty() && !wantedNames.contains(name))
                    continue;
                if (request.propertyIds && !request.wantsProperty(context.elementCard(ref.set, element).pid))
                    continue;
                kept.push_back(row);
            }
            if (kept.empty())
                continue;
            // the ends of each element from its card: the first and second node
            std::map<std::int32_t, std::array<std::int32_t, 2>> endsOf;
            if (context.modelOf(ref.set) != nullptr)
            {
                CardFacts facts(*context.modelOf(ref.set));
                for (const std::size_t row : kept)
                {
                    const std::int32_t element = decoded->elementIds[row];
                    if (endsOf.contains(element))
                        continue;
                    std::array<std::int32_t, 2> ends = {0, 0};
                    if (const auto location = index.element(ref.set, element))
                    {
                        const std::vector<base::EntityId> nodes =
                            facts.references(*location, model::ReferenceTarget::Node);
                        for (std::size_t position = 0; position < 2 && position < nodes.size(); ++position)
                            ends[position] = nodes[position];
                    }
                    endsOf[element] = ends;
                }
            }
            const std::size_t count = values.steps() * kept.size();
            RowBlock block(count);
            std::vector<std::int32_t> eid;
            std::vector<std::string> type;
            std::vector<std::int32_t> pid;
            std::vector<std::uint8_t> pidValid;
            std::vector<std::string> end;
            std::vector<std::uint8_t> endValid;
            std::vector<std::int32_t> nid;
            std::vector<std::size_t> stepOf;
            std::array<std::vector<double>, 6> real;
            std::array<std::vector<double>, 6> imaginary;
            for (std::size_t step = 0; step < values.steps(); ++step)
                for (const std::size_t row : kept)
                {
                    const std::int32_t element = decoded->elementIds[row];
                    eid.push_back(element);
                    type.push_back(cardTypeOfForceName(names->at(row)));
                    const std::int32_t property = context.elementCard(ref.set, element).pid;
                    pid.push_back(property);
                    pidValid.push_back(property > 0 ? 1 : 0);
                    const std::int32_t node = decoded->ids[row];
                    nid.push_back(node);
                    std::string which;
                    if (const auto found = endsOf.find(element); found != endsOf.end())
                    {
                        const std::int32_t modelNode = context.modelNodeId(ref.set, node);
                        if (modelNode != 0 && modelNode == found->second[0])
                            which = "a";
                        else if (modelNode != 0 && modelNode == found->second[1])
                            which = "b";
                    }
                    end.push_back(which);
                    endValid.push_back(which.empty() ? 0 : 1);
                    stepOf.push_back(step);
                    for (std::size_t number = 0; number < 6; ++number)
                    {
                        real[number].push_back(values.real(step, row, static_cast<std::size_t>(component[number])));
                        if (values.isComplex())
                            imaginary[number].push_back(
                                values.imag(step, row, static_cast<std::size_t>(component[number])));
                    }
                }
            block.ints("subcase", std::vector<std::int32_t>(count, meta.subcase));
            block.ints("elementId", std::move(eid));
            block.texts("type", std::move(type));
            block.ints("propId", std::move(pid), std::move(pidValid));
            block.texts("end", std::move(end), std::move(endValid));
            block.ints("nodeId", std::move(nid));
            static const char* const NAMES6[6] = {"fx", "fy", "fz", "mx", "my", "mz"};
            for (std::size_t number = 0; number < 6; ++number)
            {
                if (values.isComplex())
                    block.complex(NAMES6[number], std::move(real[number]), std::move(imaginary[number]));
                else
                    block.reals(NAMES6[number], std::move(real[number]));
            }
            block.texts("frame", std::vector<std::string>(count, context.frameNameOf(ref)));
            addStepsToBlock(block, meta, *decoded, stepOf);
            block.origin(ref.set, context.sets()[ref.set].analysis, meta.designCycle);
            block.precision(precisionOf(*decoded));
            block.storedAsMagnitudePhase(meta.storedFormat == results::ResultFormat::MagnitudePhase);
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
        {
            if (!anyTable && context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(
                    "the result file has no grid point force table (output of GPFORCE was not "
                    "requested): the loads of the elements on their ends are not stored anywhere else");
            return emptyTable(info().defaultColumns());
        }
        std::vector<std::string> keys = leadingKeys();
        keys.insert(keys.end(), {"elementId", "end", "nodeId"});
        return sortedResult(accumulator.finish(context, info()), context.exec(), keys);
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "barEndLoads";
        info.family = "R3";
        info.resultQuery = true;
        info.description =
            "The forces and moments that the one-dimensional elements (bars, beams, bends, rods) push on their end "
            "grid points, "
            "from the grid point force table: one row for each subcase, step, element and end, with the frame the "
            "table "
            "writes them in (the displacement system of the grid point as stored, or the frame the `frame` parameter "
            "turned "
            "them to). `end` is `a` or `b` when the results are attached to a model that gives the ends of the "
            "element. "
            "Where the document of the queries says \"in the element system as stored\", the grid point force table "
            "does "
            "not store them in the element system: `frame` says which one it is.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "the elements to list"),
                           parameter("types", ParameterKind::TextList, "None", "the element types (\"Bar2\", ...)")};
        info.defaultSort = "subcase, [step], elementId, end";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .step()
            .int32("elementId")
            .dictionary("type")
            .int32("propId")
            .dictionary("end")
            .int32("nodeId");
        columns.results("fx fy fz mx my mz").dictionary("frame");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addBarEndLoadsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<BarEndLoadsQuery>());
}

} // namespace oor::query
