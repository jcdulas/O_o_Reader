#include "SeriesSource.hpp"

#include "oor/query/QueryErrors.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace oor::query
{

bool isSeriesQuantity(const std::string& quantity) noexcept
{
    return quantity == "displacement" || quantity == "velocity" || quantity == "acceleration" ||
           quantity == "spcForce" || quantity == "appliedLoad" || quantity == "temperature" || quantity == "stress" ||
           quantity == "strain" || quantity == "force";
}

std::vector<Series> collectSeries(const ResultQueryContext& context, const std::string& quantity,
                                  const std::string& component, const std::optional<std::vector<std::int64_t>>& ids,
                                  results::StepKind kind)
{
    if (!isSeriesQuantity(quantity))
        throw QueryError(
            "quantity is \"displacement\", \"velocity\", \"acceleration\", \"spcForce\", \"appliedLoad\", "
            "\"temperature\", \"stress\", \"strain\" or \"force\", not '" +
            quantity + "'");
    std::set<std::int64_t> wanted;
    if (ids)
        wanted.insert(ids->begin(), ids->end());
    std::vector<TableRef> tables;
    const bool element = quantity == "stress" || quantity == "strain" || quantity == "force";
    if (!element)
    {
        const results::ResultKind nodal = quantity == "displacement"   ? results::ResultKind::Displacements
                                          : quantity == "velocity"     ? results::ResultKind::Velocities
                                          : quantity == "acceleration" ? results::ResultKind::Accelerations
                                          : quantity == "spcForce"     ? results::ResultKind::SinglePointConstrForces
                                          : quantity == "appliedLoad"  ? results::ResultKind::AppliedLoads
                                                                       : results::ResultKind::Temperatures;
        tables = context.tables(nodal, quantity != "temperature");
    }
    else
    {
        const results::ResultKind kinds = quantity == "stress"   ? results::ResultKind::Stress
                                          : quantity == "strain" ? results::ResultKind::Strain
                                                                 : results::ResultKind::ElementForces;
        for (const std::string& name : context.elementNames(kinds))
        {
            std::vector<TableRef> more = context.elementTables(kinds, name, false);
            tables.insert(tables.end(), more.begin(), more.end());
        }
    }
    sortTables(tables);
    std::vector<Series> found;
    for (const TableRef& ref : tables)
    {
        const results::ResultTableMeta& meta = ref.table->meta();
        if (meta.stepKind != kind)
            continue;
        const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
        const TableValues values(*decoded);
        const int column = values.component(component);
        if (column < 0)
            continue;
        // the row of each place: a grid point is one row; an element is its centre at the first layer
        std::vector<std::size_t> chosen;
        if (!element)
        {
            for (std::size_t row = 0; row < values.rows(); ++row)
                if (!ids || wanted.contains(decoded->ids[row]))
                    chosen.push_back(row);
        }
        else
        {
            std::map<std::int32_t, std::size_t> best;
            for (std::size_t row = 0; row < values.rows(); ++row)
            {
                if (ids && !wanted.contains(decoded->ids[row]))
                    continue;
                const std::int32_t node = row < decoded->nodeIds.size() ? decoded->nodeIds[row] : 0;
                if (node != 0)
                    continue;
                const auto found = best.find(decoded->ids[row]);
                if (found == best.end() ||
                    (row < decoded->layers.size() && decoded->layers[row] < decoded->layers[found->second]))
                    best[decoded->ids[row]] = row;
            }
            for (const auto& [eid, row] : best)
                chosen.push_back(row);
            std::sort(chosen.begin(), chosen.end(),
                      [&](std::size_t a, std::size_t b) { return decoded->ids[a] < decoded->ids[b]; });
        }
        for (const std::size_t row : chosen)
        {
            Series series;
            series.set = ref.set;
            series.analysis = context.sets()[ref.set].analysis;
            series.subcase = meta.subcase;
            series.designCycle = meta.designCycle;
            series.id = decoded->ids[row];
            series.stepKind = kind;
            series.precision =
                decoded->type == results::ValueType::Float32 || decoded->type == results::ValueType::Complex64
                    ? Precision::Single
                    : Precision::Double;
            series.magnitudePhase = meta.storedFormat == results::ResultFormat::MagnitudePhase;
            for (std::size_t step = 0; step < values.steps(); ++step)
            {
                series.steps.push_back(decoded->steps[step]);
                series.real.push_back(values.real(step, row, static_cast<std::size_t>(column)));
                if (values.isComplex())
                    series.imaginary.push_back(values.imag(step, row, static_cast<std::size_t>(column)));
            }
            found.push_back(std::move(series));
        }
    }
    return found;
}

} // namespace oor::query
