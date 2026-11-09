#include "../../topology/t9_design/DesignQuerySupport.hpp"
#include "OptimisationData.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r8_optimisation/OptimisationQueries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// designComparison: two design cycles side by side.
class DesignComparisonQuery final : public ResultQuery
{
public:
    DesignComparisonQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const std::string detail = context.arguments().textOr("detail", "variables");
        if (detail != "variables" && detail != "fields")
            throw QueryError("detail is \"variables\" or \"fields\", not '" + detail + "'");
        if (detail == "fields")
            (void)context.requireModel(
                "list the fields the design variables drive: the design relations (DVPREL1, DVPREL2, DVMREL1, DVMREL2, "
                "DVCREL1, DVCREL2) are evaluated with the variables of each cycle and the elements they drive are "
                "weighed");
        const OptimisationData data(context);
        if (data.cycles().empty())
            return emptyTable(info().columnsFor(detail));
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            // the first and the last cycle that have design variables (a file may hold responses of a cycle 0 that its
            // variable history does not have)
            std::int32_t first = std::numeric_limits<std::int32_t>::max();
            std::int32_t last = -1;
            for (const DesignCycle& item : data.cycles())
            {
                if (item.set != set)
                    continue;
                first = std::min(first, item.cycle);
                last = std::max(last, item.cycle);
            }
            if (last < 0)
                continue;
            const std::int32_t cycleA = context.arguments().has("cycleA")
                                            ? static_cast<std::int32_t>(context.arguments().integerOr("cycleA", first))
                                            : first;
            const std::int32_t cycleB = context.arguments().has("cycleB")
                                            ? static_cast<std::int32_t>(context.arguments().integerOr("cycleB", last))
                                            : last;
            const DesignCycle* a = data.cycle(set, cycleA);
            const DesignCycle* b = data.cycle(set, cycleB);
            if (a == nullptr || b == nullptr)
            {
                const std::int32_t missing = a == nullptr ? cycleA : cycleB;
                throw QueryError("the design cycle " + std::to_string(missing) + " is not one of the result set '" +
                                 context.sets()[set].analysis + "' (or the designCycles parameter left it out)");
            }
            RowBlock block = detail == "variables" ? variables(context, data, set, *a, *b, sources)
                                                   : fields(context, set, *a, *b, sources);
            if (block.rows() == 0)
                continue;
            block.origin(set, context.sets()[set].analysis, -1);
            block.precision(b->precision);
            block.validation(b->status);
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
            return emptyTable(info().columnsFor(detail));
        return accumulator.finish(context, info(), detail);
    }

private:
    // detail="variables": the value of each variable at the two cycles.
    static RowBlock variables(const ResultQueryContext& context, const OptimisationData& data, std::size_t set,
                              const DesignCycle& a, const DesignCycle& b, SourceIndex& sources)
    {
        (void)context;
        std::vector<std::int32_t> order;
        for (const DesignVariable& variable : data.variables(set))
        {
            if (a.values.contains(variable.id) || b.values.contains(variable.id))
                order.push_back(variable.id);
        }
        for (const DesignCycle* cycle : {&a, &b})
        {
            for (const auto& entry : cycle->values)
            {
                if (std::find(order.begin(), order.end(), entry.first) == order.end())
                    order.push_back(entry.first);
            }
        }
        std::vector<std::int32_t> id;
        std::vector<std::string> label;
        std::vector<double> valueA, valueB, change, relative;
        std::vector<std::int16_t> provider;
        for (const std::int32_t variableId : order)
        {
            const DesignVariable* variable = data.variable(set, variableId);
            const auto itemA = a.values.find(variableId);
            const auto itemB = b.values.find(variableId);
            const double x = itemA == a.values.end() ? NOT_A_NUMBER : itemA->second;
            const double y = itemB == b.values.end() ? NOT_A_NUMBER : itemB->second;
            id.push_back(variableId);
            label.push_back(variable != nullptr ? variable->label : std::string());
            valueA.push_back(x);
            valueB.push_back(y);
            change.push_back(y - x);
            relative.push_back(x != 0.0 && !std::isnan(x) ? (y - x) / std::fabs(x) : NOT_A_NUMBER);
            provider.push_back(sources.of(b.source));
        }
        RowBlock block(id.size());
        if (id.empty())
            return block;
        block.ints("designVariable", std::move(id));
        block.texts("label", std::move(label));
        block.reals("valueA", std::move(valueA));
        block.reals("valueB", std::move(valueB));
        block.reals("change", std::move(change));
        block.reals("relativeChange", std::move(relative));
        block.providers(std::move(provider));
        return block;
    }

    // detail="fields": the field each relation drives at the two cycles, and the mass of the elements it drives. The
    // variables of a cycle go through the relations of the model (`model.design`): the values are those the relations
    // give, and the mass is that of a copy of the model with the cycle applied.
    static RowBlock fields(const ResultQueryContext& context, std::size_t set, const DesignCycle& a,
                           const DesignCycle& b, SourceIndex& sources)
    {
        const model::Model* model = context.modelOf(set);
        if (model == nullptr)
            throw QueryError("designComparison with detail=\"fields\": the result set '" +
                             context.sets()[set].analysis + "' is not attached to a model");
        const model::DesignModel design = model::DesignModel::read(*model);
        const auto valuesAt = [&](const DesignCycle& cycle)
        {
            model::DesignPoint point = design.initialPoint();
            for (const auto& [id, value] : cycle.values)
            {
                if (const std::optional<std::size_t> index = design.findVariable(id))
                    point.values[*index] = value;
            }
            return design.evaluate(*model, point, false);
        };
        const model::DesignEvaluation evaluationA = valuesAt(a);
        const model::DesignEvaluation evaluationB = valuesAt(b);

        // the mass of what each relation drives, in the copy of each cycle
        const auto shared = std::make_shared<model::Model>(model->copy());
        DrivenCards driven(shared, design, context.exec());
        base::ThreadPool* pool = context.exec().pool();
        const ElementMassList massesA = massOfEach(modelAt(*model, design, a.values), driven.elements(), pool);
        const ElementMassList massesB = massOfEach(modelAt(*model, design, b.values), driven.elements(), pool);
        const auto massOf = [&](const ElementMassList& masses, const std::vector<std::size_t>& elements)
        {
            double total = 0.0;
            for (const std::size_t element : elements)
            {
                if (std::isfinite(masses.mass[element]))
                    total += masses.mass[element];
            }
            return total;
        };

        std::vector<std::string> card, field;
        std::vector<std::int32_t> id;
        std::vector<double> valueA, valueB, massA, massB, change;
        std::vector<std::int16_t> provider;
        const std::int16_t source =
            sources.of("design relations evaluated with the variables of the cycles (model.design)");
        for (std::size_t index = 0; index < design.relations().size(); ++index)
        {
            const model::DesignRelation& relation = design.relations()[index];
            if (!relation.applicable || !evaluationA.relations[index].problem.empty() ||
                !evaluationB.relations[index].problem.empty())
                continue;
            const std::vector<std::size_t>& elements = driven.of(index).elements;
            card.push_back(relation.targetCard);
            id.push_back(relation.targetId);
            field.push_back(relation.facts.name);
            valueA.push_back(evaluationA.relations[index].requested);
            valueB.push_back(evaluationB.relations[index].requested);
            massA.push_back(massOf(massesA, elements));
            massB.push_back(massOf(massesB, elements));
            change.push_back(massB.back() - massA.back());
            provider.push_back(source);
        }
        RowBlock block(id.size());
        if (id.empty())
            return block;
        block.texts("targetCard", std::move(card));
        block.ints("targetId", std::move(id));
        block.texts("field", std::move(field));
        block.reals("valueA", std::move(valueA));
        block.reals("valueB", std::move(valueB));
        block.reals("massA", std::move(massA));
        block.reals("massB", std::move(massB));
        block.reals("massChange", std::move(change));
        block.providers(std::move(provider));
        return block;
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designComparison";
        info.family = "R8";
        info.resultQuery = true;
        info.detailParameter = "detail";
        info.description =
            "Two design cycles side by side. `detail=\"variables\"` (the default): the value of each design variable "
            "at `cycleA` "
            "(the first cycle by default) and at `cycleB` (the last), its change and its relative change. "
            "`detail=\"fields\"` (needs the model): one row for each design relation that can be applied "
            "(`model.design.relations()`), with the value it gives the field of the property, material or element at "
            "the "
            "two cycles (the values of the variables of each cycle go through the relations; a variable a cycle does "
            "not "
            "give keeps its initial value), and the mass of the elements it drives in a copy of the model with each "
            "cycle "
            "applied (`model.design.apply`, as `massHistory` does) and its change.";
        info.parameters = {
            parameter("cycleA", ParameterKind::Integer, "None", "the first cycle (the first by default)"),
            parameter("cycleB", ParameterKind::Integer, "None", "the second cycle (the last by default)"),
            parameter("detail", ParameterKind::Text, "\"variables\"", "\"variables\" or \"fields\"")};
        info.defaultSort = "designVariable";
        ColumnListBuilder variables;
        variables.int32("designVariable")
            .dictionary("label")
            .real("valueA")
            .real("valueB")
            .real("change")
            .real("relativeChange");
        info.columnSets.push_back(variables.build("variables"));
        ColumnListBuilder fields;
        fields.dictionary("targetCard")
            .int32("targetId")
            .dictionary("field")
            .real("valueA")
            .real("valueB")
            .real("massA");
        fields.real("massB").real("massChange");
        info.columnSets.push_back(fields.build("fields"));
        return info;
    }
};

} // namespace

void addDesignComparisonQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignComparisonQuery>());
}

} // namespace oor::query
