#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/constraints/freeBodySupport/FreeBodySupportTable.hpp"
#include "oor/model/cards/constraints/freeBodySupportSet/FreeBodySupportSetTable.hpp"
#include "oor/model/cards/sets/analysisDofSet/AnalysisDofSetTable.hpp"
#include "oor/model/cards/sets/analysisDofSet1/AnalysisDofSet1Table.hpp"
#include "oor/model/cards/sets/fixedBoundaryDofSet/FixedBoundaryDofSetTable.hpp"
#include "oor/model/cards/sets/fixedBoundaryDofSet1/FixedBoundaryDofSet1Table.hpp"
#include "oor/model/cards/sets/freeBoundaryDofSet/FreeBoundaryDofSetTable.hpp"
#include "oor/model/cards/sets/freeBoundaryDofSet1/FreeBoundaryDofSet1Table.hpp"
#include "oor/model/cards/sets/generalizedDofSet/GeneralizedDofSetTable.hpp"
#include "oor/model/cards/sets/generalizedDofSet1/GeneralizedDofSet1Table.hpp"
#include "oor/model/cards/sets/omittedDofSet/OmittedDofSetTable.hpp"
#include "oor/model/cards/sets/omittedDofSet1/OmittedDofSet1Table.hpp"
#include "oor/model/cards/sets/userDofSet/UserDofSetTable.hpp"
#include "oor/model/cards/sets/userDofSet1/UserDofSet1Table.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"
#include "oor/query/providers/ModelSource.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

// One row of dofSetsTable: a point of a degree-of-freedom set card and its components.
struct DofRow
{
    std::string card;
    std::optional<std::int32_t> sid;    // SUPORT1
    std::optional<std::string> setName; // USET, USET1
    std::int32_t node = 0;
    std::int32_t components = 0; // the digits, 0 for a scalar point
};

// Gathers the rows of the cards of one kind.
class DofRowCollector
{
public:
    DofRowCollector(const model::Model& model, std::vector<DofRow>& rows) : m_model(model), m_rows(rows)
    {
        std::vector<std::uint16_t> types = typesOfCategory(model::CardCategory::Node);
        const std::vector<std::uint16_t> points = typesOfCategory(model::CardCategory::Point);
        types.insert(types.end(), points.begin(), points.end());
        m_pointIds = listCards(model, types).ids;
        std::sort(m_pointIds.begin(), m_pointIds.end());
    }

    // The cards that list (point, components) pairs: ASET, BSET, CSET, QSET, OMIT, SUPORT, SUPORT1, USET.
    template <typename TableT>
    void pairs(const char* card)
    {
        analysis::forEachCard<TableT>(m_model,
                                      [&](const typename TableT::CardType& value, model::CardLocation)
                                      {
                                          for (const model::DofPair& pair : value.dofs)
                                              add(card, value, pair.node, pair.components);
                                      });
    }

    // The cards that give one set of components for a list of points with THRU: ASET1, BSET1, CSET1, QSET1, OMIT1,
    // USET1. A THRU range gives the grid and scalar points of the model that lie in it.
    template <typename TableT>
    void lists(const char* card)
    {
        analysis::forEachCard<TableT>(m_model,
                                      [&](const typename TableT::CardType& value, model::CardLocation)
                                      {
                                          for (const model::IdRun& run : value.nodes)
                                          {
                                              if (run.first == run.last)
                                              {
                                                  add(card, value, run.first, value.components);
                                                  continue;
                                              }
                                              const auto first =
                                                  std::lower_bound(m_pointIds.begin(), m_pointIds.end(), run.first);
                                              for (auto at = first; at != m_pointIds.end() && *at <= run.last; ++at)
                                                  add(card, value, *at, value.components);
                                          }
                                      });
    }

private:
    template <typename CardT>
    void add(const char* card, const CardT& value, std::int32_t node, std::int32_t components)
    {
        DofRow row;
        row.card = card;
        row.node = node;
        row.components = components;
        if constexpr (std::is_same_v<CardT, model::FreeBodySupportSetCard>)
            row.sid = value.id;
        if constexpr (std::is_same_v<CardT, model::UserDofSetCard> || std::is_same_v<CardT, model::UserDofSet1Card>)
            row.setName = value.setName;
        m_rows.push_back(std::move(row));
    }

    const model::Model& m_model;
    std::vector<DofRow>& m_rows;
    std::vector<std::int32_t> m_pointIds;
};

// dofSetsTable: the points and components of the degree-of-freedom set cards.
class DofSetsTableQuery final : public QueryBase
{
public:
    DofSetsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // the card types asked for, given by their type or their keyword (compared as card types)
        std::set<std::string> onlyCards;
        for (const std::string& name : run.arguments.texts("cards"))
            onlyCards.emplace(name);
        std::vector<DofRow> rows;
        DofRowCollector collect(model, rows);
        collect.pairs<model::AnalysisDofSetTable>("AnalysisDofSet");
        collect.lists<model::AnalysisDofSet1Table>("AnalysisDofSet1");
        collect.pairs<model::FixedBoundaryDofSetTable>("FixedBoundaryDofSet");
        collect.lists<model::FixedBoundaryDofSet1Table>("FixedBoundaryDofSet1");
        collect.pairs<model::FreeBoundaryDofSetTable>("FreeBoundaryDofSet");
        collect.lists<model::FreeBoundaryDofSet1Table>("FreeBoundaryDofSet1");
        collect.pairs<model::GeneralizedDofSetTable>("GeneralizedDofSet");
        collect.lists<model::GeneralizedDofSet1Table>("GeneralizedDofSet1");
        collect.pairs<model::OmittedDofSetTable>("OmittedDofSet");
        collect.lists<model::OmittedDofSet1Table>("OmittedDofSet1");
        collect.pairs<model::FreeBodySupportTable>("FreeBodySupport");
        collect.pairs<model::FreeBodySupportSetTable>("FreeBodySupportSet");
        collect.pairs<model::UserDofSetTable>("UserDofSet");
        collect.lists<model::UserDofSet1Table>("UserDofSet1");
        if (!onlyCards.empty())
            std::erase_if(rows, [&](const DofRow& row) { return !onlyCards.contains(std::string(row.card)); });

        // The subcases whose SUPORT1 command selects each set (the Case Control
        // SUPORT1 requests the SUPORT1 cards of a set, whose components join those of the SUPORT cards).
        std::map<std::int32_t, std::vector<std::int32_t>> subcasesOfSet;
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
        {
            if (const auto sid = analysis::effectiveInteger(lineages, *subcase, "FreeBodySupportSelection"))
                subcasesOfSet[static_cast<std::int32_t>(*sid)].push_back(static_cast<std::int32_t>(subcase->id));
        }
        std::vector<std::string> card;
        std::vector<std::int32_t> sid;
        std::vector<std::uint8_t> sidValid;
        std::vector<std::string> setName;
        std::vector<std::uint8_t> setNameValid;
        std::vector<std::int32_t> node;
        std::vector<std::string> dof;
        std::vector<std::int64_t> subcaseOffsets{0};
        std::vector<std::int32_t> subcases;
        std::vector<std::uint8_t> subcasesValid;
        for (const DofRow& row : rows)
        {
            card.push_back(row.card);
            sid.push_back(row.sid.value_or(0));
            sidValid.push_back(row.sid ? 1 : 0);
            setName.push_back(row.setName.value_or(std::string()));
            setNameValid.push_back(row.setName ? 1 : 0);
            node.push_back(row.node);
            dof.push_back(row.components != 0 ? std::to_string(row.components) : std::string("0"));
            if (row.sid)
            {
                if (const auto found = subcasesOfSet.find(*row.sid); found != subcasesOfSet.end())
                    subcases.insert(subcases.end(), found->second.begin(), found->second.end());
            }
            subcaseOffsets.push_back(static_cast<std::int64_t>(subcases.size()));
            subcasesValid.push_back(row.sid ? 1 : 0);
        }
        TableBuilder builder;
        builder.dictionary("card", card);
        builder.int32("setId", std::move(sid), std::move(sidValid));
        builder.dictionary("setName", setName, std::move(setNameValid));
        builder.int32("nodeId", std::move(node));
        builder.text("dof", dof);
        builder.intList("subcases", std::move(subcaseOffsets), std::move(subcases), std::move(subcasesValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "dofSetsTable";
        info.family = "T8";
        info.description =
            "The points and components of the degree-of-freedom set cards, one row for each (card, point): ASET, "
            "BSET, CSET, QSET, OMIT, SUPORT and USET give their (point, components) pairs, ASET1, BSET1, CSET1, "
            "QSET1, OMIT1 and USET1 their components for each point of the list (a THRU range gives the grid and "
            "scalar points of the model in it), SUPORT1 its pairs with its set ID in setId. dof is the components as "
            "text (0 for a scalar point); setName the SNAME of a USET or USET1 (null for the others); subcases, for "
            "a SUPORT1, the subcases whose Case Control SUPORT1 command selects its set (null for the other cards: "
            "SUPORT applies to every subcase). cards keeps these card types (by type, FreeBodySupport, or keyword, "
            "SUPORT). In the order of the card types above, "
            "then of the cards.";
        info.parameters = {parameter("cards", ParameterKind::TextList, "None", "keep these card types")};
        info.defaultSort = "card (in the order of the description), then the order of the cards";
        ColumnListBuilder columns;
        columns.dictionary("card").int32("setId").dictionary("setName").int32("nodeId").text("dof").intList("subcases");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDofSetsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DofSetsTableQuery>());
}

} // namespace oor::query
