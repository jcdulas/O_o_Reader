#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"

#include <cmath>
#include <map>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

// strainEnergyByProperty: the strain energy of the elements added up by property.
class StrainEnergyByPropertyQuery final : public ResultQuery
{
public:
    StrainEnergyByPropertyQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        struct Total
        {
            double energy = 0.0;
            double percent = 0.0;
            bool anyPercent = false;
            std::set<std::int32_t> elements;
            Precision precision = Precision::Single;
            std::string analysis;
            results::StepKind stepKind = results::StepKind::None;
            double step = std::nan("");
            double eigenvalue = std::nan("");
        };
        // (design cycle, set, subcase, step key, pid)
        std::map<std::tuple<std::int32_t, std::size_t, std::int32_t, double, std::int32_t>, Total> totals;
        double grandEnergy = 0.0;
        for (const std::string& name : context.elementNames(results::ResultKind::StrainEnergy))
            for (const TableRef& ref : context.elementTables(results::ResultKind::StrainEnergy, name, false))
            {
                const results::ResultTableMeta& meta = ref.table->meta();
                const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
                const TableValues values(*decoded);
                const int energy = values.component("strainEnergy");
                const int percent = values.component("percent");
                if (energy < 0 || values.isComplex())
                    continue;
                const bool stepped =
                    meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                    meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
                for (std::size_t step = 0; step < values.steps(); ++step)
                    for (std::size_t row = 0; row < values.rows(); ++row)
                    {
                        const std::int32_t pid = context.elementCard(ref.set, decoded->ids[row]).pid;
                        if (pid <= 0)
                            continue;
                        const double stepValue = stepped ? decoded->steps[step] : std::nan("");
                        Total& total = totals[{meta.designCycle, ref.set, meta.subcase,
                                               std::isnan(stepValue) ? -1.0e300 : stepValue, pid}];
                        total.analysis = context.sets()[ref.set].analysis;
                        total.stepKind = stepped ? meta.stepKind : results::StepKind::None;
                        total.step = stepValue;
                        total.eigenvalue =
                            step < decoded->eigenvalues.size() ? decoded->eigenvalues[step] : std::nan("");
                        total.energy += values.real(step, row, static_cast<std::size_t>(energy));
                        if (percent >= 0)
                        {
                            total.percent += values.real(step, row, static_cast<std::size_t>(percent));
                            total.anyPercent = true;
                        }
                        total.elements.insert(decoded->ids[row]);
                        if (decoded->type == results::ValueType::Float64)
                            total.precision = Precision::Double;
                        grandEnergy += values.real(step, row, static_cast<std::size_t>(energy));
                    }
            }
        (void)grandEnergy;
        if (totals.empty())
            return emptyTable(info().defaultColumns());
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        // one block for each (design cycle, set, subcase, step kind), the rows ordered by step then property
        std::map<std::tuple<std::int32_t, std::size_t, std::int32_t>,
                 std::vector<const std::pair<
                     const std::tuple<std::int32_t, std::size_t, std::int32_t, double, std::int32_t>, Total>*>>
            groups;
        for (const auto& entry : totals)
            groups[{std::get<0>(entry.first), std::get<1>(entry.first), std::get<2>(entry.first)}].push_back(&entry);
        for (const auto& [key, entries] : groups)
        {
            RowBlock block(entries.size());
            std::vector<std::int32_t> subcase(entries.size(), std::get<2>(key));
            std::vector<std::int32_t> pid, count;
            std::vector<double> energy, percent, steps, eigenvalues;
            bool single = true;
            for (const auto* entry : entries)
            {
                const Total& total = entry->second;
                pid.push_back(std::get<4>(entry->first));
                count.push_back(static_cast<std::int32_t>(total.elements.size()));
                energy.push_back(total.energy);
                percent.push_back(total.anyPercent ? total.percent : std::nan(""));
                steps.push_back(total.step);
                eigenvalues.push_back(total.eigenvalue);
                single = single && total.precision == Precision::Single;
            }
            block.ints("subcase", std::move(subcase));
            block.ints("propId", std::move(pid));
            block.ints("elementCount", std::move(count));
            block.reals("energy", std::move(energy));
            block.reals("percent", std::move(percent));
            const Total& first = entries.front()->second;
            if (first.stepKind != results::StepKind::None)
                block.steps(first.stepKind, std::move(steps), std::move(eigenvalues));
            block.origin(std::get<1>(key), first.analysis, std::get<0>(key));
            block.precision(single ? Precision::Single : Precision::Double);
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        std::vector<std::string> keys = leadingKeys();
        keys.push_back("propId");
        return sortedResult(accumulator.finish(context, info()), context.exec(), keys);
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "strainEnergyByProperty";
        info.family = "R6";
        info.resultQuery = true;
        info.needsModel = true;
        info.description =
            "The strain energy of the elements added up for each property of the model, for each subcase and step: the "
            "number "
            "of elements, the energy and the percent of the total energy that the table gives (their sum). Needs the "
            "model "
            "for the property of each element.";
        info.defaultSort = "subcase, [step], propId";
        ColumnListBuilder columns;
        columns.int32("subcase").step().int32("propId").int64("elementCount").results("energy percent");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addStrainEnergyByPropertyQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<StrainEnergyByPropertyQuery>());
}

} // namespace oor::query
