#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/analysis/connections/RigidElementTerms.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <string>

namespace oor::query
{

namespace
{

// rigidElementsTable: one row for each node of each rigid element.
class RigidElementsTableQuery final : public QueryBase
{
public:
    RigidElementsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        std::vector<std::uint16_t> types;
        const std::vector<std::uint16_t> wanted = typeNames.empty()
                                                      ? typesOfCategory(model::CardCategory::Element)
                                                      : typesNamed(typeNames, model::CardCategory::Element, "types");
        for (const std::uint16_t type : wanted)
        {
            if (analysis::roleOfType(type) == analysis::ElementRole::Rigid)
                types.push_back(type);
        }
        const CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        std::vector<std::int32_t> eid;
        std::vector<std::string> type;
        std::vector<std::string> role;
        std::vector<std::int32_t> nid;
        std::vector<std::string> dof;
        std::vector<std::uint8_t> dofValid;
        std::vector<double> weight;
        std::vector<double> alpha;
        std::vector<double> tref;
        for (std::size_t index = 0; index < list.size(); ++index)
        {
            analysis::RigidThermal thermal;
            for (const analysis::RigidTerm& row :
                 analysis::RigidElementTerms::of(model, list.location(index), &thermal))
            {
                alpha.push_back(thermal.alpha);
                tref.push_back(thermal.tref);
                eid.push_back(list.ids[index]);
                type.emplace_back(typeName(list.types[index]));
                role.emplace_back(row.isIndependent ? "independent" : "dependent");
                nid.push_back(row.node);
                dof.push_back(row.components != 0 ? std::to_string(row.components) : std::string());
                dofValid.push_back(row.components != 0 ? 1 : 0);
                weight.push_back(row.weight);
            }
        }
        TableBuilder builder;
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        builder.dictionary("role", role);
        builder.int32("nodeId", std::move(nid));
        builder.text("dof", dof, std::move(dofValid));
        builder.real("weight", std::move(weight));
        builder.real("alpha", std::move(alpha));
        builder.real("referenceTemperature", std::move(tref));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "rigidElementsTable";
        info.family = "T6";
        info.description =
            "One row for each node of each rigid element (RBE1, RBE2, RBE3, RBAR, RBAR1, RROD, RSPLINE, RJOINT, "
            "RTRPLT, "
            "RTRPLT1): its role, independent or dependent, the components (dof, the digits as text) it is that for, "
            "and the "
            "weight (RBE3). RBE2: the independent grid point GN with all its components, the dependent ones with CM. "
            "RBE3: "
            "the weighted grid points are independent, the reference grid point and the UM list dependent. RBAR: CNA "
            "and "
            "CNB independent, CMA and CMB dependent (a node can have both rows). RROD: the end with CMA or CMB "
            "dependent, "
            "the other independent with no dof. RSPLINE: the two ends independent, the points between dependent. "
            "Independent rows come before dependent ones, in the order of the card. alpha and referenceTemperature are "
            "the ALPHA and "
            "TREF of the element, repeated on each of its rows (null for RJOINT and RSPLINE, which have none).";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "keep these elements"),
                           parameter("types", ParameterKind::TextList, "None", "keep these card types")};
        info.defaultSort = "elementId (then the order of the card)";
        ColumnListBuilder columns;
        columns.int32("elementId").dictionary("type").dictionary("role").int32("nodeId").text("dof").real("weight");
        columns.reals("alpha referenceTemperature");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addRigidElementsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<RigidElementsTableQuery>());
}

} // namespace oor::query
