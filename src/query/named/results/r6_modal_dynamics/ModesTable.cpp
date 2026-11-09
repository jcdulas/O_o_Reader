#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"
#include "oor/query/providers/quantities/ModalQuantities.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// modesTable: the modes of a modal analysis: eigenvalue, circular frequency, frequency, generalized mass and
// stiffness, effective mass fractions and their running sums.
class ModesTableQuery final : public ResultQuery
{
public:
    ModesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        ModalRequest request;
        request.modes = modeRowsOf(context, ModeAnalysis::RealModes);
        if (request.modes.empty())
            return emptyTable(info().defaultColumns());
        std::stable_sort(request.modes.begin(), request.modes.end(),
                         [](const ModeRow& a, const ModeRow& b)
                         {
                             return std::tie(a.designCycle, a.set, a.subcase, a.mode) <
                                    std::tie(b.designCycle, b.set, b.subcase, b.mode);
                         });
        ModalFractionSet fractions;
        // never raises: the effective mass is null when no way gives it
        (void)effectiveMassChain().resolve(context, request, fractions);
        std::map<std::tuple<std::size_t, std::int32_t, std::int32_t, std::int32_t>, const ModalFractions*> byMode;
        for (const ModalFractions& row : fractions.rows())
            byMode[{row.set, row.subcase, row.designCycle, row.mode}] = &row;
        const std::size_t count = request.modes.size();
        std::vector<std::int32_t> subcase, mode;
        std::vector<double> eigenvalue, radians, frequency, generalizedMass, generalizedStiffness;
        std::array<std::vector<double>, 6> fraction, cumulative;
        bool single = true;
        std::map<std::tuple<std::size_t, std::int32_t, std::int32_t>, std::array<double, 6>> running;
        std::int64_t derivedFrequencies = 0;
        std::map<std::int16_t, std::int64_t> byProvider;
        for (const ModeRow& row : request.modes)
        {
            subcase.push_back(row.subcase);
            mode.push_back(row.mode);
            eigenvalue.push_back(row.eigenvalue);
            radians.push_back(row.radians);
            frequency.push_back(row.cycles);
            generalizedMass.push_back(row.generalizedMass);
            generalizedStiffness.push_back(row.generalizedStiffness);
            single = single && row.precision == Precision::Single;
            derivedFrequencies += row.frequencyWay == "stored cycles" ? 0 : 1;
            std::array<double, 6>& total = running[{row.set, row.subcase, row.designCycle}];
            const auto found = byMode.find({row.set, row.subcase, row.designCycle, row.mode});
            for (std::size_t direction = 0; direction < 6; ++direction)
            {
                const double value = found != byMode.end() ? found->second->fraction[direction] : NOT_A_NUMBER;
                fraction[direction].push_back(value);
                if (!std::isnan(value))
                    total[direction] += value;
                cumulative[direction].push_back(found != byMode.end() && !std::isnan(value) ? total[direction]
                                                                                            : NOT_A_NUMBER);
            }
            if (found != byMode.end())
                ++byProvider[found->second->provider];
        }
        RowBlock block(count);
        block.ints("subcase", std::move(subcase));
        block.ints("mode", std::move(mode));
        block.reals("eigenvalue", std::move(eigenvalue));
        block.reals("radians", std::move(radians));
        block.reals("freq", std::move(frequency));
        block.reals("generalizedMass", std::move(generalizedMass));
        block.reals("generalizedStiffness", std::move(generalizedStiffness));
        // the suffixes of the columns of the six directions: meffT1, cumT1, ...
        static const char* const DIRECTIONS[6] = {"T1", "T2", "T3", "R1", "R2", "R3"};
        for (std::size_t direction = 0; direction < 6; ++direction)
        {
            block.reals(std::string("meff") + DIRECTIONS[direction], std::move(fraction[direction]));
            block.reals(std::string("cum") + DIRECTIONS[direction], std::move(cumulative[direction]));
        }
        block.precision(single ? Precision::Single : Precision::Double);
        // the way of each row: the frequency (stored cycles or derived) and the effective mass
        RowAccumulator accumulator;
        const std::int16_t stored = accumulator.addProvider("stored table", false);
        const std::int16_t derived = accumulator.addProvider("frequency derived from the eigenvalue", false);
        block.provider(stored);
        accumulator.note(derived, {"freq"}, derivedFrequencies);
        for (const auto& [provider, rows] : byProvider)
        {
            const ProviderInfo& way = fractions.providers()[static_cast<std::size_t>(provider)];
            const std::int16_t index = accumulator.addProvider(way.name, way.approximate);
            std::vector<std::string> names;
            for (std::size_t direction = 0; direction < 6; ++direction)
                names.push_back(std::string("meff") + DIRECTIONS[direction]);
            accumulator.note(index, names, rows);
        }
        accumulator.add(std::move(block));
        return sortedResult(accumulator.finish(context, info()), context.exec(),
                            {"designCycle", "analysis", "subcase", "mode"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "modesTable";
        info.family = "R6";
        info.resultQuery = true;
        info.description =
            "The modes of a normal modes analysis, one row for each subcase and mode: the eigenvalue, the circular "
            "frequency "
            "`radians`, the frequency `freq` (Hz: the stored cycles, else derived from the eigenvalue), the "
            "generalized mass and "
            "stiffness, the effective mass fractions in the six directions `meffT1 ... meffR3` and their running "
            "sums "
            "`cumT1 ... cumR3`. The effective mass comes from the named result `modalEffectiveMass` (which also "
            "gives the modes when the set has no eigenvalue table), else is "
            "computed from the eigenvectors and the mass of the model when it is made of concentrated masses; it is "
            "null when no "
            "way gives it (this query never raises for it). The eigenvalue tables carry no subcase: it is the one "
            "subcase of "
            "the eigenvectors when there is only one.";
        info.parameters = {};
        info.defaultSort = "subcase, mode";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .int32("mode")
            .results("eigenvalue radians")
            .real("freq")
            .results("generalizedMass generalizedStiffness");
        columns.reals("meffT1 meffT2 meffT3 meffR1 meffR2 meffR3 cumT1 cumT2 cumT3 cumR1 cumR2 cumR3");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addModesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ModesTableQuery>());
}

} // namespace oor::query
