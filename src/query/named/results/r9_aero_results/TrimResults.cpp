#include "../NamedTables.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"

#include <algorithm>
#include <tuple>

namespace oor::query
{

namespace
{

// aeroTrimResults: the trim variables of a static aeroelastic analysis and their trimmed values.
class TrimResultsQuery final : public ResultQuery
{
public:
    TrimResultsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        std::vector<ResultQueryContext::NamedRef> results = context.named("aeroTrimVariables");
        std::stable_sort(results.begin(), results.end(),
                         [](const auto& a, const auto& b)
                         {
                             return std::tie(a.result->meta().designCycle, a.set, a.result->meta().subcase) <
                                    std::tie(b.result->meta().designCycle, b.set, b.result->meta().subcase);
                         });
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const ResultQueryContext::NamedRef& ref : results)
        {
            const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
            const results::ResultArray* type = data->find("variableType");
            const results::ResultArray* fixed = data->find("isFixed");
            const results::ResultArray* value = data->find("value");
            const results::ResultText* name = data->findText("variable");
            if (type == nullptr || fixed == nullptr || value == nullptr || name == nullptr)
                continue;
            const std::vector<std::int32_t> kinds = arrayInts(*type);
            const std::vector<std::int32_t> flags = arrayInts(*fixed);
            const std::size_t count = kinds.size();
            std::vector<std::string> kind(count);
            for (std::size_t row = 0; row < count; ++row)
                kind[row] = kinds[row] == 1 ? "rigidBody" : (kinds[row] == 2 ? "controlSurface" : "user");
            RowBlock block(count);
            block.ints("subcase", std::vector<std::int32_t>(count, ref.result->meta().subcase));
            block.texts("variable", name->values());
            block.texts("variableKind", std::move(kind));
            block.reals("value", arrayDoubles(*value));
            block.flags("isFixed", std::vector<std::uint8_t>(flags.begin(), flags.end()));
            block.reals("mach", std::vector<double>(count, attributeOf(*ref.result, "mach")));
            block.reals("dynamicPressure", std::vector<double>(count, attributeOf(*ref.result, "dynamicPressure")));
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
        info.name = "aeroTrimResults";
        info.family = "R9";
        info.resultQuery = true;
        info.description =
            "The trim variables of a static aeroelastic analysis, one row for each subcase and variable: its kind "
            "(`rigidBody` for the angle of attack, the rates and the load factor, `controlSurface`, else `user`), "
            "its value "
            "(the trimmed value of a free variable), whether it was fixed, and the Mach number and the dynamic "
            "pressure of "
            "the trim condition when the table says them. The aeroelastic tables are `experimental` when read from a "
            "file and "
            "`notValidated` when built in memory (`table.sources` says).";
        info.defaultSort = "subcase";
        ColumnListBuilder columns;
        columns.int32("subcase").dictionary("variable").dictionary("variableKind").result("value").flag("isFixed");
        columns.real("mach").real("dynamicPressure");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addTrimResultsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<TrimResultsQuery>());
}

} // namespace oor::query
