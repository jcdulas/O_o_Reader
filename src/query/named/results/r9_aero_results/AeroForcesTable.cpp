#include "../NamedTables.hpp"
#include "AeroModelLinks.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

// What the provenance says of the `elementId` of the rows. A row whose tables cannot be paired has no box to look up.
const LookupWords WORDS{"elementId", "CAERO of the model that owns the box of the same row of the pressure table",
                        "no CAERO of the model owns the box of the same row of the pressure table: elementId is null",
                        "the result set has no model: elementId is null", ""};
constexpr const char* NOT_PAIRED =
    "the forces cannot be paired with a pressure table of the subcase (same rows, same element kinds): elementId is "
    "null";

// aeroForcesTable: the force and the moment on each aerodynamic grid point.
class AeroForcesTableQuery final : public ResultQuery
{
public:
    AeroForcesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const auto boxes = context.arguments().ids("boxIds");
        std::set<std::int64_t> wanted;
        if (boxes)
            wanted.insert(boxes->begin(), boxes->end());
        std::vector<ResultQueryContext::NamedRef> results = context.named("aeroForces");
        std::stable_sort(results.begin(), results.end(),
                         [](const auto& a, const auto& b)
                         {
                             return std::tie(a.result->meta().designCycle, a.set, a.result->meta().subcase) <
                                    std::tie(b.result->meta().designCycle, b.set, b.result->meta().subcase);
                         });
        const std::vector<ResultQueryContext::NamedRef> pressures = context.named("aeroPressures");
        AeroModelLinks links(context);
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const ResultQueryContext::NamedRef& ref : results)
        {
            const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
            const results::ResultArray* grid = data->find("nodeId");
            const results::ResultArray* forces = data->find("forces");
            if (grid == nullptr || forces == nullptr || forces->shape.size() != 2 || forces->shape[1] != 6)
                continue;
            const std::vector<std::int32_t> ids = arrayInts(*grid);
            const std::vector<double> numbers = arrayDoubles(*forces);
            std::vector<std::size_t> kept;
            for (std::size_t row = 0; row < ids.size(); ++row)
                if (!boxes || wanted.contains(ids[row]))
                    kept.push_back(row);
            if (kept.empty())
                continue;
            std::vector<std::int32_t> box;
            std::array<std::vector<double>, 6> value;
            for (const std::size_t row : kept)
            {
                box.push_back(ids[row]);
                for (std::size_t component = 0; component < 6; ++component)
                    value[component].push_back(numbers[row * 6 + component]);
            }
            IdLookup panels = panelsOf(context, links, accumulator, ref, *data, pressures, kept);
            RowBlock block(kept.size());
            block.ints("subcase", std::vector<std::int32_t>(kept.size(), ref.result->meta().subcase));
            block.ints("boxId", std::move(box));
            block.ints("elementId", std::move(panels.id), std::move(panels.valid));
            static const char* const NAMES[6] = {"t1", "t2", "t3", "r1", "r2", "r3"};
            for (std::size_t component = 0; component < 6; ++component)
                block.reals(NAMES[component], std::move(value[component]));
            block.texts("frame", std::vector<std::string>(kept.size(), "aerodynamic"));
            block.origin(ref.set, context.sets()[ref.set].analysis, ref.result->meta().designCycle);
            block.precision(precisionOfNamed(*data));
            block.validation(statusOfNamed(*ref.result));
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    // The CAERO of each kept row. The forces are on an internal number, so the row is paired with the same row of the
    // pressure table of its subcase (and design cycle), which names the box; the pairing is refused unless the two
    // tables list the same number of elements of the same kinds in the same order.
    static IdLookup panelsOf(const ResultQueryContext& context, AeroModelLinks& links, RowAccumulator& accumulator,
                             const ResultQueryContext::NamedRef& forces, const results::NamedResultData& forceData,
                             const std::vector<ResultQueryContext::NamedRef>& pressures,
                             const std::vector<std::size_t>& kept)
    {
        IdLookup none;
        none.id.assign(kept.size(), 0);
        none.valid.assign(kept.size(), 0);
        if (!links.hasModel(forces.set))
        {
            noteLookup(accumulator, links, forces.set, none, kept.size(), WORDS);
            return none;
        }
        std::vector<std::int32_t> paired;
        for (const ResultQueryContext::NamedRef& pressure : pressures)
        {
            if (pressure.set != forces.set || pressure.result->meta().subcase != forces.result->meta().subcase ||
                pressure.result->meta().designCycle != forces.result->meta().designCycle)
                continue;
            const std::shared_ptr<const results::NamedResultData> data = pressure.result->data(context.decode());
            const results::ResultArray* element = data->find("elementId");
            const results::ResultText* pressureKinds = data->findText("elementKind");
            const results::ResultText* forceKinds = forceData.findText("elementKind");
            if (element == nullptr || pressureKinds == nullptr || forceKinds == nullptr)
                continue;
            const std::vector<std::int32_t> elementIds = arrayInts(*element);
            paired = boxesOfForces(elementIds, pressureKinds->values(), forceKinds->values());
            break;
        }
        if (paired.empty())
        {
            accumulator.note(accumulator.addProvider(NOT_PAIRED, false), {WORDS.column},
                             static_cast<std::int64_t>(kept.size()));
            return none;
        }
        std::vector<std::int32_t> boxes;
        boxes.reserve(kept.size());
        for (const std::size_t row : kept)
            boxes.push_back(paired[row]);
        IdLookup panels = links.panelsOfBoxes(forces.set, boxes);
        noteLookup(accumulator, links, forces.set, panels, kept.size(), WORDS);
        return panels;
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroForcesTable";
        info.family = "R9";
        info.resultQuery = true;
        info.description =
            "The force and the moment on each aerodynamic grid point, for each subcase. `boxId` is the number the "
            "table gives the point (an internal number that follows the order of the pressure table); `elementId`, the "
            "panel (CAERO1 to CAERO5), is the one that owns the box of the same row of the pressure table of the "
            "subcase, when the two tables list the same elements of the same kinds in the same order. It is null when "
            "the result set has no model, when the tables cannot be paired and for a box no panel of the model owns; "
            "the provenance of the table says which. `frame` is `aerodynamic`, the table not saying more.";
        info.parameters = {parameter("boxIds", ParameterKind::IdList, "None", "the points to list")};
        info.defaultSort = "subcase, boxId";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("boxId").int32("elementId").results("t1 t2 t3 r1 r2 r3").dictionary("frame");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addAeroForcesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<AeroForcesTableQuery>());
}

} // namespace oor::query
