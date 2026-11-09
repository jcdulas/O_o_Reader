#include "../NamedTables.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"

#include <algorithm>
#include <tuple>

namespace oor::query
{

namespace
{

// stabilityDerivativesTable: the six coefficients of each trim variable, rigid, elastic and inertial.
class StabilityDerivativesTableQuery final : public ResultQuery
{
public:
    StabilityDerivativesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        std::vector<ResultQueryContext::NamedRef> results = context.named("stabilityDerivatives");
        std::stable_sort(results.begin(), results.end(),
                         [](const auto& a, const auto& b)
                         {
                             return std::tie(a.result->meta().designCycle, a.set, a.result->meta().subcase) <
                                    std::tie(b.result->meta().designCycle, b.set, b.result->meta().subcase);
                         });
        static const char* const COEFFICIENTS[6] = {"cx", "cy", "cz", "cmx", "cmy", "cmz"};
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const ResultQueryContext::NamedRef& ref : results)
        {
            const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
            const results::ResultArray* values = data->find("values");
            const results::ResultText* variables = data->findText("variable");
            if (values == nullptr || variables == nullptr || values->shape.size() != 3 || values->shape[1] != 6 ||
                values->shape[2] != 6)
                continue;
            const std::vector<double> numbers = arrayDoubles(*values);
            const std::size_t count = values->shape[0] * 6;
            std::vector<std::string> variable(count);
            std::vector<std::string> coefficient(count);
            // columns of the table: rigid unsplined, rigid splined, elastic restrained, elastic unrestrained, inertial
            // restrained, inertial unrestrained: `rigid` is the splined one
            std::array<std::vector<double>, 5> column;
            for (auto& entry : column)
                entry.resize(count);
            const std::size_t position[5] = {1, 2, 3, 4, 5};
            for (std::size_t item = 0; item < values->shape[0]; ++item)
                for (std::size_t row = 0; row < 6; ++row)
                {
                    const std::size_t at = item * 6 + row;
                    variable[at] = variables->at(item);
                    coefficient[at] = COEFFICIENTS[row];
                    for (std::size_t which = 0; which < 5; ++which)
                        column[which][at] = numbers[(item * 6 + row) * 6 + position[which]];
                }
            RowBlock block(count);
            block.ints("subcase", std::vector<std::int32_t>(count, ref.result->meta().subcase));
            block.texts("variable", std::move(variable));
            block.texts("coefficient", std::move(coefficient));
            static const char* const NAMES[5] = {"rigid", "elasticRestrained", "elasticUnrestrained",
                                                 "inertialRestrained", "inertialUnrestrained"};
            for (std::size_t which = 0; which < 5; ++which)
                block.reals(NAMES[which], std::move(column[which]));
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
        info.name = "stabilityDerivativesTable";
        info.family = "R9";
        info.resultQuery = true;
        info.description =
            "The stability derivatives: for each subcase and trim variable (the reference coefficients are "
            "`REFCOEFF`), one row "
            "for each of the six coefficients (`cx cy cz cmx cmy cmz`) with its value for the rigid aircraft (the "
            "splined "
            "aerodynamic model: the unsplined values of the table are not given), the elastic aircraft restrained and "
            "unrestrained, and with the inertial relief restrained and unrestrained.";
        info.defaultSort = "subcase";
        ColumnListBuilder columns;
        columns.int32("subcase").dictionary("variable").dictionary("coefficient");
        columns.results("rigid elasticRestrained elasticUnrestrained inertialRestrained inertialUnrestrained");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addStabilityDerivativesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<StabilityDerivativesTableQuery>());
}

} // namespace oor::query
