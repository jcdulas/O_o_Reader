#include "../NamedTables.hpp"
#include "AeroModelLinks.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"

#include <algorithm>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

// What the provenance says of the `elementId` of the rows.
const LookupWords WORDS{"elementId", "CAERO of the model that owns the box",
                        "no CAERO of the model owns the box: elementId is null",
                        "the result set has no model: elementId is null", ""};

// aeroPressuresTable: the pressure coefficient and the pressure on each aerodynamic box.
class AeroPressuresTableQuery final : public ResultQuery
{
public:
    AeroPressuresTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const auto boxes = context.arguments().ids("boxIds");
        std::set<std::int64_t> wanted;
        if (boxes)
            wanted.insert(boxes->begin(), boxes->end());
        std::vector<ResultQueryContext::NamedRef> results = context.named("aeroPressures");
        std::stable_sort(results.begin(), results.end(),
                         [](const auto& a, const auto& b)
                         {
                             return std::tie(a.result->meta().designCycle, a.set, a.result->meta().subcase) <
                                    std::tie(b.result->meta().designCycle, b.set, b.result->meta().subcase);
                         });
        AeroModelLinks links(context);
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const ResultQueryContext::NamedRef& ref : results)
        {
            const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
            const results::ResultArray* element = data->find("elementId");
            const results::ResultArray* coefficient = data->find("pressureCoefficient");
            const results::ResultArray* pressure = data->find("pressure");
            if (element == nullptr || coefficient == nullptr || pressure == nullptr)
                continue;
            const std::vector<std::int32_t> ids = arrayInts(*element);
            const std::vector<double> cp = arrayDoubles(*coefficient);
            const std::vector<double> p = arrayDoubles(*pressure);
            std::vector<std::size_t> kept;
            for (std::size_t row = 0; row < ids.size(); ++row)
                if (!boxes || wanted.contains(ids[row]))
                    kept.push_back(row);
            if (kept.empty())
                continue;
            std::vector<std::int32_t> box;
            std::vector<double> coefficients, pressures;
            for (const std::size_t row : kept)
            {
                box.push_back(ids[row]);
                coefficients.push_back(cp[row]);
                pressures.push_back(p[row]);
            }
            // the panel that owns each box, found in the model the results are attached to
            IdLookup panels = links.panelsOfBoxes(ref.set, box);
            noteLookup(accumulator, links, ref.set, panels, kept.size(), WORDS);
            RowBlock block(kept.size());
            block.ints("subcase", std::vector<std::int32_t>(kept.size(), ref.result->meta().subcase));
            block.ints("boxId", std::move(box));
            block.ints("elementId", std::move(panels.id), std::move(panels.valid));
            block.reals("pressureCoefficient", std::move(coefficients));
            block.reals("pressure", std::move(pressures));
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
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroPressuresTable";
        info.family = "R9";
        info.resultQuery = true;
        info.description =
            "The pressure coefficient and the pressure on each aerodynamic box, for each subcase. `boxId` is the "
            "number the table gives the box; `elementId`, the aerodynamic panel (CAERO1 to CAERO5) the box belongs to, "
            "is "
            "the card of the model whose boxes (numbered from its EID on) hold `boxId`. It is null when the result "
            "set "
            "has no model and for a box that no panel of the model owns; the provenance of the table says which.";
        info.parameters = {parameter("boxIds", ParameterKind::IdList, "None", "the boxes to list")};
        info.defaultSort = "subcase, boxId";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("boxId").int32("elementId").results("pressureCoefficient pressure");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addAeroPressuresTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<AeroPressuresTableQuery>());
}

} // namespace oor::query
