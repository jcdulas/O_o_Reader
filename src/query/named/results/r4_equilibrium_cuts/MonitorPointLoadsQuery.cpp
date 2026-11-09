#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r4_equilibrium_cuts/EquilibriumQueries.hpp"
#include "oor/query/providers/quantities/MonitorPointLoads.hpp"

#include <algorithm>
#include <numeric>
#include <tuple>

namespace oor::query
{

namespace
{

// The order of the contributions in a table: the loads, then their totals.
int contributionRank(const std::string& contribution)
{
    static constexpr std::array<const char*, 7> ORDER = {"inertial",  "external", "flexible", "gust",
                                                         "totalAero", "total",    "resultant"};
    const auto found = std::find(ORDER.begin(), ORDER.end(), contribution);
    return static_cast<int>(found - ORDER.begin());
}

// monitorPointLoads: the integrated loads of the monitor points of the deck.
class MonitorPointLoadsQuery final : public ResultQuery
{
public:
    MonitorPointLoadsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        const std::int64_t system = context.resultantSystem();
        if (system != 0 || context.selection().hasFrame())
            throw QueryError("monitorPointLoads gives the loads along the CD axes of each point: it takes no frame");
        MonitorRequest request;
        request.names = arguments.texts("names");
        for (const std::string& card : arguments.texts("cards"))
        {
            // a card type ("MonitorIntegratedLoad1") or its keyword in the standard dictionary ("MONPNT1")
            request.cards.push_back(card);
        }
        MonitorLoadSet set;
        const std::vector<ProviderAttempt> attempts = monitorChain().resolve(context, request, set);
        if (set.rowCount() == 0)
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(describeAttempts("the monitor point loads", attempts));
            return emptyTable(info().defaultColumns());
        }
        const std::vector<MonitorLoadRow>& rows = set.rows();
        std::vector<std::size_t> order(rows.size());
        std::iota(order.begin(), order.end(), std::size_t{0});
        std::stable_sort(order.begin(), order.end(),
                         [&](std::size_t left, std::size_t right)
                         {
                             const MonitorLoadRow& a = rows[left];
                             const MonitorLoadRow& b = rows[right];
                             const auto key = [](const MonitorLoadRow& row)
                             {
                                 return std::make_tuple(row.designCycle, row.set, row.name, row.side,
                                                        contributionRank(row.contribution), row.subcase);
                             };
                             if (key(a) != key(b))
                                 return key(a) < key(b);
                             return a.step < b.step || (std::isnan(b.step) && !std::isnan(a.step));
                         });
        RowAccumulator accumulator;
        std::vector<std::int16_t> providers;
        for (const ProviderInfo& info : set.providers())
            providers.push_back(accumulator.addProvider(info.name, info.approximate));
        std::size_t begin = 0;
        while (begin < order.size())
        {
            const MonitorLoadRow& first = rows[order[begin]];
            std::size_t end = begin + 1;
            while (end < order.size())
            {
                const MonitorLoadRow& row = rows[order[end]];
                if (row.set != first.set || row.designCycle != first.designCycle || row.stepKind != first.stepKind ||
                    row.precision != first.precision || row.isComplex != first.isComplex || row.status != first.status)
                    break;
                ++end;
            }
            accumulator.add(blockOf(rows, order, begin, end, providers, set.providers()));
            begin = end;
        }
        return accumulator.finish(context, info());
    }

private:
    static RowBlock blockOf(const std::vector<MonitorLoadRow>& rows, const std::vector<std::size_t>& order,
                            std::size_t begin, std::size_t end, const std::vector<std::int16_t>& providers,
                            const std::vector<ProviderInfo>& infos)
    {
        static constexpr std::array<const char*, 6> NAMES = {"fx", "fy", "fz", "mx", "my", "mz"};
        const MonitorLoadRow& first = rows[order[begin]];
        const std::size_t count = end - begin;
        RowBlock block(count);
        std::vector<std::string> name;
        std::vector<std::string> card;
        std::vector<std::string> side;
        std::vector<std::string> contribution;
        std::vector<std::int32_t> subcase;
        std::array<std::vector<double>, 6> real;
        std::array<std::vector<double>, 6> imaginary;
        std::vector<std::int32_t> cd;
        std::array<std::vector<double>, 3> point;
        std::vector<double> steps;
        std::vector<std::int16_t> provider;
        std::vector<std::string> source;
        for (std::size_t position = begin; position < end; ++position)
        {
            const MonitorLoadRow& row = rows[order[position]];
            name.push_back(row.name);
            card.push_back(row.card);
            side.push_back(row.side);
            contribution.push_back(row.contribution);
            subcase.push_back(row.subcase);
            for (std::size_t component = 0; component < 6; ++component)
            {
                real[component].push_back(row.real[component]);
                imaginary[component].push_back(std::isnan(row.real[component]) ? std::nan("")
                                                                               : row.imaginary[component]);
            }
            cd.push_back(row.displacementFrameId);
            for (std::size_t axis = 0; axis < 3; ++axis)
                point[axis].push_back(row.point[axis]);
            steps.push_back(row.step);
            provider.push_back(providers[static_cast<std::size_t>(row.provider)]);
            const ProviderInfo& info = infos[static_cast<std::size_t>(row.provider)];
            source.push_back(sourceText(info.name, info.approximate));
        }
        block.texts("name", std::move(name));
        block.texts("card", std::move(card));
        block.texts("side", std::move(side));
        block.texts("contribution", std::move(contribution));
        block.ints("subcase", std::move(subcase));
        for (std::size_t component = 0; component < 6; ++component)
        {
            if (first.isComplex)
                block.complex(NAMES[component], std::move(real[component]), std::move(imaginary[component]));
            else
                block.reals(NAMES[component], std::move(real[component]));
        }
        block.ints("displacementFrameId", std::move(cd));
        block.reals("pointX", std::move(point[0]));
        block.reals("pointY", std::move(point[1]));
        block.reals("pointZ", std::move(point[2]));
        block.texts("source", std::move(source));
        block.providers(std::move(provider));
        if (first.stepKind == results::StepKind::Time || first.stepKind == results::StepKind::Frequency)
            block.steps(first.stepKind, std::move(steps));
        block.origin(first.set, first.analysis, first.designCycle);
        block.precision(first.precision);
        if (!first.status.empty())
            block.validation(first.status);
        return block;
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "monitorPointLoads";
        info.family = "R4";
        info.resultQuery = true;
        info.description =
            "The integrated loads of the monitor points, for each point, side, contribution, subcase and step, along "
            "the CD axes of the point and about its point. They are read from the monitor point table "
            "of the results (the named result `monitorPointLoads`): a MONPNT1 gives its "
            "structural contributions (inertial, external, flexible increment, gust) and its aerodynamic ones "
            "(flexible increment, gust), with their totals (totalAero = flexible + gust; total = "
            "inertial + external + flexible + gust on the structure); a MONPNT3 gives its `resultant`. A MONPNT3 that "
            "the results do not store is computed from the grid point forces (the forces of the "
            "elements of ELEMSET at the grid points of GRIDSET and the loads XFLAG does not exclude; it needs the "
            "model). `pointX pointY pointZ` is the point in the basic system (null when its CP is not 0 and no "
            "model gives the system). A component the point does not monitor is null.";
        info.parameters = {
            parameter("names", ParameterKind::TextList, "None", "the monitor points to give, by name (None: all)"),
            parameter(
                "cards", ParameterKind::TextList, "None",
                "the card types to give: \"MonitorIntegratedLoad1\", \"MonitorElemLoad2\", \"MonitorNodeForceSum3\", "
                "\"AeroControlSurface\" (a hinge moment), or their keywords (None: all)")};
        info.defaultSort = "name, side, contribution, subcase, [step]";
        ColumnListBuilder columns;
        columns.dictionary("name").dictionary("card").dictionary("side").dictionary("contribution");
        columns.int32("subcase").step().results("fx fy fz mx my mz").int32("displacementFrameId");
        columns.reals("pointX pointY pointZ").dictionary("source");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMonitorPointLoadsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MonitorPointLoadsQuery>());
}

} // namespace oor::query
