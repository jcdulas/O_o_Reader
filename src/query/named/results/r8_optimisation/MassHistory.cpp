#include "../../topology/t9_design/DesignQuerySupport.hpp"
#include "OptimisationData.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r8_optimisation/OptimisationQueries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
constexpr const char* WEIGHT_PROVIDER = "weight response stored with the results";
constexpr const char* MODEL_PROVIDER = "model with the design variables of the cycle applied (model.design.apply)";

// The weight response of a result set that a solver gave in memory, by design cycle: the objective when it is a weight,
// else the first response of the type weight. Empty when the set has none.
std::map<std::int32_t, double> storedWeight(const OptimisationData& data, std::size_t set)
{
    const auto isWeight = [](const DesignResponse& response)
    {
        std::string type = response.type;
        std::transform(type.begin(), type.end(), type.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return type == "weight";
    };
    std::int32_t chosen = 0;
    bool found = false;
    for (const DesignResponse& response : data.responses())
    {
        if (response.set != set || !isWeight(response) || (found && !response.isObjective))
            continue;
        chosen = response.id;
        found = true;
        if (response.isObjective)
            break;
    }
    std::map<std::int32_t, double> weights;
    for (const DesignResponse& response : data.responses())
    {
        if (response.set == set && found && response.id == chosen && std::isfinite(response.value))
            weights.emplace(response.cycle, response.value);
    }
    return weights;
}

// massHistory: the mass of the design at each design cycle.
class MassHistoryQuery final : public ResultQuery
{
public:
    MassHistoryQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const std::string by = context.arguments().has("by") ? context.arguments().textOr("by", "") : std::string();
        if (!by.empty() && by != "property" && by != "material" && by != "component")
            throw QueryError("by is None, \"property\", \"material\" or \"component\", not '" + by + "'");
        const OptimisationData data(context);
        if (data.cycles().empty())
            return emptyTable(info().defaultColumns());
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            std::vector<const DesignCycle*> cycles;
            for (const DesignCycle& item : data.cycles())
            {
                if (item.set == set)
                    cycles.push_back(&item);
            }
            if (cycles.empty())
                continue;
            std::vector<std::int32_t> cycleNumber, group;
            std::vector<std::uint8_t> groupValid;
            std::vector<double> mass;
            std::vector<std::int16_t> provider;
            // the total can come from the weight the solver stored
            const std::map<std::int32_t, double> stored =
                by.empty() ? storedWeight(data, set) : std::map<std::int32_t, double>();
            const bool useStored =
                !stored.empty() && std::all_of(cycles.begin(), cycles.end(),
                                               [&](const DesignCycle* item) { return stored.contains(item->cycle); });
            if (useStored)
            {
                for (const DesignCycle* item : cycles)
                {
                    cycleNumber.push_back(item->cycle);
                    group.push_back(0);
                    groupValid.push_back(0);
                    mass.push_back(stored.at(item->cycle));
                    provider.push_back(sources.of(WEIGHT_PROVIDER));
                }
            }
            else
            {
                fromModel(context, set, by, cycles, sources, cycleNumber, group, groupValid, mass, provider);
            }
            if (cycleNumber.empty())
                continue;
            // the change from the previous cycle of the same group
            std::vector<double> change(mass.size(), NOT_A_NUMBER);
            std::map<std::pair<std::int32_t, bool>, double> previous;
            for (std::size_t row = 0; row < mass.size(); ++row)
            {
                const std::pair<std::int32_t, bool> key{group[row], groupValid[row] != 0};
                if (const auto before = previous.find(key); before != previous.end())
                    change[row] = mass[row] - before->second;
                previous[key] = mass[row];
            }
            RowBlock block(cycleNumber.size());
            block.ints("designCycle", std::move(cycleNumber));
            block.ints("group", std::move(group), std::move(groupValid));
            block.reals("mass", std::move(mass));
            block.reals("change", std::move(change));
            block.providers(std::move(provider));
            block.origin(set, context.sets()[set].analysis, -1);
            block.precision(cycles.front()->precision);
            block.validation("notValidated");
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    // The masses of each cycle, in a copy of the model of the set with the design variables of the cycle applied.
    static void fromModel(const ResultQueryContext& context, std::size_t set, const std::string& by,
                          const std::vector<const DesignCycle*>& cycles, SourceIndex& sources,
                          std::vector<std::int32_t>& cycleNumber, std::vector<std::int32_t>& group,
                          std::vector<std::uint8_t>& groupValid, std::vector<double>& mass,
                          std::vector<std::int16_t>& provider)
    {
        const model::Model* model = context.modelOf(set);
        if (model == nullptr)
            (void)context.requireModel(
                "add up the mass of the design at each cycle: the design variables of the cycle are applied to a copy "
                "of "
                "the model (unless the results store a weight response, which gives the total)");
        if (model == nullptr)
            throw QueryError("massHistory: the result set '" + context.sets()[set].analysis +
                             "' is not attached to a model");
        const model::DesignModel design = model::DesignModel::read(*model);
        if (design.variables().empty())
            throw QueryError("massHistory: the model has no design variables (DESVAR) to apply the cycles to");
        const CardList elements = listCards(*model, typesOfCategory(model::CardCategory::Element));
        base::ThreadPool* pool = context.exec().pool();
        std::shared_ptr<const analysis::ComponentModel> components;
        if (by == "component")
            components = analysis::ComponentModel::of(*model, pool);
        const std::int16_t source = sources.of(MODEL_PROVIDER);
        for (const DesignCycle* item : cycles)
        {
            const model::Model work = modelAt(*model, design, item->values);
            const ElementMassList masses = massOfEach(work, elements, pool);
            std::map<std::int32_t, double> byKey; // 0 for none
            double total = 0.0;
            for (std::size_t position = 0; position < elements.size(); ++position)
            {
                if (!std::isfinite(masses.mass[position]))
                    continue;
                total += masses.mass[position];
                if (by == "property")
                    byKey[masses.property[position]] += masses.mass[position];
                else if (by == "material")
                    byKey[masses.material[position]] += masses.mass[position];
                else if (by == "component")
                    byKey[components->componentOfElement(elements.location(position))] += masses.mass[position];
            }
            if (by.empty())
            {
                cycleNumber.push_back(item->cycle);
                group.push_back(0);
                groupValid.push_back(0);
                mass.push_back(total);
                provider.push_back(source);
                continue;
            }
            // the groups in order, the elements that belong to none last
            std::vector<std::int32_t> keys;
            for (const auto& entry : byKey)
            {
                if (entry.first != 0)
                    keys.push_back(entry.first);
            }
            if (byKey.contains(0))
                keys.push_back(0);
            for (const std::int32_t key : keys)
            {
                cycleNumber.push_back(item->cycle);
                group.push_back(key);
                groupValid.push_back(key != 0 ? 1 : 0);
                mass.push_back(byKey.at(key));
                provider.push_back(source);
            }
        }
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "massHistory";
        info.family = "R8";
        info.resultQuery = true;
        // It needs the model unless the results store a weight response: the query says so when it cannot go on.
        info.description =
            "The mass of the design at each design cycle: in total (`by` None), or by property, material or component. "
            "The design variables of each cycle (their values in the result set) are applied to a copy of the model "
            "through `model.design.apply` (the limits of the relations are not applied, and a relation that cannot be "
            "applied is left out) and the elements are weighed as `massByProperty`, `massByMaterial` and "
            "`massByComponent` do: the structural and non-structural mass of the cards and of the NSM set the Case "
            "Control selects, with no WTMASS. A variable a cycle does not give keeps its initial value (XINIT). "
            "`change` is the difference from the previous cycle of the same group (null at the first). A result set "
            "that stores a weight response (a solver in memory: the named result `responseValues`, a response of the "
            "type weight, the objective first) gives the total from it with no model (every other form needs the "
            "model). `group` is the property, material "
            "or component ID, null for the total and for the elements that have none. A material group takes the mass "
            "of the elements that use one material; a laminate counts in the group null.";
        info.parameters = {parameter("by", ParameterKind::Text, "None",
                                     "None (the total), \"property\", \"material\" or \"component\"")};
        info.defaultSort = "designCycle, group";
        ColumnListBuilder columns;
        columns.int32("designCycle").int32("group").real("mass").real("change");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMassHistoryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MassHistoryQuery>());
}

} // namespace oor::query
