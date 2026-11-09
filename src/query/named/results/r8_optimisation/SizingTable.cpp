#include "../r5_margins/MarginData.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/results/r8_optimisation/OptimisationQueries.hpp"
#include "oor/query/providers/quantities/ElementModelFacts.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <cmath>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// sizingTable: for each zone, the smallest margin and the factor on the thickness that would bring it to zero (a fully
// stressed design).
class SizingTableQuery final : public ResultQuery
{
public:
    SizingTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const std::string by = context.arguments().textOr("by", "property");
        if (by != "property" && by != "material" && by != "component")
            throw QueryError("by is \"property\", \"material\" or \"component\", not '" + by + "'");
        context.requireModel(
            "size the zones: the thickness, the area and the mass of a zone are read from the property and material "
            "cards");
        MarginSet margins = computeMargins(context);
        if (margins.rows.empty())
            return emptyTable(info().defaultColumns());
        // the design cycle: the one asked for, else the last the margins have
        std::int32_t cycle = -1;
        if (context.arguments().has("designCycle"))
            cycle = static_cast<std::int32_t>(context.arguments().integerOr("designCycle", 0));
        else
            for (const MarginRow& row : margins.rows)
                cycle = std::max(cycle, row.designCycle);
        ResultModelIndex index(context);
        // the most critical row of each element
        std::map<std::pair<std::size_t, std::int32_t>, const MarginRow*> worst;
        for (const MarginRow& row : margins.rows)
        {
            if (std::isnan(row.ms) || row.designCycle != cycle)
                continue;
            auto [position, inserted] = worst.try_emplace({row.set, row.eid}, &row);
            if (!inserted && row.ms < position->second->ms)
                position->second = &row;
        }
        struct Zone
        {
            const MarginRow* critical = nullptr;
            double mass = 0.0;
            bool massKnown = true;
            std::set<std::int32_t> properties;
            std::size_t set = 0;
        };
        std::map<std::int32_t, Zone> zones;
        // the mass of the elements of the model, by element ID
        std::map<const model::Model*, std::map<std::int32_t, double>> massOf;
        const auto elementMass = [&](std::size_t set, std::int32_t modelId) -> double
        {
            const model::Model* model = context.modelOf(set);
            if (model == nullptr)
                return NOT_A_NUMBER;
            auto found = massOf.find(model);
            if (found == massOf.end())
            {
                const CardList elements = listCards(*model, typesOfCategory(model::CardCategory::Element));
                std::map<std::int32_t, double> masses;
                if (elements.size() > 0)
                {
                    ElementData data(context.modelPointer(), elements, context.exec());
                    const ElementMassData& values = data.masses();
                    for (std::size_t position = 0; position < elements.size(); ++position)
                        masses[elements.ids[position]] = values.mass[position];
                }
                found = massOf.emplace(model, std::move(masses)).first;
            }
            const auto item = found->second.find(modelId);
            return item == found->second.end() ? NOT_A_NUMBER : item->second;
        };
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
            Zone& zone = zones[id];
            if (zone.critical == nullptr || row->ms < zone.critical->ms)
            {
                zone.critical = row;
                zone.set = row->set;
            }
            const double mass = elementMass(row->set, context.elementCard(row->set, row->eid).modelId);
            if (std::isnan(mass))
                zone.massKnown = false;
            else
                zone.mass += mass;
            zone.properties.insert(row->pid);
        }
        if (zones.empty())
            return emptyTable(info().defaultColumns());
        std::vector<std::int32_t> group, subcase, eid;
        std::vector<std::uint8_t> massValid;
        std::vector<double> thickness, area, mass, minMs, minRf, resize, suggested, change;
        std::vector<std::int32_t> variable;
        std::vector<std::uint8_t> variableValid;
        std::map<std::size_t, std::unique_ptr<ElementModelFacts>> facts;
        std::map<std::size_t, model::DesignModel> designs;
        for (const auto& [id, zone] : zones)
        {
            const MarginRow& critical = *zone.critical;
            auto found = facts.find(zone.set);
            if (found == facts.end())
                found = facts.emplace(zone.set, std::make_unique<ElementModelFacts>(context, zone.set)).first;
            ElementModelFacts& model = *found->second;
            // the thickness of a shell and the area of a rod of the zone, from its most critical element
            double t = NOT_A_NUMBER;
            double a = NOT_A_NUMBER;
            if (model.hasModel())
            {
                const ShellFacts& shell = model.shell(critical.eid);
                if (shell.ok)
                    t = shell.thickness;
                const RodFacts& rod = model.rod(critical.eid);
                if (rod.ok)
                    a = rod.area;
            }
            const double rf = critical.rf;
            const double resizeFactor = std::isfinite(rf) && rf > 0.0 ? 1.0 / rf : NOT_A_NUMBER;
            const double zoneMass = zone.massKnown ? zone.mass : NOT_A_NUMBER;
            group.push_back(id);
            // the variable that drives the zone: of the relation on its property (the property of the critical element
            // for a component) or on its material
            std::optional<base::EntityId> driver;
            if (const model::Model* modelOfZone = context.modelOf(zone.set))
            {
                auto design = designs.find(zone.set);
                if (design == designs.end())
                    design = designs.emplace(zone.set, model::DesignModel::read(*modelOfZone)).first;
                driver = by == "material" ? design->second.variableDriving(model::DesignTargetKind::Material, id)
                                          : design->second.variableDriving(model::DesignTargetKind::Property,
                                                                           by == "property" ? id : critical.pid);
            }
            variable.push_back(driver.value_or(0));
            variableValid.push_back(driver ? 1 : 0);
            thickness.push_back(t);
            area.push_back(a);
            mass.push_back(zoneMass);
            minMs.push_back(critical.ms);
            minRf.push_back(rf);
            eid.push_back(critical.eid);
            subcase.push_back(critical.subcase);
            resize.push_back(resizeFactor);
            suggested.push_back(std::isnan(t) ? NOT_A_NUMBER : t * resizeFactor);
            // the mass of a zone follows its thickness (a shell) or its area (a rod)
            change.push_back(std::isnan(zoneMass) || (std::isnan(t) && std::isnan(a))
                                 ? NOT_A_NUMBER
                                 : zoneMass * (resizeFactor - 1.0));
        }
        RowAccumulator accumulator;
        const std::int16_t provider =
            accumulator.addProvider("fully stressed design: resize factor = 1 / reserve factor", true);
        RowBlock block(group.size());
        block.ints("group", std::move(group));
        block.ints("designVariable", std::move(variable), std::move(variableValid));
        block.reals("thickness", std::move(thickness));
        block.reals("area", std::move(area));
        block.reals("mass", std::move(mass));
        block.reals("minMs", std::move(minMs));
        block.reals("minRf", std::move(minRf));
        block.ints("criticalElementId", std::move(eid));
        block.ints("criticalSubcase", std::move(subcase));
        block.reals("resizeFactor", std::move(resize));
        block.reals("suggestedThickness", std::move(suggested));
        block.reals("suggestedMassChange", std::move(change));
        block.origin(0, context.sets().front().analysis, -1);
        block.precision(Precision::Double);
        block.validation("notValidated");
        block.provider(provider);
        accumulator.add(std::move(block));
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "sizingTable";
        info.family = "R8";
        info.resultQuery = true;
        info.needsModel = true;
        // Deprecated: a fully stressed design is a sizing method, not what a reader of results gives; kept through
        // 1.x, removed in 2.0.
        info.deprecation =
            "sizingTable is deprecated and will be removed in oor_reader 2.0: a fully stressed "
            "design is a sizing method; `margins` gives the reserve factors it starts from";
        info.description =
            "Deprecated, to be removed in 2.0 (a sizing method: `margins` gives the reserve factors it starts "
            "from). For each zone (property, material or component, `by`), the smallest margin and reserve factor of "
            "its "
            "elements for "
            "the criterion (the parameters are those of `margins`), where it occurs, and the fully stressed design "
            "that follows: "
            "`resizeFactor` is `1 / minRf`, the factor on the thickness that would bring the margin to zero if the "
            "internal "
            "loads did not change; `suggestedThickness` is the thickness of the zone (a shell of a PSHELL with a "
            "MAT1) times it "
            "and `suggestedMassChange` the mass of the judged elements times `resizeFactor - 1` (the mass follows "
            "the "
            "thickness of a shell and the area of a rod). It is a heuristic, marked approximate: it ignores the "
            "redistribution of "
            "the loads, bending (where the stress does not scale as `1 / thickness`) and buckling. `designVariable` "
            "is the design variable that drives the zone: the first variable of the first relation (DVPREL1, DVPREL2, "
            "DVMREL1, DVMREL2) on the property of the zone (on its material for `by=\"material\"`, on the property of "
            "the critical element for a component) that drives a thickness, an area, a density or a non-structural "
            "mass; null when none does. `designCycle` defaults to the last cycle the results have.";
        info.parameters = {
            parameter("criterion", ParameterKind::Text, "(required)",
                      "\"vonMises\", \"maxPrincipal\", \"minPrincipal\", \"composite\", \"bond\" or \"fastener\""),
            parameter("by", ParameterKind::Text, "\"property\"", "\"property\", \"material\" or \"component\""),
            parameter("allowables", ParameterKind::Values, "None",
                      "a dict keyed by (\"material\", mid), (\"property\", propId) or (\"element\", elementId), or one "
                      "value for all"),
            parameter("factor", ParameterKind::Real, "1.0", "the factor of safety on the loads"),
            parameter("designCycle", ParameterKind::Integer, "None", "the design cycle (the last by default)")};
        info.defaultSort = "group";
        ColumnListBuilder columns;
        columns.int32("group")
            .int32("designVariable")
            .real("thickness")
            .real("area")
            .real("mass")
            .real("minMs")
            .real("minRf");
        columns.int32("criticalElementId").int32("criticalSubcase").real("resizeFactor").real("suggestedThickness");
        columns.real("suggestedMassChange");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addSizingTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SizingTableQuery>());
}

} // namespace oor::query
