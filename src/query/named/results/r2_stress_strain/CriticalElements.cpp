#include "StressData.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace oor::query
{

namespace
{

struct Candidate
{
    ElementExtreme extreme;
    double value = 0.0;
    std::size_t set = 0; // the result set where the value is
    std::int32_t subcase = 0;
    double step = 0.0;
    std::int32_t layer = 0;
    std::string location;
    std::int32_t pid = 0;
};

// criticalElements: the elements with the largest (or smallest) value of a stress, overall or in each group.
class CriticalElementsQuery final : public ResultQuery
{
public:
    CriticalElementsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        const std::string quantity = arguments.textOr("quantity", "vonMises");
        const bool strain = arguments.boolOr("strain", false);
        const std::int64_t top = arguments.integerOr("top", 10);
        const std::string mode = arguments.textOr("mode", "max");
        const std::string per = arguments.textOr("per", "");
        if (!isStressQuantity(quantity, strain))
            throw QueryError("quantity '" + quantity + "' is not a " + (strain ? "strain" : "stress") + " quantity");
        if (top < 0)
            throw QueryError("top is a number of elements, not negative");
        if (mode != "max" && mode != "min" && mode != "absmax")
            throw QueryError("mode is \"max\", \"min\" or \"absmax\", not '" + mode + "'");
        if (!per.empty() && per != "property" && per != "material" && per != "subcase" && per != "component")
            throw QueryError("per is None, \"property\", \"material\", \"subcase\" or \"component\", not '" + per +
                             "'");
        if (per == "property" || per == "material" || per == "component")
            context.requireModel("group the elements by " + per);
        const ElementRequest request = ElementRequest::from(arguments);
        if (request.propertyIds)
            context.requireModel("find the property of each element");
        const StressData data = resolveStress(context, request, strain);
        const std::vector<ElementExtreme> extremes = elementExtremes(data, quantity, per == "subcase");
        if (extremes.empty())
            return emptyTable(info().defaultColumns());
        ResultModelIndex index(context);
        std::map<std::int32_t, std::vector<Candidate>> groups;
        for (const ElementExtreme& extreme : extremes)
        {
            Candidate candidate;
            candidate.extreme = extreme;
            candidate.pid = context.elementCard(extreme.set, extreme.eid).pid;
            bool takeMax = mode != "min";
            if (mode == "absmax")
                takeMax = std::fabs(extreme.maxValue) >= std::fabs(extreme.minValue);
            candidate.value = takeMax ? extreme.maxValue : extreme.minValue;
            candidate.set = takeMax ? extreme.maxSet : extreme.minSet;
            candidate.subcase = takeMax ? extreme.maxSubcase : extreme.minSubcase;
            candidate.step = takeMax ? extreme.maxStep : extreme.minStep;
            candidate.layer = takeMax ? extreme.maxLayer : extreme.minLayer;
            candidate.location = takeMax ? extreme.maxLocation : extreme.minLocation;
            if (mode == "absmax")
                candidate.value = std::fabs(candidate.value);
            std::int32_t group = 0;
            if (per == "property")
                group = candidate.pid;
            else if (per == "material")
                group = index.material(extreme.set, extreme.eid);
            else if (per == "subcase")
                group = extreme.subcase;
            else if (per == "component")
                group = index.component(extreme.set, extreme.eid);
            groups[group].push_back(std::move(candidate));
        }
        std::vector<std::int32_t> groupColumn;
        std::vector<std::uint8_t> groupValid;
        std::vector<std::int32_t> rank, eid, pid, subcase, layer;
        std::vector<std::uint8_t> pidValid, layerValid;
        std::vector<std::string> type, location, analysis;
        std::vector<double> value, step;
        bool single = true;
        for (auto& [group, candidates] : groups)
        {
            const bool descending = mode != "min";
            std::stable_sort(candidates.begin(), candidates.end(),
                             [&](const Candidate& left, const Candidate& right)
                             {
                                 if (left.value != right.value)
                                     return descending ? left.value > right.value : left.value < right.value;
                                 return left.extreme.eid < right.extreme.eid;
                             });
            const std::size_t keep = std::min<std::size_t>(candidates.size(), static_cast<std::size_t>(top));
            for (std::size_t position = 0; position < keep; ++position)
            {
                const Candidate& candidate = candidates[position];
                groupColumn.push_back(group);
                groupValid.push_back(per.empty() ? 0 : 1);
                rank.push_back(static_cast<std::int32_t>(position + 1));
                eid.push_back(candidate.extreme.eid);
                type.push_back(candidate.extreme.card);
                pid.push_back(candidate.pid);
                pidValid.push_back(candidate.pid > 0 ? 1 : 0);
                value.push_back(candidate.value);
                analysis.push_back(context.sets()[candidate.set].analysis);
                subcase.push_back(candidate.subcase);
                step.push_back(candidate.step);
                layer.push_back(candidate.layer);
                layerValid.push_back(candidate.layer > 0 ? 1 : 0);
                location.push_back(candidate.location);
                single = single && candidate.extreme.precision == Precision::Single;
            }
        }
        if (eid.empty())
            return emptyTable(info().defaultColumns());
        RowBlock block(eid.size());
        block.ints("group", std::move(groupColumn), std::move(groupValid));
        block.ints("rank", std::move(rank));
        block.ints("elementId", std::move(eid));
        block.texts("type", std::move(type));
        block.ints("propId", std::move(pid), std::move(pidValid));
        block.reals("value", std::move(value));
        block.texts("analysis", std::move(analysis));
        block.ints("subcase", std::move(subcase));
        block.reals("step", std::move(step));
        block.ints("layer", std::move(layer), std::move(layerValid));
        block.texts("location", std::move(location));
        block.precision(single ? Precision::Single : Precision::Double);
        RowAccumulator accumulator;
        noteInputs(accumulator, data, {"value"});
        accumulator.add(std::move(block));
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "criticalElements";
        info.family = "R2";
        info.resultQuery = true;
        info.description =
            "The `top` elements with the largest value of a stress (a strain with `strain`): `mode=\"max\"` the "
            "largest, "
            "\"min\" the smallest, \"absmax\" the largest magnitude. Each element is judged by its extreme over every "
            "subcase, step, layer and location, which `subcase`, `step`, `layer` and `location` say. With `per` the "
            "top N "
            "of each group is given: \"property\", \"material\", \"component\" (needs the model) or \"subcase\" (an "
            "element "
            "is then judged in each subcase); `group` is the ID of the group, null without `per`. `analysis` is the "
            "label "
            "of the result set where the critical value is.";
        info.parameters = {
            parameter("quantity", ParameterKind::Text, "\"vonMises\"", "the stress or strain quantity"),
            parameter("top", ParameterKind::Integer, "10", "the number of elements to keep (in each group)"),
            parameter("mode", ParameterKind::Text, "\"max\"", "\"max\", \"min\" or \"absmax\""),
            parameter("per", ParameterKind::Text, "None",
                      "None, \"property\", \"material\", \"subcase\" or \"component\""),
            parameter("strain", ParameterKind::Bool, "False", "a strain quantity instead of a stress"),
            parameter("elementIds", ParameterKind::IdList, "None", "the elements to look at"),
            parameter("types", ParameterKind::TextList, "None", "the element types"),
            parameter("propertyIds", ParameterKind::IdList, "None",
                      "the properties of the elements (needs the model)")};
        info.defaultSort = "group, rank";
        ColumnListBuilder columns;
        columns.int32("group")
            .int32("rank")
            .int32("elementId")
            .dictionary("type")
            .int32("propId")
            .result("value")
            .dictionary("analysis");
        columns.int32("subcase");
        columns.real("step").int32("layer").dictionary("location");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addCriticalElementsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<CriticalElementsQuery>());
}

} // namespace oor::query
