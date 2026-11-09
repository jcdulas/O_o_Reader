#include "PlyData.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"

#include <cmath>
#include <map>
#include <tuple>

namespace oor::query
{

namespace
{

struct Best
{
    std::int16_t provider = -1;
    double value = 0.0;
    std::int32_t layer = 0;
    std::int32_t plyId = 0;
    bool hasPlyId = false;
    std::int32_t subcase = 0;
    double step = 0.0;
    std::string theory;
    std::int32_t pid = 0;
    Precision precision = Precision::Double;
    std::size_t set = 0;
};

// criticalPly: for each laminate element, the ply that is most critical by a failure index, a strength ratio or a
// stress.
class CriticalPlyQuery final : public ResultQuery
{
public:
    CriticalPlyQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const std::string criterion = context.arguments().textOr("criterion", "failureIndex");
        const bool byRatio = criterion == "strengthRatio";
        const bool byIndex = criterion == "failureIndex";
        static const char* const STRESSES[] = {"o11", "o22", "t12", "t1z", "t2z", "omax", "omin", "maxShear"};
        bool known = byRatio || byIndex;
        for (const char* name : STRESSES)
            known = known || criterion == name;
        if (!known)
            throw QueryError(
                "criterion is \"failureIndex\", \"strengthRatio\" or a ply stress (o11 o22 t12 t1z t2z omax omin "
                "maxShear), not '" +
                criterion + "'");
        const ElementRequest request = ElementRequest::from(context.arguments());
        if (request.propertyIds)
            context.requireModel("find the property of each element");
        const PlyData data = buildPlyData(context, request);
        // PERFORMANCE (COMPOSITE-API-AUDIT P2): the columns of a flat looked up once, the entry of an element kept
        // across the rows of its plies, its property looked up when a row of it wins
        std::map<std::int32_t, Best> best;
        for (const ElementFlat& flat : data.flats)
        {
            const std::vector<double>& values = flat.values.at(criterion);
            const std::vector<double>& providers = flat.values.at("failureProvider");
            const std::vector<double>& plyIds = flat.values.at("plyId");
            const std::vector<std::string>& theories = flat.texts.at("failureTheory");
            std::map<std::int32_t, Best>::iterator position = best.end();
            for (std::size_t row = 0; row < flat.rows(); ++row)
            {
                const double value = values[row];
                if (std::isnan(value))
                    continue;
                bool inserted = false;
                if (position == best.end() || position->first != flat.eid[row])
                    std::tie(position, inserted) = best.try_emplace(flat.eid[row]);
                Best& current = position->second;
                const bool better = inserted || (byRatio ? value < current.value : value > current.value);
                if (!better)
                {
                    if (flat.precision == Precision::Double)
                        current.precision = Precision::Double;
                    continue;
                }
                current.value = value;
                current.provider = byRatio || byIndex ? static_cast<std::int16_t>(providers[row]) : std::int16_t(-1);
                current.layer = flat.layer[row];
                const double plyId = plyIds[row];
                current.hasPlyId = !std::isnan(plyId);
                current.plyId = current.hasPlyId ? static_cast<std::int32_t>(plyId) : 0;
                current.subcase = flat.subcase;
                current.step = flat.step.empty() ? std::nan("") : flat.step[row];
                current.theory = theories[row];
                current.pid = context.elementCard(flat.set, flat.eid[row]).pid;
                current.set = flat.set;
                current.precision = flat.precision;
            }
        }
        if (best.empty())
        {
            if (data.flats.empty() && context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(describeAttempts("the failure of the plies", data.attempts));
            return emptyTable(info().defaultColumns());
        }
        std::vector<std::int32_t> eid, pid, ply, plyId, subcase;
        std::vector<std::uint8_t> pidValid, plyIdValid;
        std::vector<double> value, step;
        std::vector<std::string> theory;
        bool single = true;
        for (const auto& [element, current] : best)
        {
            eid.push_back(element);
            pid.push_back(current.pid);
            pidValid.push_back(current.pid > 0 ? 1 : 0);
            ply.push_back(current.layer);
            plyId.push_back(current.plyId);
            plyIdValid.push_back(current.hasPlyId ? 1 : 0);
            value.push_back(current.value);
            subcase.push_back(current.subcase);
            step.push_back(current.step);
            theory.push_back(current.theory);
            single = single && current.precision == Precision::Single;
        }
        RowBlock block(eid.size());
        block.ints("elementId", std::move(eid));
        block.ints("propId", std::move(pid), std::move(pidValid));
        block.ints("ply", std::move(ply));
        block.ints("plyId", std::move(plyId), std::move(plyIdValid));
        block.reals("value", std::move(value));
        block.ints("subcase", std::move(subcase));
        block.reals("step", std::move(step));
        block.texts("failureTheory", std::move(theory));
        block.precision(single ? Precision::Single : Precision::Double);
        RowAccumulator accumulator;
        std::map<std::int16_t, std::int64_t> counts;
        for (const auto& entry : best)
            ++counts[entry.second.provider];
        for (const auto& [provider, count] : counts)
        {
            if (provider >= 0)
                accumulator.note(
                    accumulator.addProvider(data.providers[static_cast<std::size_t>(provider)].name,
                                            data.providers[static_cast<std::size_t>(provider)].approximate),
                    {"value"}, count);
            else
                accumulator.note(accumulator.addProvider("ply stress table of the result file", false), {"value"},
                                 count);
        }
        accumulator.add(std::move(block));
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "criticalPly";
        info.family = "R2";
        info.resultQuery = true;
        info.description =
            "For each laminate element, the most critical ply over every subcase and step: the largest failure index "
            "(`criterion=\"failureIndex\"`), the smallest strength ratio (\"strengthRatio\") or the largest value of "
            "a ply "
            "stress (`o11`, `omax`, ...). `value` is that number; `subcase` and `step` say where it occurs. The "
            "failure index "
            "and the strength ratio are those of `compositePlyTable`.";
        info.parameters = {parameter("criterion", ParameterKind::Text, "\"failureIndex\"",
                                     "\"failureIndex\", \"strengthRatio\" or a ply stress column"),
                           parameter("elementIds", ParameterKind::IdList, "None", "the elements to look at"),
                           parameter("propertyIds", ParameterKind::IdList, "None", "the properties (needs the model)")};
        info.defaultSort = "elementId";
        ColumnListBuilder columns;
        columns.int32("elementId")
            .int32("propId")
            .int32("ply")
            .int32("plyId")
            .result("value")
            .int32("subcase")
            .real("step");
        columns.dictionary("failureTheory");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addCriticalPlyQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<CriticalPlyQuery>());
}

} // namespace oor::query
