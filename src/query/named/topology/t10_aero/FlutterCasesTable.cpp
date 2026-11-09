#include "AeroQuerySupport.hpp"
#include "oor/model/cards/aero/aeroFlutter/AeroFlutterTable.hpp"
#include "oor/model/cards/aero/aeroFlutterFactors/AeroFlutterFactorsTable.hpp"
#include "oor/model/cards/aero/aeroMachFreqPairs2/AeroMachFreqPairs2Table.hpp"
#include "oor/model/cards/aero/aeroMachFreqTable1/AeroMachFreqTable1Table.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <optional>

namespace oor::query
{

namespace
{

// The numbers of the FLFACT card with this SID (the THRU form expanded); an empty list when no FLFACT has it.
std::vector<double> factorsOf(const model::Model& model, base::EntityId sid)
{
    const auto* table = model.findTable<model::AeroFlutterFactorsTable>();
    if (table == nullptr)
        return {};
    const std::optional<model::CardLocation> found =
        model.locateInType(model::typeIndexOf<model::AeroFlutterFactorsTable>(), sid);
    return found ? model::aeroFlutterFactorsValues(table->card(found->row)) : std::vector<double>();
}

// aeroFlutterCasesTable: the FLUTTER cards with the numbers their FLFACT lists hold (detail "cases"), or the pairs of
// Mach number and reduced frequency of the MKAERO1 and MKAERO2 cards (detail "machFrequency").
class FlutterCasesTableQuery final : public QueryBase
{
public:
    FlutterCasesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "cases");
        if (detail != "cases" && detail != "machFrequency")
            throw QueryError("detail is \"cases\" or \"machFrequency\", not '" + detail + "'");
        return detail == "cases" ? cases(run) : pairs(*run.model);
    }

private:
    [[nodiscard]] std::shared_ptr<const QueryTable> cases(const QueryRun& run) const
    {
        const model::Model& model = *run.model;
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("setIds"));
        std::vector<model::AeroFlutterCard> cards = aero::cardsOf<model::AeroFlutterTable>(model);
        std::ranges::stable_sort(cards, {}, &model::AeroFlutterCard::id);
        if (wanted)
        {
            std::erase_if(cards, [&](const model::AeroFlutterCard& card)
                          { return !std::binary_search(wanted->begin(), wanted->end(), std::int64_t{card.id}); });
        }
        if (cards.empty())
            return emptyTable(info().columnsFor("cases"));

        const std::map<base::EntityId, std::vector<std::int32_t>> subcasesOf =
            aero::subcasesSelecting(model, "FlutterMethodSelection");
        const std::size_t count = cards.size();
        std::vector<std::int32_t> sid(count), rootCount(count);
        std::vector<std::uint8_t> rootCountValid(count);
        std::vector<std::string> method(count), interpolation(count);
        std::vector<double> epsilon(count);
        std::vector<std::int64_t> subcaseOffsets{0};
        std::vector<std::int32_t> subcases;
        std::array<std::vector<std::int64_t>, 3> listOffsets{std::vector<std::int64_t>{0}, std::vector<std::int64_t>{0},
                                                             std::vector<std::int64_t>{0}};
        std::array<std::vector<double>, 3> listValues;
        for (std::size_t row = 0; row < count; ++row)
        {
            const model::AeroFlutterCard& card = cards[row];
            sid[row] = card.id;
            method[row] = model::FLUTTER_METHOD_WORDS[static_cast<std::size_t>(card.method)];
            interpolation[row] = model::FLUTTER_INTERPOLATION_WORDS[static_cast<std::size_t>(card.interpolationMethod)];
            // field 7 is the largest frequency (OMAX) of the sweep methods, the number of roots (NVALUE) of the others;
            // a blank NVALUE is the default the analysis computes
            rootCount[row] = card.eigenvalueCount;
            rootCountValid[row] = !model::isSweepMethod(card.method) && card.eigenvalueCount > 0 ? 1 : 0;
            epsilon[row] = card.epsilon;
            if (const auto found = subcasesOf.find(card.id); found != subcasesOf.end())
                subcases.insert(subcases.end(), found->second.begin(), found->second.end());
            subcaseOffsets.push_back(static_cast<std::int64_t>(subcases.size()));
            // the densities, the Mach numbers, then the reduced frequencies or the velocities
            const std::array<base::EntityId, 3> lists{card.densityFactorsId, card.machFactorsId,
                                                      card.reducedFreqFactorsId};
            for (std::size_t list = 0; list < lists.size(); ++list)
            {
                const std::vector<double> factors = factorsOf(model, lists[list]);
                listValues[list].insert(listValues[list].end(), factors.begin(), factors.end());
                listOffsets[list].push_back(static_cast<std::int64_t>(listValues[list].size()));
            }
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.intList("subcases", std::move(subcaseOffsets), std::move(subcases));
        builder.dictionary("method", method);
        builder.realList("densityRatios", std::move(listOffsets[0]), std::move(listValues[0]));
        builder.realList("machs", std::move(listOffsets[1]), std::move(listValues[1]));
        builder.realList("velocitiesOrFrequencies", std::move(listOffsets[2]), std::move(listValues[2]));
        builder.dictionary("interpolation", interpolation);
        builder.int32("rootCount", std::move(rootCount), std::move(rootCountValid));
        builder.real("epsilon", std::move(epsilon));
        return builder.finish();
    }

    // The pairs the aerodynamic matrices are computed at: an MKAERO1 card gives every Mach number with every reduced
    // frequency of the same card (the Mach numbers in the outer loop), an MKAERO2 card gives the pairs themselves.
    [[nodiscard]] std::shared_ptr<const QueryTable> pairs(const model::Model& model) const
    {
        std::vector<double> mach, frequency;
        for (const model::AeroMachFreqTable1Card& card : aero::cardsOf<model::AeroMachFreqTable1Table>(model))
        {
            for (const double number : card.machs)
            {
                for (const double reduced : card.frequencies)
                {
                    mach.push_back(number);
                    frequency.push_back(reduced);
                }
            }
        }
        for (const model::AeroMachFreqPairs2Card& card : aero::cardsOf<model::AeroMachFreqPairs2Table>(model))
        {
            for (const model::AeroMachFreqPairs2Pair& pair : card.pairs)
            {
                mach.push_back(pair.mach);
                frequency.push_back(pair.frequency);
            }
        }
        if (mach.empty())
            return emptyTable(info().columnsFor("machFrequency"));
        TableBuilder builder;
        builder.real("mach", std::move(mach));
        builder.real("reducedFrequency", std::move(frequency));
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroFlutterCasesTable";
        info.family = "T10";
        info.description =
            "`detail=\"cases\"` (the default): the FLUTTER cards, one row for each. `subcases` are the subcases whose "
            "Case Control FMETHOD selects the card. The three lists are the numbers of the FLFACT cards it names (the "
            "THRU form expanded by the formula of the format reference; empty when the FLFACT is missing): the density "
            "ratios, the "
            "Mach numbers, and the reduced frequencies (K, KE) or the velocities (the PK methods). `rootCount` is "
            "NVALUE, null when blank and for the sweep methods (PKS, PKNLS), whose field 7 is OMAX. "
            "`detail=\"machFrequency\"`: the pairs of Mach number and reduced frequency of the MKAERO1 cards (every "
            "Mach number of a card with every frequency of the same card, the Mach numbers in the outer loop) and then "
            "of the MKAERO2 cards, in the order of the cards; it is not about one case, and `setIds` does not apply.";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the FLUTTER sets to list"),
                           parameter("detail", ParameterKind::Text, "\"cases\"", "\"cases\" or \"machFrequency\"")};
        info.detailParameter = "detail";
        info.defaultSort = "setId; for machFrequency, the order of the cards (MKAERO1, then MKAERO2)";
        ColumnListBuilder caseColumns;
        caseColumns.int32("setId").intList("subcases").dictionary("method").realList("densityRatios").realList("machs");
        caseColumns.realList("velocitiesOrFrequencies").dictionary("interpolation").int32("rootCount");
        caseColumns.real("epsilon");
        info.columnSets.push_back(caseColumns.build("cases"));
        ColumnListBuilder pairColumns;
        pairColumns.real("mach").real("reducedFrequency");
        info.columnSets.push_back(pairColumns.build("machFrequency"));
        return info;
    }
};

} // namespace

void addFlutterCasesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FlutterCasesTableQuery>());
}

} // namespace oor::query
