#include "oor/analysis/connections/ContactSearch.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/contactPair/ContactPairTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSet/ContactSetTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSetUnion/ContactSetUnionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSlideline/ContactSlidelineTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactTable/ContactTableTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactTable1/ContactTable1Table.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <tuple>
#include <utility>

namespace oor::query
{

namespace
{

// One contact table of the Bulk Data and the pair cards it is made of.
struct ContactTableEntry
{
    std::string card; // the card type
    base::EntityId id = 0;
    bool isSelectable = true;                                      // a ContactSlideline is not selected by BCONTACT
    std::vector<std::pair<std::string, base::EntityId>> pairCards; // ("ContactPair", 3), ...
};

// The tables of the Bulk Data in the order of the card names, then of their IDs.
std::vector<ContactTableEntry> tablesOf(const model::Model& model)
{
    std::vector<ContactTableEntry> tables;
    analysis::forEachCard<model::ContactTableTable>(
        model, [&](const model::ContactTableCard& card, model::CardLocation)
        { tables.push_back({"ContactTable", card.id, true, {{"ContactTable", card.id}}}); });
    bool anyBctabl1 = false;
    analysis::forEachCard<model::ContactTable1Table>(model,
                                                     [&](const model::ContactTable1Card& card, model::CardLocation)
                                                     {
                                                         anyBctabl1 = true;
                                                         ContactTableEntry entry{"ContactTable1", card.id, true, {}};
                                                         for (const model::ContactRun& run : card.pairs)
                                                         {
                                                             for (base::EntityId id = run.first; id <= run.last; ++id)
                                                                 entry.pairCards.emplace_back("ContactPair", id);
                                                         }
                                                         tables.push_back(std::move(entry));
                                                     });
    // BCONTACT selects a BCONECT directly when the deck has no BCTABL1.
    if (!anyBctabl1)
    {
        analysis::forEachCard<model::ContactPairTable>(
            model, [&](const model::ContactPairCard& card, model::CardLocation)
            { tables.push_back({"ContactPair", card.id, true, {{"ContactPair", card.id}}}); });
    }
    analysis::forEachCard<model::ContactSetTable>(
        model, [&](const model::ContactSetCard& card, model::CardLocation)
        { tables.push_back({"ContactSet", card.id, true, {{"ContactSet", card.id}}}); });
    analysis::forEachCard<model::ContactSetUnionTable>(model,
                                                       [&](const model::ContactSetUnionCard& card, model::CardLocation)
                                                       {
                                                           ContactTableEntry entry{
                                                               "ContactSetUnion", card.id, true, {}};
                                                           for (const base::EntityId set : card.sets)
                                                               entry.pairCards.emplace_back("ContactSet", set);
                                                           tables.push_back(std::move(entry));
                                                       });
    analysis::forEachCard<model::ContactSlidelineTable>(
        model, [&](const model::ContactSlidelineCard& card, model::CardLocation)
        { tables.push_back({"ContactSlideline", card.id, false, {{"ContactSlideline", card.id}}}); });
    std::stable_sort(tables.begin(), tables.end(), [](const ContactTableEntry& left, const ContactTableEntry& right)
                     { return std::tie(left.card, left.id) < std::tie(right.card, right.id); });
    return tables;
}

// contactTablesTable: the contact tables of the Bulk Data, their pairs and the subcases that select them.
class ContactTablesTableQuery final : public QueryBase
{
public:
    ContactTablesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::vector<ContactTableEntry> tables = tablesOf(model);
        if (tables.empty())
            return emptyTable(info().defaultColumns());
        // The subcases that select each ID (format reference BCONTACT: the effective integer of the subcase).
        std::map<base::EntityId, std::vector<std::int32_t>> selectors;
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
        {
            const model::CaseEntry* entry = lineages.command(subcase->key, "ContactSelection");
            if (entry != nullptr && entry->value.kind == model::CaseValueKind::Integer)
                selectors[static_cast<base::EntityId>(entry->value.integer)].push_back(
                    static_cast<std::int32_t>(subcase->id));
        }
        // The pairs of the search, by their card.
        const auto search = analysis::ContactSearch::of(model, std::nullopt);
        std::map<std::pair<std::string, base::EntityId>, std::vector<const analysis::ContactPair*>> pairsOfCard;
        for (const analysis::ContactPair& pair : search->pairs())
        {
            if (pair.table)
                pairsOfCard[{pair.tableName, pair.tableId}].push_back(&pair);
        }

        std::vector<std::string> table;
        std::vector<std::string> tableCard;
        std::vector<std::int32_t> tableId;
        std::vector<std::string> pairCard;
        std::vector<std::uint8_t> pairCardValid;
        std::vector<std::int32_t> secondary;
        std::vector<std::uint8_t> secondaryValid;
        std::vector<std::int32_t> primary;
        std::vector<std::uint8_t> primaryValid;
        std::vector<std::int64_t> subcaseOffsets{0};
        std::vector<std::int32_t> subcases;
        for (const ContactTableEntry& entry : tables)
        {
            const auto selected = selectors.find(entry.id);
            const auto addRow = [&](const std::string& card, const analysis::ContactPair* pair)
            {
                table.push_back(entry.card + " " + std::to_string(entry.id));
                tableCard.push_back(entry.card);
                tableId.push_back(static_cast<std::int32_t>(entry.id));
                pairCard.push_back(card);
                pairCardValid.push_back(card.empty() ? 0 : 1);
                secondary.push_back(pair != nullptr ? pair->secondary->id : 0);
                secondaryValid.push_back(pair != nullptr ? 1 : 0);
                primary.push_back(pair != nullptr ? pair->primary->id : 0);
                primaryValid.push_back(pair != nullptr ? 1 : 0);
                if (entry.isSelectable && selected != selectors.end())
                    subcases.insert(subcases.end(), selected->second.begin(), selected->second.end());
                subcaseOffsets.push_back(static_cast<std::int64_t>(subcases.size()));
            };
            bool anyRow = false;
            for (const auto& [name, id] : entry.pairCards)
            {
                const auto found = pairsOfCard.find({name, id});
                if (found == pairsOfCard.end())
                    continue;
                for (const analysis::ContactPair* pair : found->second)
                {
                    addRow(name + " " + std::to_string(id), pair);
                    anyRow = true;
                }
            }
            if (!anyRow)
                addRow("", nullptr); // a table with no pair of deformable bodies still has its subcases
        }
        TableBuilder builder;
        builder.text("table", table);
        builder.dictionary("tableCard", tableCard);
        builder.int32("tableId", std::move(tableId));
        builder.text("pairCard", pairCard, std::move(pairCardValid));
        builder.int32("secondaryBody", std::move(secondary), std::move(secondaryValid));
        builder.int32("primaryBody", std::move(primary), std::move(primaryValid));
        builder.intList("subcases", std::move(subcaseOffsets), std::move(subcases));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "contactTablesTable";
        info.family = "T6";
        info.description =
            "The contact tables of the Bulk Data, for information (glued contact; the "
            "components use every table whatever the Case Control selects): one row per table and pair of contact "
            "bodies (BCTABLE; BCTABL1 expanded to the pairs of its BCONECT, pairCard the BCONECT; a BCONECT alone "
            "when the deck has no BCTABL1; BCTSET; BCTADD expanded to its BCTSET; "
            "BCONP), and "
            "subcases, the analysis subcases whose effective BCONTACT names the table's ID (none for a BCONP, which "
            "BCONTACT does not select). A pair with a rigid body is left out as in contactPairsTable; a table with "
            "no pair left has one row with null bodies.";
        info.defaultSort = "tableCard, tableId, then the pairs";
        ColumnListBuilder columns;
        columns.text("table").dictionary("tableCard").int32("tableId").text("pairCard").int32("secondaryBody");
        columns.int32("primaryBody").intList("subcases");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addContactTablesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ContactTablesTableQuery>());
}

} // namespace oor::query
