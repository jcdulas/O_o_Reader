#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrUnion/MultiPointConstrUnionTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <algorithm>
#include <set>

namespace oor::query
{

namespace
{

// multiPointConstrTable: the terms of the MPC equations of the sets asked for, MPCADD expanded.
class MultiPointConstrTableQuery final : public QueryBase
{
public:
    MultiPointConstrTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const model::MultiPointConstrTable* table = model.findTable<model::MultiPointConstrTable>();
        // The sets asked for: the IDs given, the ones the subcases select, else every set of the model.
        std::set<base::EntityId> sets;
        bool asked = false;
        if (const auto ids = run.arguments.ids("setIds"))
        {
            asked = true;
            for (const std::int64_t id : *ids)
                sets.insert(static_cast<base::EntityId>(id));
        }
        if (const auto numbers = run.arguments.ids("subcases"))
        {
            asked = true;
            const model::CaseControl& caseControl = model.control().caseControl;
            const model::CaseLineages lineages(caseControl);
            for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
            {
                if (std::find(numbers->begin(), numbers->end(), subcase->id) == numbers->end())
                    continue;
                if (const auto sid = analysis::effectiveInteger(lineages, *subcase, "MultiPointConstrSelection"))
                    sets.insert(static_cast<base::EntityId>(*sid));
            }
        }
        if (!asked)
        {
            if (table != nullptr)
            {
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (table->rows().isAlive(row))
                        sets.insert(table->idAt(row));
                }
            }
            analysis::forEachCard<model::MultiPointConstrUnionTable>(
                model,
                [&](const model::MultiPointConstrUnionCard& card, model::CardLocation) { sets.insert(card.id); });
        }
        std::vector<std::int32_t> sid;
        std::vector<std::int32_t> source;
        std::vector<std::int32_t> equation;
        std::vector<std::int32_t> term;
        std::vector<std::int32_t> nid;
        std::vector<std::int32_t> dof;
        std::vector<double> coefficient;
        std::vector<std::uint8_t> dependent;
        if (table != nullptr)
        {
            for (const base::EntityId set : sets)
            {
                const std::vector<base::EntityId> members =
                    analysis::expandedConstraintSet(model, model::MULTI_POINT_CONSTR_SELECTION, set);
                std::int32_t number = 0;
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (!table->rows().isAlive(row))
                        continue;
                    const model::MultiPointConstrCard card = table->card(row);
                    if (!std::binary_search(members.begin(), members.end(), card.id))
                        continue;
                    ++number;
                    std::int32_t position = 0;
                    for (const model::MultiPointConstrTerm& item : card.terms)
                    {
                        ++position;
                        sid.push_back(set);
                        source.push_back(card.id);
                        equation.push_back(number);
                        term.push_back(position);
                        nid.push_back(item.node);
                        dof.push_back(item.component);
                        coefficient.push_back(item.coefficient);
                        dependent.push_back(position == 1 ? 1 : 0);
                    }
                }
            }
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.int32("sourceSetId", std::move(source));
        builder.int32("equation", std::move(equation));
        builder.int32("term", std::move(term));
        builder.int32("nodeId", std::move(nid));
        builder.int32("dof", std::move(dof));
        builder.real("coefficient", std::move(coefficient));
        builder.flag("isDependent", dependent);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "multiPointConstrTable";
        info.family = "T6";
        info.description =
            "One row for each term of each MPC equation of the sets asked for, MPCADD expanded: setId is the set asked "
            "for, sourceSetId the set of the MPC card of the equation (the setId of a set reached through an MPCADD), "
            "equation the position (from 1) of the equation among those of the set in the order of the cards, term the "
            "position of the term in the equation, nodeId and dof (the component 1 to 6; 0 for a scalar point) the "
            "degree "
            "of "
            "freedom, coefficient its coefficient, isDependent true for the first term (the dependent degree of "
            "freedom). "
            "setIds and subcases name the sets (a subcase selects the one its MPC command names); with neither, every "
            "set of "
            "the model.";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the MPC sets to list"),
                           parameter("subcases", ParameterKind::IdList, "None", "the sets these subcases select")};
        info.defaultSort = "setId, equation, term";
        ColumnListBuilder columns;
        columns.int32("setId")
            .int32("sourceSetId")
            .int32("equation")
            .int32("term")
            .int32("nodeId")
            .int32("dof")
            .real("coefficient")
            .flag("isDependent");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMultiPointConstrTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MultiPointConstrTableQuery>());
}

} // namespace oor::query
