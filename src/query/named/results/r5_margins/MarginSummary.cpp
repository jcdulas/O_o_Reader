#include "MarginData.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/results/r5_margins/MarginQueries.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <cmath>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

// marginSummary: the margins summarised by property, material or component.
class MarginSummaryQuery final : public ResultQuery
{
public:
    MarginSummaryQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const std::string by = context.arguments().textOr("by", "property");
        if (by != "property" && by != "material" && by != "component")
            throw QueryError("by is \"property\", \"material\" or \"component\", not '" + by + "'");
        if (by == "component")
            context.requireModel("group the elements by component");
        const MarginSet set = computeMargins(context);
        if (set.rows.empty())
            return emptyTable(info().defaultColumns());
        ResultModelIndex index(context);
        // the most critical row of each element
        std::map<std::pair<std::size_t, std::int32_t>, const MarginRow*> worst;
        for (const MarginRow& row : set.rows)
        {
            if (std::isnan(row.ms))
                continue;
            auto [position, inserted] = worst.try_emplace({row.set, row.eid}, &row);
            if (!inserted && row.ms < position->second->ms)
                position->second = &row;
        }
        struct Group
        {
            std::int64_t elements = 0;
            std::int64_t negative = 0;
            const MarginRow* critical = nullptr;
        };
        std::map<std::int32_t, Group> groups;
        for (const auto& [key, row] : worst)
        {
            std::int32_t id = 0;
            if (by == "property")
                id = row->pid;
            else if (by == "material")
                id = row->mid;
            else
                id = index.component(row->set, row->eid);
            if (id <= 0)
                continue;
            Group& group = groups[id];
            ++group.elements;
            group.negative += row->ms < 0.0 ? 1 : 0;
            if (group.critical == nullptr || row->ms < group.critical->ms)
                group.critical = row;
        }
        if (groups.empty())
            return emptyTable(info().defaultColumns());
        std::vector<std::int32_t> key, subcase, eid, layer;
        std::vector<std::uint8_t> layerValid;
        std::vector<std::int32_t> elements, negative;
        std::vector<double> minMs, minRf, step;
        std::vector<std::string> criterion;
        bool single = true;
        for (const auto& [id, group] : groups)
        {
            key.push_back(id);
            elements.push_back(static_cast<std::int32_t>(group.elements));
            negative.push_back(static_cast<std::int32_t>(group.negative));
            criterion.push_back(group.critical->criterion);
            minMs.push_back(group.critical->ms);
            minRf.push_back(group.critical->rf);
            eid.push_back(group.critical->eid);
            subcase.push_back(group.critical->subcase);
            step.push_back(group.critical->step);
            layer.push_back(group.critical->layer);
            layerValid.push_back(group.critical->layer > 0 ? 1 : 0);
            single = single && group.critical->precision == Precision::Single;
        }
        (void)single;
        RowBlock block(key.size());
        block.ints("group", std::move(key));
        block.texts("criterion", std::move(criterion));
        block.ints("elementCount", std::move(elements));
        block.ints("negativeCount", std::move(negative));
        block.reals("minMs", std::move(minMs));
        block.reals("minRf", std::move(minRf));
        block.ints("criticalElementId", std::move(eid));
        block.ints("criticalSubcase", std::move(subcase));
        block.reals("criticalStep", std::move(step));
        block.ints("criticalLayer", std::move(layer), std::move(layerValid));
        RowAccumulator accumulator;
        std::map<std::int16_t, std::int64_t> inputs;
        for (const auto& entry : worst)
            if (entry.second->provider >= 0)
                ++inputs[entry.second->provider];
        for (const auto& [provider, count] : inputs)
        {
            const ProviderInfo& way = set.providers[static_cast<std::size_t>(provider)];
            accumulator.note(accumulator.addProvider(way.name, way.approximate), {"minMs", "minRf"}, count);
        }
        accumulator.add(std::move(block));
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "marginSummary";
        info.family = "R5";
        info.resultQuery = true;
        info.description =
            "The margins summarised by property, material or component (`by`): for each group the number of "
            "elements, the "
            "number whose margin is negative, the smallest margin of safety and reserve factor and where they occur "
            "(the "
            "element, subcase, step and layer). Each element counts by its worst margin over the subcases and steps. "
            "The "
            "parameters are those of `margins`; `group` is the ID of the property, material or component.";
        std::vector<ParameterInfo> shared = marginParameters();
        info.parameters.push_back(shared.front());
        info.parameters.push_back(
            parameter("by", ParameterKind::Text, "\"property\"", "\"property\", \"material\" or \"component\""));
        for (std::size_t index = 1; index < shared.size(); ++index)
            info.parameters.push_back(shared[index]);
        info.parameters.push_back(parameter("elementIds", ParameterKind::IdList, "None", "the elements to judge"));
        info.parameters.push_back(parameter("types", ParameterKind::TextList, "None", "the element types"));
        info.parameters.push_back(
            parameter("propertyIds", ParameterKind::IdList, "None", "the properties (needs the model)"));
        info.defaultSort = "group";
        ColumnListBuilder columns;
        columns.int32("group")
            .dictionary("criterion")
            .int64("elementCount")
            .int64("negativeCount")
            .real("minMs")
            .real("minRf");
        columns.int32("criticalElementId").int32("criticalSubcase").real("criticalStep").int32("criticalLayer");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMarginSummaryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MarginSummaryQuery>());
}

} // namespace oor::query
