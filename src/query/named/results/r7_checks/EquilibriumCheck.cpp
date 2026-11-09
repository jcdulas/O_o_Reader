#include "oor/analysis/coordinates/ModelBounds.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r7_checks/CheckQueries.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"
#include "oor/query/results/NamedArrays.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

double norm3(const std::array<double, 6>& value, std::size_t first)
{
    return std::sqrt(value[first] * value[first] + value[first + 1] * value[first + 1] +
                     value[first + 2] * value[first + 2]);
}

// equilibriumCheck: the applied loads and the reactions of each static subcase add up to zero.
class EquilibriumCheckQuery final : public ResultQuery
{
public:
    EquilibriumCheckQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const double tolerance = context.arguments().realOr("tolerance", 1.0e-3);
        if (!(tolerance >= 0.0))
            throw QueryError("tolerance is a relative residual, not negative");
        ResultantRequest request;
        request.point = context.arguments().point("point").value_or(std::array<double, 3>{0.0, 0.0, 0.0});
        const std::int64_t system = context.resultantSystem();
        ResultantSet applied;
        ResultantSet reactions;
        const std::vector<ProviderAttempt> appliedAttempts = appliedLoadChain().resolve(context, request, applied);
        const std::vector<ProviderAttempt> reactionAttempts = reactionChain().resolve(context, request, reactions);
        // static subcases only: the checks of a time or a frequency would need a row for each step, and the tables of
        // the modes and of the buckling are eigenvectors, which no load balances
        std::set<std::pair<std::size_t, std::int32_t>> staticSubcases;
        for (std::size_t set = 0; set < context.sets().size(); ++set)
            for (const std::shared_ptr<results::ResultTable>& table : context.sets()[set].tables->all())
                if (table->meta().analysisCode == 1)
                    staticSubcases.insert({set, table->meta().subcase});
        const auto isStatic = [&](const ResultantRow& row)
        {
            return (row.stepKind == results::StepKind::None || row.stepKind == results::StepKind::LoadCombinationSet) &&
                   (staticSubcases.contains({row.set, row.subcase}) || row.note.rfind("load cards", 0) == 0);
        };
        using Key = std::tuple<std::int32_t, std::size_t, std::int32_t>;
        std::map<Key, std::pair<const ResultantRow*, const ResultantRow*>> pairs;
        for (const ResultantRow& row : applied.rows())
            if (isStatic(row))
                pairs[{row.designCycle, row.set, row.subcase}].first = &row;
        for (const ResultantRow& row : reactions.rows())
            if (isStatic(row))
                pairs[{row.designCycle, row.set, row.subcase}].second = &row;
        if (pairs.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(
                    describeAttempts("the applied loads of a static subcase", appliedAttempts) + "\n" +
                    describeAttempts("the reactions of a static subcase", reactionAttempts));
            return emptyTable(info().defaultColumns());
        }
        if (context.selection().onMissing == OnMissing::Raise)
        {
            for (const auto& [key, pair] : pairs)
            {
                if (pair.first == nullptr)
                    throw QuantityUnavailableError("the subcase " + std::to_string(std::get<2>(key)) +
                                                   " has reactions and no applied load: " +
                                                   describeAttempts("the applied loads", appliedAttempts));
                if (pair.second == nullptr)
                    throw QuantityUnavailableError(
                        "the subcase " + std::to_string(std::get<2>(key)) +
                        " has applied loads and no reaction: " + describeAttempts("the reactions", reactionAttempts));
            }
        }
        // the size of the model: the moments are compared with the forces times this length, and a force is compared
        // with the noise of the file (single precision of the largest load) when a subcase has no load worth the name
        double size = 1.0;
        if (const auto box = analysis::boundingBox(*context.modelPointer(), context.exec().pool()))
            size = std::max(base::norm(box->maximum - box->minimum), 1.0e-30);
        double largest = 0.0;
        for (const auto& [key, pair] : pairs)
            for (const ResultantRow* row : {pair.first, pair.second})
                if (row != nullptr && std::isfinite(row->value[0] + row->value[1] + row->value[2]))
                    largest = std::max(largest, norm3(row->value, 0));
        const double noise = 1.0e-6 * largest;
        // EPSILON and the external work of each subcase, when a set made in memory gives them (a solver gives them
        // as the named results `epsilon` and `externalWork`)
        const auto perSubcase = [&](const char* kind)
        {
            std::map<std::pair<std::size_t, std::int32_t>, double> values;
            for (const ResultQueryContext::NamedRef& named : context.named(kind))
            {
                const std::shared_ptr<const results::NamedResultData> data = named.result->data(context.decode());
                const results::ResultArray* subcases = data->find("subcase");
                const results::ResultArray* numbers = data->find(kind);
                if (subcases == nullptr || numbers == nullptr)
                    continue;
                const std::vector<std::int32_t> keys = arrayInts(*subcases);
                const std::vector<double> found = arrayDoubles(*numbers);
                for (std::size_t index = 0; index < keys.size() && index < found.size(); ++index)
                    values[{named.set, keys[index]}] = found[index];
            }
            return values;
        };
        const std::map<std::pair<std::size_t, std::int32_t>, double> epsilons = perSubcase("epsilon");
        const std::map<std::pair<std::size_t, std::int32_t>, double> works = perSubcase("externalWork");
        std::vector<double> epsilon, externalWork;
        const std::size_t count = pairs.size();
        std::vector<std::int32_t> subcase;
        std::array<std::vector<double>, 6> appliedValue, reactionValue, residual;
        std::vector<double> relative;
        std::vector<std::uint8_t> balanced, balancedValid;
        std::vector<std::string> appliedSource, reactionSource;
        std::vector<std::uint8_t> appliedSourceValid, reactionSourceValid;
        bool single = true;
        for (const auto& [key, pair] : pairs)
        {
            if (context.selection().onMissing == OnMissing::Skip && (pair.first == nullptr || pair.second == nullptr))
                continue;
            subcase.push_back(std::get<2>(key));
            const std::pair<std::size_t, std::int32_t> setSubcase{std::get<1>(key), std::get<2>(key)};
            const auto foundEpsilon = epsilons.find(setSubcase);
            epsilon.push_back(foundEpsilon != epsilons.end() ? foundEpsilon->second : NOT_A_NUMBER);
            const auto foundWork = works.find(setSubcase);
            externalWork.push_back(foundWork != works.end() ? foundWork->second : NOT_A_NUMBER);
            std::array<double, 6> a = {NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER,
                                       NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
            std::array<double, 6> r = a;
            if (pair.first != nullptr)
                a = resultantInSystem(context, pair.first->set, pair.first->value, system);
            if (pair.second != nullptr)
                r = resultantInSystem(context, pair.second->set, pair.second->value, system);
            std::array<double, 6> sum;
            for (std::size_t component = 0; component < 6; ++component)
            {
                appliedValue[component].push_back(a[component]);
                reactionValue[component].push_back(r[component]);
                sum[component] = a[component] + r[component];
                residual[component].push_back(sum[component]);
            }
            const bool both = pair.first != nullptr && pair.second != nullptr;
            const double appliedForce = norm3(a, 0);
            const double reactionForce = norm3(r, 0);
            const double reference = std::max(appliedForce, reactionForce);
            const double residualForce = norm3(sum, 0);
            const double residualMoment = norm3(sum, 3);
            const bool finite = std::isfinite(residualForce) && std::isfinite(residualMoment);
            const double forceRatio = both && finite && reference > 0.0 ? residualForce / reference : NOT_A_NUMBER;
            bool ok = false;
            if (both && finite)
            {
                const double momentReference = std::max({norm3(a, 3), norm3(r, 3), reference * size});
                ok = residualForce <= tolerance * std::max(reference, noise) &&
                     residualMoment <= tolerance * std::max(momentReference, noise * size);
            }
            relative.push_back(forceRatio);
            balanced.push_back(ok ? 1 : 0);
            balancedValid.push_back(both && finite ? 1 : 0);
            const auto describe = [&](const ResultantSet& set, const ResultantRow* row)
            {
                const ProviderInfo& provider = set.providers()[static_cast<std::size_t>(row->provider)];
                return sourceText(row->note.empty() ? provider.name : row->note, provider.approximate);
            };
            appliedSource.push_back(pair.first != nullptr ? describe(applied, pair.first) : std::string());
            appliedSourceValid.push_back(pair.first != nullptr ? 1 : 0);
            reactionSource.push_back(pair.second != nullptr ? describe(reactions, pair.second) : std::string());
            reactionSourceValid.push_back(pair.second != nullptr ? 1 : 0);
            for (const ResultantRow* row : {pair.first, pair.second})
                if (row != nullptr && row->precision == Precision::Double)
                    single = false;
        }
        (void)count;
        if (subcase.empty())
            return emptyTable(info().defaultColumns());
        RowBlock block(subcase.size());
        block.ints("subcase", std::move(subcase));
        static const char* const NAMES[6] = {"Fx", "Fy", "Fz", "Mx", "My", "Mz"}; // appliedFx, reactionFx, ...
        for (std::size_t component = 0; component < 6; ++component)
        {
            block.reals(std::string("applied") + NAMES[component], std::move(appliedValue[component]));
            block.reals(std::string("reaction") + NAMES[component], std::move(reactionValue[component]));
            block.reals(std::string("residual") + NAMES[component], std::move(residual[component]));
        }
        block.reals("relativeResidual", std::move(relative));
        block.flags("balanced", std::move(balanced), std::move(balancedValid));
        block.texts("appliedSource", std::move(appliedSource), std::move(appliedSourceValid));
        block.texts("reactionSource", std::move(reactionSource), std::move(reactionSourceValid));
        block.reals("epsilon", std::move(epsilon));
        block.reals("externalWork", std::move(externalWork));
        block.precision(single ? Precision::Single : Precision::Double);
        RowAccumulator accumulator;
        const auto noteWays = [&](const ResultantSet& ways, bool applied)
        {
            std::map<std::int16_t, std::int64_t> counts;
            for (const auto& entry : pairs)
            {
                const ResultantRow* row = applied ? entry.second.first : entry.second.second;
                if (row != nullptr)
                    ++counts[row->provider];
            }
            for (const auto& [provider, count] : counts)
            {
                const ProviderInfo& way = ways.providers()[static_cast<std::size_t>(provider)];
                std::vector<std::string> columns;
                for (const char* name : {"Fx", "Fy", "Fz", "Mx", "My", "Mz"})
                    columns.push_back(std::string(applied ? "applied" : "reaction") + name);
                accumulator.note(accumulator.addProvider(way.name, way.approximate), columns, count);
            }
        };
        noteWays(applied, true);
        noteWays(reactions, false);
        accumulator.add(std::move(block));
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "equilibriumCheck";
        info.family = "R7";
        info.resultQuery = true;
        info.needsModel = true;
        info.description =
            "For each static subcase, the resultant of the applied loads, the resultant of the reactions and their "
            "sum, the "
            "residual, about `point` (basic system) and in the basic system or along the axes of the coordinate system "
            "`frame`. "
            "The applied loads come from the applied-load table of the result file, else from the load cards of the "
            "model; the "
            "reactions from the SPC force table, else from the grid point force balance (`appliedSource` and "
            "`reactionSource` say which). `relativeResidual` is the norm of the residual force over the norm of the "
            "applied "
            "force; `balanced` is true when it is within `tolerance` and so is the residual moment relative to the "
            "larger of the "
            "two moments. Dynamic subcases are not checked (a row for each step would be needed). `epsilon` and "
            "`externalWork` are those a set made in memory gives (the named results `epsilon` and `externalWork`), "
            "null otherwise.";
        info.parameters = {
            parameter("tolerance", ParameterKind::Real, "0.001", "the relative residual that is accepted"),
            parameter("point", ParameterKind::Point, "(0, 0, 0)",
                      "the point the moments are taken about (basic system)")};
        info.defaultSort = "subcase";
        ColumnListBuilder columns;
        columns.int32("subcase").results("appliedFx appliedFy appliedFz appliedMx appliedMy appliedMz");
        columns.results("reactionFx reactionFy reactionFz reactionMx reactionMy reactionMz");
        columns.results("residualFx residualFy residualFz residualMx residualMy residualMz");
        columns.real("relativeResidual").flag("balanced").dictionary("appliedSource").dictionary("reactionSource");
        columns.real("epsilon").real("externalWork");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addEquilibriumCheckQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<EquilibriumCheckQuery>());
}

} // namespace oor::query
