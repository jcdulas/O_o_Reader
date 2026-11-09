#include "AeroQuerySupport.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <algorithm>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The boxes a control surface moves: the items of its one or two AELIST cards. `complete` is false when a list the card
// names does not exist, so that the count is not given as if the surface had fewer boxes.
struct SurfaceBoxes
{
    aero::IdRuns items;
    bool complete = true;
};

SurfaceBoxes boxesOf(const model::Model& model, const model::AeroControlSurfaceCard& card)
{
    SurfaceBoxes boxes;
    for (const base::EntityId list : {card.boxListId1, card.boxListId2})
    {
        if (list == 0)
            continue;
        if (const std::optional<aero::IdRuns> items = aero::aelistItems(model, list))
            boxes.items = boxes.items.united(*items);
        else
            boxes.complete = false;
    }
    return boxes;
}

// controlSurfacesTable: the AESURF cards with the boxes of their AELIST cards.
class ControlSurfacesTableQuery final : public QueryBase
{
public:
    ControlSurfacesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::vector<model::AeroControlSurfaceCard> cards = aero::cardsOf<model::AeroControlSurfaceTable>(model);
        if (cards.empty())
            return emptyTable(info().defaultColumns());
        std::ranges::stable_sort(cards, {}, &model::AeroControlSurfaceCard::id);

        const std::size_t count = cards.size();
        std::vector<std::int32_t> id(count), cid1(count), alid1(count), cid2(count), alid2(count);
        std::vector<std::uint8_t> secondValid(count), linear(count);
        std::vector<std::string> label(count);
        std::vector<double> effectiveness(count), chord(count), area(count), lowerPosition(count), upperPosition(count),
            lowerMoment(count), upperMoment(count);
        std::vector<std::int64_t> boxCount(count), boxOffsets{0};
        std::vector<std::uint8_t> boxCountValid(count);
        std::vector<std::int32_t> boxes;
        for (std::size_t row = 0; row < count; ++row)
        {
            const model::AeroControlSurfaceCard& card = cards[row];
            id[row] = card.id;
            label[row] = card.label;
            cid1[row] = card.frameId1;
            alid1[row] = card.boxListId1;
            cid2[row] = card.frameId2;
            alid2[row] = card.boxListId2;
            secondValid[row] = card.boxListId2 != 0 ? 1 : 0;
            effectiveness[row] = card.effectiveness;
            linear[row] = card.linearDownwash == model::AeroControlSurfaceDownwash::Computed ? 1 : 0;
            chord[row] = card.referenceChord;
            area[row] = card.referenceArea;
            // a limit that a table gives (TQLLIM, TQULIM) is a function of the dynamic pressure, not a number
            lowerPosition[row] = card.lowerLimitTableId != 0 ? NOT_A_NUMBER : card.deflectionLowerLimit;
            upperPosition[row] = card.upperLimitTableId != 0 ? NOT_A_NUMBER : card.deflectionUpperLimit;
            lowerMoment[row] = card.hingeMomentLowerLimit.value_or(NOT_A_NUMBER);
            upperMoment[row] = card.hingeMomentUpperLimit.value_or(NOT_A_NUMBER);
            const SurfaceBoxes surfaceBoxes = boxesOf(model, card);
            const std::vector<base::EntityId> ids = surfaceBoxes.items.expand("the AESURF " + std::to_string(card.id));
            boxes.insert(boxes.end(), ids.begin(), ids.end());
            boxOffsets.push_back(static_cast<std::int64_t>(boxes.size()));
            boxCount[row] = surfaceBoxes.items.count();
            boxCountValid[row] = surfaceBoxes.complete ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("id", std::move(id));
        builder.text("label", label);
        builder.int32("frameId1", std::move(cid1));
        builder.int32("aeroListId1", std::move(alid1));
        builder.int32("frameId2", std::move(cid2), secondValid);
        builder.int32("aeroListId2", std::move(alid2), secondValid);
        builder.real("effectiveness", std::move(effectiveness));
        builder.flag("isLinear", linear);
        builder.real("refChord", std::move(chord));
        builder.real("refArea", std::move(area));
        builder.real("positionLower", std::move(lowerPosition));
        builder.real("positionUpper", std::move(upperPosition));
        builder.real("hingeMomentLower", std::move(lowerMoment));
        builder.real("hingeMomentUpper", std::move(upperMoment));
        builder.int64("boxCount", std::move(boxCount), std::move(boxCountValid));
        builder.intList("boxes", std::move(boxOffsets), std::move(boxes));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "controlSurfacesTable";
        info.family = "T10";
        info.description =
            "The control surfaces (AESURF cards), one row for each. `effectiveness` is EFF, `isLinear` is true for "
            "LDW "
            "(linear downwash) and false for NOLDW, `refChord` and `refArea` are CREFC and CREFS. `positionLower` "
            "and "
            "`positionUpper` are the deflection limits PLLIM and PULIM in radians (null when a TABLED gives the limit "
            "as a function of the dynamic pressure), `hingeMomentLower` and `hingeMomentUpper` HMLLIM and HMULIM "
            "(null: no limit). `frameId2` and `aeroListId2` are null for a surface with one component. `boxes` are the "
            "distinct "
            "items of the AELIST cards ALID1 and ALID2 in increasing order and `boxCount` their number (null when a "
            "list the card names does not exist).";
        info.defaultSort = "id";
        ColumnListBuilder columns;
        columns.int32("id").text("label").int32("frameId1").int32("aeroListId1").int32("frameId2").int32("aeroListId2");
        columns.real("effectiveness").flag("isLinear").reals("refChord refArea positionLower positionUpper");
        columns.reals("hingeMomentLower hingeMomentUpper").int64("boxCount").intList("boxes");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addControlSurfacesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ControlSurfacesTableQuery>());
}

} // namespace oor::query
