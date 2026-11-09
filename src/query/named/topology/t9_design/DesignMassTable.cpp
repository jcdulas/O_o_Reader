#include "DesignQuerySupport.hpp"
#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
constexpr double RELATIVE_STEP = 1.0e-4;

bool addFinite(double& sum, double value)
{
    if (!std::isfinite(value))
        return false;
    sum += value;
    return true;
}

// True when the mass of the elements a variable drives is a linear function of the variable: every relation it moves is
// a linear one, and drives a field the mass does not depend on or depends on linearly (a thickness, an area, a density,
// a non-structural mass).
bool isExact(const model::DesignModel& design, const std::vector<std::size_t>& relations)
{
    for (const std::size_t index : relations)
    {
        const model::DesignRelation& relation = design.relations()[index];
        if (!relation.applicable || relation.facts.massEffect == model::DesignMassEffect::None)
            continue;
        if (relation.shape != model::DesignRelationShape::Linear ||
            relation.facts.massEffect != model::DesignMassEffect::Linear)
            return false;
    }
    return true;
}

// designMassTable: the mass each variable carries and what the mass of the model does when it changes.
class DesignMassTableQuery final : public QueryBase
{
public:
    DesignMassTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const model::DesignModel design = model::DesignModel::read(model);
        if (design.variables().empty())
            return emptyTable(info().defaultColumns());
        DrivenCards driven(run.model, design, run.context);
        const std::vector<std::vector<std::size_t>> moved = relationsMovedBy(design);
        base::ThreadPool* pool = run.context.pool();

        const analysis::MassTotals total = analysis::MassCalculator::total(
            model, analysis::MassCalculator::allElements(model), MassSetting(model, pool).options(), pool);
        const double modelMass = total.sums.mass();

        const std::size_t count = design.variables().size();
        std::vector<std::int32_t> id(count);
        std::vector<std::string> label(count);
        std::vector<double> value(count), mass(count, 0.0), fraction(count), sensitivity(count, NOT_A_NUMBER);
        std::vector<std::uint8_t> exact(count, 0);
        const ElementMassData& masses = driven.data().masses();
        std::map<std::int32_t, double> initial;
        for (const model::DesignVariable& variable : design.variables())
            initial[variable.id] = variable.initial;
        for (std::size_t row = 0; row < count; ++row)
        {
            run.context.checkCancelled();
            const model::DesignVariable& variable = design.variables()[row];
            id[row] = variable.id;
            label[row] = variable.label;
            value[row] = variable.initial;
            const RelationDrive drive = driven.of(moved[row]);
            std::vector<model::CardLocation> locations;
            for (const std::size_t element : drive.elements)
            {
                addFinite(mass[row], masses.mass[element]);
                locations.push_back(driven.elements().location(element));
            }
            fraction[row] = modelMass != 0.0 ? mass[row] / modelMass : NOT_A_NUMBER;
            // A dependent variable is given by its DLINK: it has no value of its own to move.
            if (variable.link != 0)
                continue;
            // A variable that moves grid points (DVGRID) changes the size of the elements: not a linear function.
            const bool movesGrids =
                std::any_of(design.shapeTerms().begin(), design.shapeTerms().end(),
                            [&](const model::DesignShapeTerm& term) { return term.variable == variable.id; });
            exact[row] = isExact(design, moved[row]) && !movesGrids ? 1 : 0;
            if (locations.empty())
            {
                sensitivity[row] = 0.0;
                continue;
            }
            // The central difference of the mass of the elements the variable drives, in two copies of the model with
            // the variable moved either way. The mass is linear in the fields that a relation to a thickness, an area,
            // a density or a non-structural mass gives, and the relation is linear, so the difference is exact then.
            const double step = RELATIVE_STEP * (std::fabs(variable.initial) > 0.0 ? std::fabs(variable.initial) : 1.0);
            std::map<std::int32_t, double> up = initial;
            std::map<std::int32_t, double> down = initial;
            up[variable.id] += step;
            down[variable.id] -= step;
            const auto massAt = [&](const std::map<std::int32_t, double>& values)
            { return massOfElements(modelAt(model, design, values, model::DesignLimitMode::Clip), locations, pool); };
            sensitivity[row] = (massAt(up) - massAt(down)) / (2.0 * step);
        }
        std::vector<std::uint8_t> sensitivityValid(count);
        for (std::size_t row = 0; row < count; ++row)
            sensitivityValid[row] = std::isnan(sensitivity[row]) ? 0 : 1;
        TableBuilder builder;
        builder.int32("designVariable", std::move(id));
        builder.text("label", label);
        builder.real("value", std::move(value));
        builder.real("mass", std::move(mass));
        builder.real("massFraction", std::move(fraction));
        builder.real("massSensitivity", std::move(sensitivity), std::move(sensitivityValid));
        builder.flag("isExact", exact);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designMassTable";
        info.family = "T9";
        info.description =
            "The mass of the elements each design variable drives (`mass`, with its `massFraction` of the model) and "
            "the "
            "change of the mass of the model per unit of the variable (`massSensitivity`), at the initial values of "
            "the "
            "variables. The sensitivity is the central difference of the mass of the driven elements between two "
            "copies "
            "of the model with the variable moved a little either way (`model.design.apply` on each): it is exact "
            "(`isExact`) when every relation the variable moves is linear and drives a field the mass depends on "
            "linearly or not at all (a thickness, an area, a density, a non-structural mass, a stiffness), and an "
            "approximation of the derivative when a relation goes through an equation or drives a dimension of a "
            "section. A variable a DLINK makes dependent has no sensitivity of its own (null): the independent "
            "variables "
            "carry it. The mass counts the structural and non-structural mass of the cards and of the NSM set the Case "
            "Control selects, with no WTMASS, as `massByProperty` does.";
        info.defaultSort = "designVariable";
        ColumnListBuilder columns;
        columns.int32("designVariable").text("label").reals("value mass massFraction massSensitivity");
        columns.flag("isExact");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignMassTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignMassTableQuery>());
}

} // namespace oor::query
