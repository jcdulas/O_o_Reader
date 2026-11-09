#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/analysis/mass/MassProperties.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/common/MassGroups.hpp"
#include "oor/query/named/topology/t7_mass/MassQueries.hpp"

#include <cmath>
#include <limits>
#include <map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// massProperties: mass, centre of gravity and inertia, in total or per group.
class MassPropertiesQuery final : public QueryBase
{
public:
    MassPropertiesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::optional<std::string> by = run.arguments.text("by");
        if (by && *by != "property" && *by != "material" && *by != "type" && *by != "component")
            throw QueryError(
                "massProperties: by is None, \"property\", \"material\", \"type\" or \"component\", not \"" + *by +
                "\"");
        const std::optional<std::array<double, 3>> point = run.arguments.point("referencePoint");
        const std::int64_t cid = run.arguments.integerOr("frameId", 0);
        const model::Model& model = *run.model;
        const CardList elements = listCards(model, typesOfCategory(model::CardCategory::Element));
        std::vector<std::string> labels;
        std::vector<std::uint8_t> labelValid;
        std::vector<GroupTotals> groups;
        if (elements.size() > 0)
        {
            ElementData data(run.model, elements, run.context);
            const ElementMassData& masses = data.masses();
            if (!by)
            {
                GroupTotals total;
                for (std::size_t index = 0; index < elements.size(); ++index)
                    addElementMass(total, masses, index, 1.0, true, true);
                labels.emplace_back("total");
                labelValid.push_back(1);
                groups.push_back(std::move(total));
            }
            else if (*by == "material")
            {
                for (auto& [material, group] : materialGroups(data))
                {
                    if (material == 0 && group.mass() == 0.0)
                        continue;
                    labels.push_back(material != 0 ? std::to_string(material) : std::string());
                    labelValid.push_back(material != 0 ? 1 : 0);
                    groups.push_back(std::move(group));
                }
            }
            else if (*by == "type")
            {
                std::map<std::string, GroupTotals> byType;
                for (std::size_t index = 0; index < elements.size(); ++index)
                    addElementMass(byType[std::string(typeName(elements.types[index]))], masses, index, 1.0, true,
                                   true);
                for (auto& [name, group] : byType)
                {
                    labels.push_back(name);
                    labelValid.push_back(1);
                    groups.push_back(std::move(group));
                }
            }
            else
            {
                // property or component: a number each, 0 for none.
                std::map<std::int32_t, GroupTotals> byNumber;
                if (*by == "property")
                {
                    const std::vector<std::int32_t>& pids = data.properties();
                    for (std::size_t index = 0; index < elements.size(); ++index)
                        addElementMass(byNumber[pids[index]], masses, index, 1.0, true, true);
                }
                else
                {
                    const auto components = analysis::ComponentModel::of(model, run.context.pool());
                    for (const analysis::ComponentSummary& summary : components->components())
                        byNumber[summary.id];
                    for (std::size_t index = 0; index < elements.size(); ++index)
                        addElementMass(byNumber[components->componentOfElement(elements.location(index))], masses,
                                       index, 1.0, true, true);
                }
                for (auto& [number, group] : byNumber)
                {
                    if (number == 0 && group.mass() == 0.0)
                        continue;
                    labels.push_back(number != 0 ? std::to_string(number) : std::string());
                    labelValid.push_back(number != 0 ? 1 : 0);
                    groups.push_back(std::move(group));
                }
            }
        }
        else if (!by)
        {
            labels.emplace_back("total");
            labelValid.push_back(1);
            groups.emplace_back();
        }
        if (groups.empty())
            return emptyTable(info().defaultColumns());

        // The axes the inertia is given along.
        base::Mat3 axes = base::Mat3::identity();
        if (cid != 0)
        {
            analysis::CoordinateTransform transform(model);
            axes = transform.frame(static_cast<base::EntityId>(cid)).axes;
        }
        std::vector<double> mass;
        std::array<std::vector<double>, 3> centre;
        std::array<std::vector<double>, 6> inertia;
        for (const GroupTotals& group : groups)
        {
            mass.push_back(group.mass());
            const base::Vec3 cg = group.sums.centerOfGravity();
            centre[0].push_back(cg.x);
            centre[1].push_back(cg.y);
            centre[2].push_back(cg.z);
            std::array<double, 6> values;
            values.fill(NOT_A_NUMBER);
            if (group.mass() != 0.0 && std::isfinite(cg.x))
            {
                const base::Vec3 reference = point ? base::Vec3{(*point)[0], (*point)[1], (*point)[2]} : cg;
                const base::Mat3 tensor = group.sums.inertiaAbout(reference);
                values = analysis::inertiaProducts(analysis::rotatedInertia(axes.transposed(), tensor));
            }
            for (std::size_t item = 0; item < 6; ++item)
                inertia[item].push_back(values[item]);
        }
        TableBuilder builder;
        builder.text("group", labels, std::move(labelValid));
        builder.real("mass", std::move(mass));
        builder.real("cx", std::move(centre[0]));
        builder.real("cy", std::move(centre[1]));
        builder.real("cz", std::move(centre[2]));
        static const char* const NAMES[6] = {"ixx", "iyy", "izz", "ixy", "ixz", "iyz"};
        for (std::size_t item = 0; item < 6; ++item)
            builder.real(NAMES[item], std::move(inertia[item]));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "massProperties";
        info.family = "T7";
        info.description =
            "The mass, the centre of gravity (in the basic system) and the inertia of the model (by=None: one row, "
            "group "
            "\"total\") or of each property, material, element type or component (group is its ID or name; a null "
            "group is "
            "what has none). The inertia is (Ixx, Iyy, Izz, Ixy, Ixz, Iyz) with the products of inertia positive (the "
            "signs of "
            "the CONM2 fields), about the centre of gravity of the group, or about `referencePoint` "
            "(in "
            "the basic system) when it is given, along the axes of the coordinate system `frameId`. Elements are point "
            "masses at "
            "their centres except the concentrated masses and the beams that have an inertia of their own.";
        info.parameters = {
            parameter("by", ParameterKind::Text, "None", "None, \"property\", \"material\", \"type\" or \"component\""),
            parameter("referencePoint", ParameterKind::Point, "None", "the point the inertia is taken about"),
            parameter("frameId", ParameterKind::Integer, "0", "the coordinate system the inertia is given along")};
        info.defaultSort = "group";
        ColumnListBuilder columns;
        columns.text("group").reals("mass cx cy cz ixx iyy izz ixy ixz iyz");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMassPropertiesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MassPropertiesQuery>());
}

} // namespace oor::query
