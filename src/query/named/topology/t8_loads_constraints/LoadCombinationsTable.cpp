#include "oor/analysis/components/LoadSets.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/loads/loadCombination/LoadCombinationTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace oor::query
{

namespace
{

// The LOAD cards of a model by their set ID (the first of a set ID given twice).
std::map<base::EntityId, model::LoadCombinationCard> loadCards(const model::Model& model)
{
    std::map<base::EntityId, model::LoadCombinationCard> cards;
    const auto* table = static_cast<const model::LoadCombinationTable*>(
        model.findTable(model::typeIndexOf<model::LoadCombinationTable>()));
    if (table == nullptr)
        return cards;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (table->rows().isAlive(row))
        {
            model::LoadCombinationCard card = table->card(row);
            cards.emplace(card.id, std::move(card));
        }
    }
    return cards;
}

// One row of the table: a set a LOAD entry reaches, with the sum of its factors over the ways it is reached.
struct Reached
{
    base::EntityId sid = 0;
    double factor = 0.0;
};

// The sets a LOAD entry reaches (analysis::LoadSets::reachedSets), each once, sorted: a set reached twice has the sum
// of its factors, added from 0 in the order of the terms (as this table always did).
std::vector<Reached> reachedOnce(const model::Model& model, const analysis::LoadSetIndex& index, base::EntityId sid)
{
    std::vector<Reached> sets;
    for (const analysis::ReachedSet& reached : analysis::LoadSets::reachedSets(model, index, sid))
    {
        auto found = std::find_if(sets.begin(), sets.end(), [&](const Reached& set) { return set.sid == reached.sid; });
        if (found == sets.end())
        {
            sets.push_back(Reached{reached.sid, 0.0});
            found = sets.end() - 1;
        }
        found->factor += reached.factor;
    }
    std::sort(sets.begin(), sets.end(), [](const Reached& left, const Reached& right) { return left.sid < right.sid; });
    return sets;
}

// loadCombinationsTable: every LOAD entry with the load sets it reaches and the subcases that load each set alone.
class LoadCombinationsTableQuery final : public QueryBase
{
public:
    LoadCombinationsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // the cards of every load set, read once for all the sets
        const analysis::LoadSetIndex loadSets(model);
        const std::map<base::EntityId, model::LoadCombinationCard> cards = loadCards(model);
        const auto asked = run.arguments.ids("loads");

        // The analysis subcases by the set their effective LOAD selects.
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        std::map<std::int64_t, std::vector<std::int32_t>> subcasesOfSet;
        for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
        {
            const model::CaseEntry* entry = lineages.command(subcase->key, "StaticLoadSelection");
            if (entry != nullptr && entry->value.kind == model::CaseValueKind::Integer)
                subcasesOfSet[entry->value.integer].push_back(static_cast<std::int32_t>(subcase->id));
        }

        std::vector<std::int32_t> load;
        std::vector<double> scale;
        std::vector<std::int32_t> loadSet;
        std::vector<double> factor;
        std::vector<double> combined;
        std::vector<std::vector<std::int32_t>> units;
        std::vector<std::int64_t> cardOffsets{0};
        std::vector<std::string> cardNames;
        std::map<base::EntityId, std::vector<std::string>> cardsOfSet;
        for (const auto& [sid, card] : cards)
        {
            if (asked && std::find(asked->begin(), asked->end(), sid) == asked->end())
                continue;
            run.context.checkCancelled();
            for (const Reached& set : reachedOnce(model, loadSets, sid))
            {
                load.push_back(sid);
                scale.push_back(card.scale);
                loadSet.push_back(set.sid);
                factor.push_back(set.factor);
                combined.push_back(card.scale * set.factor);
                const auto found = subcasesOfSet.find(set.sid);
                units.push_back(found != subcasesOfSet.end() ? found->second : std::vector<std::int32_t>());
                auto [names, inserted] = cardsOfSet.try_emplace(set.sid);
                if (inserted)
                {
                    // The types of the set's own cards that are loads (those LoadSets::ownLoads gives), read from
                    // the index without reading the cards (in the order of the types: a type's cards together).
                    std::set<std::string> distinct;
                    std::uint16_t previous = 0;
                    for (const model::CardLocation location : loadSets.cardsOf(set.sid))
                    {
                        if (location.table == previous ||
                            location.table == model::typeIndexOf<model::LoadCombinationTable>())
                            continue;
                        previous = location.table;
                        if (analysis::LoadSets::kindOfType(location.table) != analysis::LoadKind::Other)
                            distinct.insert(std::string(typeName(location.table)));
                    }
                    names->second.assign(distinct.begin(), distinct.end());
                }
                cardNames.insert(cardNames.end(), names->second.begin(), names->second.end());
                cardOffsets.push_back(static_cast<std::int64_t>(cardNames.size()));
            }
        }
        if (load.empty())
            return emptyTable(info().defaultColumns());
        TableBuilder builder;
        builder.int32("load", std::move(load));
        builder.real("scale", std::move(scale));
        builder.int32("loadSet", std::move(loadSet));
        builder.real("factor", std::move(factor));
        builder.real("combinedFactor", std::move(combined));
        builder.intLists("unitSubcases", units);
        builder.textList("setCards", std::move(cardOffsets), cardNames);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "loadCombinationsTable";
        info.family = "T8";
        info.description =
            "Every LOAD entry with the load sets it reaches (the factors of a combination on its load sets, and the "
            "terms of the load tables asked with expandCombinations=False): one row per LOAD and "
            "set. format reference LOAD: the load is S times the sum of Si times the set Li; a Li that is itself a "
            "LOAD entry is "
            "expanded (nested LOAD entries are permitted), so loadSet is never a combination. scale is the "
            "S "
            "of the entry, factor the product of the factors on the way (Si, and the S and Sj of a nested LOAD), "
            "combinedFactor scale times factor (the sum when a set is reached twice): for a LOAD of sets, the `scale` "
            "of its expanded rows to the bit (through a nested LOAD the products are taken in another order, a "
            "rounding apart). unitSubcases are the analysis subcases whose effective LOAD selects the set (what a "
            "superposition of unit subcases reads), setCards the card types of the set. A cycle of LOAD entries (the "
            "validation reports it, load.combination-cycle) stops at the entry that closes it. loads keeps these LOAD "
            "entries.";
        info.parameters = {parameter("loads", ParameterKind::IdList, "None", "keep these LOAD entries (their SID)")};
        info.defaultSort = "load, loadSet";
        ColumnListBuilder columns;
        columns.int32("load").real("scale").int32("loadSet").real("factor").real("combinedFactor");
        columns.intList("unitSubcases").textList("setCards");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addLoadCombinationsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<LoadCombinationsTableQuery>());
}

} // namespace oor::query
