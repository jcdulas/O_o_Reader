#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/enforcedMotionConstr/EnforcedMotionConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrUnion/SinglePointConstrUnionTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

struct Row
{
    std::int32_t sid = 0;
    bool sidValid = true;
    std::int32_t source = 0;
    const char* card = "SinglePointConstr";
    std::int32_t node = 0;
    std::string dof;
    double enforced = NOT_A_NUMBER;
    double scale = NOT_A_NUMBER; // the factors of the LOAD combinations an SPCD was reached through
};

std::string digits(std::int32_t components)
{
    return components != 0 ? std::to_string(components) : std::string();
}

// singlePointConstrTable: the single point constraints of the sets asked for, SPCADD expanded, with the enforced
// motions and the permanent constraints of the grid points.
class SinglePointConstrTableQuery final : public QueryBase
{
public:
    SinglePointConstrTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // the cards of every load set, read once for all the sets
        const analysis::LoadSetIndex loadSetCards(model);
        const bool permanent = run.arguments.boolOr("includePermanent", true);
        // The constraint sets asked for (SPC command), else all that SPC and SPC1 cards define (and SPCADD).
        std::set<base::EntityId> sets;
        bool asked = false;
        if (const auto ids = run.arguments.ids("setIds"))
        {
            asked = true;
            for (const std::int64_t id : *ids)
                sets.insert(static_cast<base::EntityId>(id));
        }
        std::set<base::EntityId> loadSets; // the sets of SPCD cards asked for: they are load sets
        if (const auto numbers = run.arguments.ids("subcases"))
        {
            asked = true;
            const model::CaseControl& caseControl = model.control().caseControl;
            const model::CaseLineages lineages(caseControl);
            for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
            {
                if (std::find(numbers->begin(), numbers->end(), subcase->id) == numbers->end())
                    continue;
                if (const auto sid = analysis::effectiveInteger(lineages, *subcase, "SinglePointConstrSelection"))
                    sets.insert(static_cast<base::EntityId>(*sid));
                if (const auto sid = analysis::effectiveInteger(lineages, *subcase, "StaticLoadSelection"))
                    loadSets.insert(static_cast<base::EntityId>(*sid));
            }
        }
        if (!asked)
        {
            analysis::forEachCard<model::SinglePointConstrTable>(
                model, [&](const model::SinglePointConstrCard& card, model::CardLocation) { sets.insert(card.id); });
            analysis::forEachCard<model::SinglePointConstrListTable>(
                model,
                [&](const model::SinglePointConstrListCard& card, model::CardLocation) { sets.insert(card.id); });
            analysis::forEachCard<model::SinglePointConstrUnionTable>(
                model,
                [&](const model::SinglePointConstrUnionCard& card, model::CardLocation) { sets.insert(card.id); });
            // Every load set: an SPCD is listed under its own set and under each LOAD combination that reaches it.
            for (const base::EntityId sid : allLoadSets(model))
                loadSets.insert(sid);
        }
        else if (const auto ids = run.arguments.ids("setIds"))
        {
            for (const std::int64_t id : *ids)
                loadSets.insert(static_cast<base::EntityId>(id)); // a set asked for may be a set of SPCD cards
        }
        // The grid points that exist, for the THRU ranges of SPC1.
        std::vector<std::int32_t> gridIds;
        if (const model::NodeTable* grids = model.findTable<model::NodeTable>())
        {
            for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
            {
                if (grids->rows().isAlive(row))
                    gridIds.push_back(grids->idAtRow(row));
            }
            std::sort(gridIds.begin(), gridIds.end());
        }

        std::vector<Row> rows;
        const auto pushGroup = [&](Row base, std::int32_t node, std::int32_t components, double enforced)
        {
            if (node == 0)
                return;
            base.node = node;
            base.dof = digits(components);
            base.enforced = enforced;
            rows.push_back(std::move(base));
        };
        for (const base::EntityId set : sets)
        {
            const std::vector<base::EntityId> members =
                analysis::expandedConstraintSet(model, model::SINGLE_POINT_CONSTR_SELECTION, set);
            analysis::forEachCard<model::SinglePointConstrTable>(
                model,
                [&](const model::SinglePointConstrCard& card, model::CardLocation)
                {
                    if (!std::binary_search(members.begin(), members.end(), card.id))
                        return;
                    Row base;
                    base.sid = set;
                    base.source = card.id;
                    base.card = "SinglePointConstr";
                    pushGroup(base, card.nodes[0], card.components[0], card.enforced[0]);
                    if (card.hasSecondGroup())
                        pushGroup(base, card.nodes[1], card.components[1], card.enforced[1]);
                });
            analysis::forEachCard<model::SinglePointConstrListTable>(
                model,
                [&](const model::SinglePointConstrListCard& card, model::CardLocation)
                {
                    if (!std::binary_search(members.begin(), members.end(), card.id))
                        return;
                    Row base;
                    base.sid = set;
                    base.source = card.id;
                    base.card = "SinglePointConstrList";
                    for (const model::NodeRun& run1 : card.nodes)
                    {
                        if (run1.first == run1.last)
                            pushGroup(base, run1.first, card.components, NOT_A_NUMBER);
                        else
                        {
                            const auto first = std::lower_bound(gridIds.begin(), gridIds.end(), run1.first);
                            for (auto at = first; at != gridIds.end() && *at <= run1.last; ++at)
                                pushGroup(base, *at, card.components, NOT_A_NUMBER);
                        }
                    }
                });
        }
        // The enforced motions of the load sets, LOAD combinations expanded. in the
        // static solution sequences the LOAD command selects the SPCD set, so a LOAD card combines it as any other load
        // set: an SPCD reached through a LOAD is listed under the set asked for, its value multiplied by the factors of
        // the combination (given in `scale`). Unexpanded (expandCombinations=False), each set of SPCD cards once, its
        // values as given (scale 1).
        const model::EnforcedMotionConstrTable* spcds = model.findTable<model::EnforcedMotionConstrTable>();
        const LoadSetListing listing(model, loadSetCards, run.arguments,
                                     std::vector<base::EntityId>(loadSets.begin(), loadSets.end()));
        for (const base::EntityId set : listing.sets())
        {
            if (spcds == nullptr)
                break;
            for (const analysis::ElementaryLoad& load :
                 listing.loadsOf(set, sortedTypesOf<model::EnforcedMotionConstrTable>()))
            {
                if (load.kind != analysis::LoadKind::EnforcedMotion)
                    continue;
                const model::EnforcedMotionConstrCard card = spcds->card(load.card.row);
                Row base;
                base.sid = set;
                base.source = load.sourceSid;
                base.card = "EnforcedMotionConstr";
                base.scale = load.scale;
                pushGroup(base, card.nodes[0], card.components[0], load.scale * card.enforced[0]);
                if (card.hasSecondGroup())
                    pushGroup(base, card.nodes[1], card.components[1], load.scale * card.enforced[1]);
            }
        }
        if (permanent)
        {
            if (const model::NodeTable* grids = model.findTable<model::NodeTable>())
            {
                for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
                {
                    if (!grids->rows().isAlive(row) || grids->permanentConstraints(row) == 0)
                        continue;
                    Row base;
                    base.sidValid = false;
                    base.card = "gridPs"; // the permanent constraints (PS) of a grid point: no card type of its own
                    pushGroup(base, grids->idAtRow(row), grids->permanentConstraints(row), NOT_A_NUMBER);
                }
            }
        }
        // By set (the permanent constraints last), in the order the cards were found.
        std::stable_sort(rows.begin(), rows.end(),
                         [](const Row& left, const Row& right)
                         {
                             if (left.sidValid != right.sidValid)
                                 return left.sidValid;
                             return left.sid < right.sid;
                         });
        std::vector<std::int32_t> sid;
        std::vector<std::uint8_t> sidValid;
        std::vector<std::int32_t> source;
        std::vector<std::uint8_t> sourceValid;
        std::vector<std::string> card;
        std::vector<std::int32_t> node;
        std::vector<std::string> dof;
        std::vector<double> enforced;
        std::vector<double> scale;
        for (const Row& row : rows)
        {
            sid.push_back(row.sid);
            sidValid.push_back(row.sidValid ? 1 : 0);
            source.push_back(row.source);
            sourceValid.push_back(row.sidValid ? 1 : 0);
            card.emplace_back(row.card);
            node.push_back(row.node);
            dof.push_back(row.dof);
            enforced.push_back(row.enforced);
            scale.push_back(row.scale);
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid), std::move(sidValid));
        builder.int32("sourceSetId", std::move(source), std::move(sourceValid));
        builder.dictionary("card", card);
        builder.int32("nodeId", std::move(node));
        builder.text("dof", dof);
        builder.real("enforced", std::move(enforced));
        builder.real("scale", std::move(scale));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "singlePointConstrTable";
        info.family = "T8";
        info.description =
            "The single point constraints of the sets asked for, SPCADD expanded: setId is the set asked for, "
            "sourceSetId "
            "the set "
            "of the card, card the card type: SinglePointConstr (an SPC), SinglePointConstrList (an SPC1, a THRU range "
            "gives the grid points of the model in it), EnforcedMotionConstr (an SPCD, the enforced "
            "motions of the load sets asked for, LOAD combinations expanded: they count as loads) and gridPs (the PS "
            "field: the "
            "permanent constraints of the "
            "grid "
            "points, `includePermanent`, with a null setId: they apply to every set), nodeId the grid point, dof the "
            "constrained "
            "components as text, enforced the value of the enforced motion (null for SPC1 and gridPs; for an SPCD "
            "the value times scale) and scale the product of the factors of the LOAD cards an SPCD was reached "
            "through (null for the other cards). setIds and "
            "subcases name "
            "the sets (a subcase selects the SPC set and the LOAD set its commands name); with neither, every set. "
            "expandCombinations=False lists the SPCD cards of each load set once, under their own set with the "
            "scale 1, a LOAD combination replaced by the sets it reaches (loadCombinationsTable "
            "gives the factors).";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the constraint sets to list"),
                           parameter("subcases", ParameterKind::IdList, "None", "the sets these subcases select"),
                           parameter("includePermanent", ParameterKind::Bool, "True", "add the PS of the grid points"),
                           expandCombinationsParameter()};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId")
            .int32("sourceSetId")
            .dictionary("card")
            .int32("nodeId")
            .text("dof")
            .real("enforced")
            .real("scale");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addSinglePointConstrTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SinglePointConstrTableQuery>());
}

} // namespace oor::query
