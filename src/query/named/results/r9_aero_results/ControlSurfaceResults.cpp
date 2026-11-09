#include "../NamedTables.hpp"
#include "AeroModelLinks.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NO_LIMIT = 1.0e9; // the limits of a hinge moment that the file leaves open are written as 1e10

// What the provenance says of the `id` of the rows: the table holds the label of each surface (the AESURF label, the
// one the trim variable table lists), not its ID, so the ID is the one of the AESURF card of the model that has the
// label.
const LookupWords WORDS{"id", "AESURF of the model that has the label",
                        "no AESURF of the model has the label: id is null", "the result set has no model: id is null",
                        "two AESURF cards of the model have the label: id is null"};

// controlSurfaceResults: the trimmed position and the hinge moment of each control surface, against their limits.
class ControlSurfaceResultsQuery final : public ResultQuery
{
public:
    ControlSurfaceResultsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        std::vector<ResultQueryContext::NamedRef> results = context.named("controlSurfaceResults");
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
            const results::ResultArray* position = data->find("position");
            const results::ResultArray* lower = data->find("positionLower");
            const results::ResultArray* upper = data->find("positionUpper");
            const results::ResultArray* moment = data->find("hingeMoment");
            const results::ResultArray* momentLower = data->find("hingeMomentLower");
            const results::ResultArray* momentUpper = data->find("hingeMomentUpper");
            const results::ResultText* label = data->findText("surface");
            if (position == nullptr || lower == nullptr || upper == nullptr || moment == nullptr || label == nullptr)
                continue;
            const std::vector<double> p = arrayDoubles(*position);
            const std::vector<double> pl = arrayDoubles(*lower);
            const std::vector<double> pu = arrayDoubles(*upper);
            const std::vector<double> h = arrayDoubles(*moment);
            const std::vector<double> hl =
                momentLower != nullptr ? arrayDoubles(*momentLower) : std::vector<double>(p.size(), -1.0e10);
            const std::vector<double> hu =
                momentUpper != nullptr ? arrayDoubles(*momentUpper) : std::vector<double>(p.size(), 1.0e10);
            const std::size_t count = p.size();
            const std::vector<std::string> labels = label->values();
            // the surfaces in the order of their IDs (the file order for the ones the model does not name)
            IdLookup surfaces = links.surfacesOfLabels(ref.set, labels);
            noteLookup(accumulator, links, ref.set, surfaces, count, WORDS);
            std::vector<std::size_t> order(count);
            std::iota(order.begin(), order.end(), std::size_t{0});
            std::stable_sort(order.begin(), order.end(),
                             [&](std::size_t a, std::size_t b)
                             {
                                 constexpr std::int64_t LAST = std::numeric_limits<std::int64_t>::max();
                                 const std::int64_t left = surfaces.valid[a] != 0 ? surfaces.id[a] : LAST;
                                 const std::int64_t right = surfaces.valid[b] != 0 ? surfaces.id[b] : LAST;
                                 return left < right;
                             });
            std::vector<std::int32_t> id(count);
            std::vector<std::uint8_t> idValid(count), within(count), withinValid(count);
            std::vector<std::string> sortedLabels(count);
            std::vector<double> sp(count), sl(count), su(count), sh(count);
            for (std::size_t row = 0; row < count; ++row)
            {
                const std::size_t from = order[row];
                id[row] = surfaces.id[from];
                idValid[row] = surfaces.valid[from];
                sortedLabels[row] = labels[from];
                sp[row] = p[from];
                sl[row] = pl[from];
                su[row] = pu[from];
                sh[row] = h[from];
                const bool positionLimits = pu[from] > pl[from];
                const bool momentLimits = std::fabs(hl[from]) < NO_LIMIT || std::fabs(hu[from]) < NO_LIMIT;
                bool fine = true;
                if (positionLimits)
                    fine = fine && p[from] >= pl[from] && p[from] <= pu[from];
                if (momentLimits)
                    fine = fine && h[from] >= hl[from] && h[from] <= hu[from];
                within[row] = fine ? 1 : 0;
                withinValid[row] = positionLimits || momentLimits ? 1 : 0;
            }
            RowBlock block(count);
            block.ints("subcase", std::vector<std::int32_t>(count, ref.result->meta().subcase));
            block.ints("id", std::move(id), std::move(idValid));
            block.texts("label", std::move(sortedLabels));
            block.reals("position", std::move(sp));
            block.reals("hingeMoment", std::move(sh));
            block.reals("positionLower", std::move(sl));
            block.reals("positionUpper", std::move(su));
            block.flags("isWithinLimits", std::move(within), std::move(withinValid));
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
        info.name = "controlSurfaceResults";
        info.family = "R9";
        info.resultQuery = true;
        info.description =
            "The control surfaces of a static aeroelastic analysis, for each subcase: the trimmed position, the hinge "
            "moment, "
            "the limits of the position, and whether the position and the hinge moment are within their limits (null "
            "when the "
            "table gives no limit). The table holds the label of each surface, not its ID: `id` is the ID of the "
            "AESURF "
            "card of the model that has the label, null when the result set has no model, when no AESURF has the label "
            "and when two have it (the provenance of the table says which). The rows are in the order of the IDs, the "
            "surfaces with no ID last in the order of the table.";
        info.defaultSort = "subcase, id";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("id").dictionary("label").results(
            "position hingeMoment positionLower positionUpper");
        columns.flag("isWithinLimits");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addControlSurfaceResultsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ControlSurfaceResultsQuery>());
}

} // namespace oor::query
