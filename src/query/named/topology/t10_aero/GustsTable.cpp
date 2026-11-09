#include "AeroQuerySupport.hpp"
#include "oor/model/cards/aero/aeroGust/AeroGustTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <algorithm>
#include <optional>

namespace oor::query
{

namespace
{

// gustsTable: the GUST cards.
class GustsTableQuery final : public QueryBase
{
public:
    GustsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("setIds"));
        std::vector<model::AeroGustCard> cards = aero::cardsOf<model::AeroGustTable>(*run.model);
        std::ranges::stable_sort(cards, {}, &model::AeroGustCard::id);
        if (wanted)
        {
            std::erase_if(cards, [&](const model::AeroGustCard& card)
                          { return !std::binary_search(wanted->begin(), wanted->end(), std::int64_t{card.id}); });
        }
        if (cards.empty())
            return emptyTable(info().defaultColumns());
        std::vector<std::int32_t> sid, dload;
        std::vector<double> scale, x0, velocity;
        for (const model::AeroGustCard& card : cards)
        {
            sid.push_back(card.id);
            dload.push_back(card.dynamicLoadId);
            scale.push_back(card.gustScale);
            x0.push_back(card.gustOriginX);
            velocity.push_back(card.velocity);
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.int32("dynamicLoadId", std::move(dload));
        builder.real("scale", std::move(scale));
        builder.real("x0", std::move(x0));
        builder.real("velocity", std::move(velocity));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "gustsTable";
        info.family = "T10";
        info.description =
            "The GUST cards, one row for each. `dynamicLoadId` is the set of the TLOADi or RLOADi that gives the time "
            "or "
            "frequency dependence, `scale` is WG (the gust velocity over the forward velocity), `x0` the position at "
            "which the gust starts and `velocity` the V of the card, which must equal the VELOCITY of the AERO card "
            "(the check `aero.gust-velocity` says when it does not).";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the GUST sets to list")};
        info.defaultSort = "setId";
        ColumnListBuilder columns;
        columns.int32("setId").int32("dynamicLoadId").real("scale").real("x0").real("velocity");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addGustsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<GustsTableQuery>());
}

} // namespace oor::query
