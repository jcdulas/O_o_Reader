#include "AeroQuerySupport.hpp"
#include "oor/analysis/geometry/AeroPanels.hpp"
#include "oor/base/errors/OorError.hpp"
#include "oor/model/cards/aero/AeroBoxRanges.hpp"
#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroMachBoxPanel3/AeroMachBoxPanel3Table.hpp"
#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// A panel card as the table shows it: the fields the five CAERO cards share, each optional where a card has no such
// field. A blank division number or list (0 on the card) is not given.
struct Panel
{
    Panel(base::EntityId cardEid, std::string cardType, base::EntityId property, base::EntityId system)
        : eid(cardEid), type(std::move(cardType)), pid(property), cp(system)
    {
    }

    base::EntityId eid = 0;
    std::string type;
    base::EntityId pid = 0;
    base::EntityId cp = 0;
    std::optional<std::int32_t> spanCount;
    std::optional<std::int32_t> chordCount;
    std::optional<base::EntityId> spanList;
    std::optional<base::EntityId> chordList;
    std::optional<base::EntityId> group;
    std::array<double, 3> point1 = {};
    double chord1 = 0.0;
    std::optional<std::array<double, 3>> point4; // a body (CAERO2) has one point only
    double chord4 = 0.0;
};

std::optional<std::int32_t> given(std::int32_t value)
{
    return value != 0 ? std::optional<std::int32_t>(value) : std::nullopt;
}

// The panel cards of the model, by EID.
std::vector<Panel> readPanels(const model::Model& model)
{
    std::vector<Panel> panels;
    for (const model::AeroLiftingPanel1Card& card : aero::cardsOf<model::AeroLiftingPanel1Table>(model))
    {
        Panel panel{card.id, "AeroLiftingPanel1", card.propId, card.positionFrameId};
        panel.spanCount = given(card.spanBoxCount);
        panel.chordCount = given(card.chordBoxCount);
        panel.spanList = given(card.spanDivisionListId);
        panel.chordList = given(card.chordDivisionListId);
        panel.group = card.interferenceGroupId;
        panel.point1 = card.point1;
        panel.chord1 = card.rootChord;
        panel.point4 = card.point4;
        panel.chord4 = card.tipChord;
        panels.push_back(std::move(panel));
    }
    // A body is cut along its axis, which is the chord direction of a panel: NSB and LSB go in the chord columns, the
    // interference elements (NINT and LINT) in the span columns.
    for (const model::AeroBody2Card& card : aero::cardsOf<model::AeroBody2Table>(model))
    {
        Panel panel{card.id, "AeroBody2", card.propId, card.positionFrameId};
        panel.chordCount = given(card.slenderElemCount);
        panel.chordList = given(card.slenderListId);
        panel.spanCount = given(card.interferenceElemCount);
        panel.spanList = given(card.interferenceListId);
        panel.group = card.interferenceGroupId;
        panel.point1 = card.point1;
        panel.chord1 = card.length;
        panels.push_back(std::move(panel));
    }
    for (const model::AeroMachBoxPanel3Card& card : aero::cardsOf<model::AeroMachBoxPanel3Table>(model))
    {
        Panel panel{card.id, "AeroMachBoxPanel3", card.propId, card.positionFrameId};
        panel.point1 = card.point1;
        panel.chord1 = card.rootChord;
        panel.point4 = card.point4;
        panel.chord4 = card.tipChord;
        panels.push_back(std::move(panel));
    }
    for (const model::AeroStripPanel4Card& card : aero::cardsOf<model::AeroStripPanel4Table>(model))
    {
        Panel panel{card.id, "AeroStripPanel4", card.propId, card.positionFrameId};
        panel.spanCount = given(card.spanBoxCount);
        panel.spanList = given(card.spanDivisionListId);
        panel.point1 = card.point1;
        panel.chord1 = card.rootChord;
        panel.point4 = card.point4;
        panel.chord4 = card.tipChord;
        panels.push_back(std::move(panel));
    }
    for (const model::AeroPistonPanel5Card& card : aero::cardsOf<model::AeroPistonPanel5Table>(model))
    {
        Panel panel{card.id, "AeroPistonPanel5", card.propId, card.positionFrameId};
        panel.spanCount = given(card.spanBoxCount);
        panel.spanList = given(card.spanDivisionListId);
        panel.point1 = card.point1;
        panel.chord1 = card.rootChord;
        panel.point4 = card.point4;
        panel.chord4 = card.tipChord;
        panels.push_back(std::move(panel));
    }
    std::ranges::stable_sort(panels, {}, &Panel::eid);
    return panels;
}

// The columns of the table, one vector each.
struct PanelColumns
{
    std::vector<std::int32_t> eid, pid, cp, first;
    std::vector<std::string> type;
    std::vector<std::int32_t> spanCount, chordCount, spanList, chordList, group;
    std::vector<std::uint8_t> spanCountValid, chordCountValid, spanListValid, chordListValid, groupValid;
    std::array<std::vector<double>, 3> point1, point4;
    std::vector<double> chord1, chord4, span, area;
    std::vector<std::int64_t> boxCount;
    std::vector<std::uint8_t> boxCountValid;

    static void put(const std::optional<std::int32_t>& value, std::vector<std::int32_t>& values,
                    std::vector<std::uint8_t>& valid)
    {
        values.push_back(value.value_or(0));
        valid.push_back(value ? 1 : 0);
    }
};

// aeroPanelsTable: the CAERO1 to CAERO5 cards with their leading edge in the basic system and the boxes they own.
class AeroPanelsTableQuery final : public QueryBase
{
public:
    AeroPanelsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("elementIds"));
        std::vector<Panel> panels = readPanels(model);
        if (wanted)
        {
            std::erase_if(panels, [&](const Panel& panel)
                          { return !std::binary_search(wanted->begin(), wanted->end(), std::int64_t{panel.eid}); });
        }
        if (panels.empty())
            return emptyTable(info().defaultColumns());

        const model::AeroBoxRanges ranges(model);
        analysis::PanelFrame frame(model);
        PanelColumns columns;
        for (const Panel& panel : panels)
        {
            columns.eid.push_back(panel.eid);
            columns.type.push_back(panel.type);
            columns.pid.push_back(panel.pid);
            columns.cp.push_back(panel.cp);
            PanelColumns::put(panel.spanCount, columns.spanCount, columns.spanCountValid);
            PanelColumns::put(panel.chordCount, columns.chordCount, columns.chordCountValid);
            PanelColumns::put(panel.spanList, columns.spanList, columns.spanListValid);
            PanelColumns::put(panel.chordList, columns.chordList, columns.chordListValid);
            PanelColumns::put(panel.group, columns.group, columns.groupValid);
            addOutline(frame, panel, columns);
            // The first box is the EID; the card owns the IDs up to its last box when its divisions are known.
            const model::AeroBoxRange* owned = ranges.owner(panel.eid);
            const bool known = owned != nullptr && owned->panelId == panel.eid && owned->countKnown;
            columns.first.push_back(panel.eid);
            columns.boxCount.push_back(known ? owned->boxCount : 0);
            columns.boxCountValid.push_back(known ? 1 : 0);
        }

        TableBuilder builder;
        builder.int32("elementId", std::move(columns.eid));
        builder.dictionary("type", columns.type);
        builder.int32("propId", std::move(columns.pid));
        builder.int32("positionFrameId", std::move(columns.cp));
        builder.int32("spanBoxCount", std::move(columns.spanCount), std::move(columns.spanCountValid));
        builder.int32("chordBoxCount", std::move(columns.chordCount), std::move(columns.chordCountValid));
        builder.int32("spanDivisionListId", std::move(columns.spanList), std::move(columns.spanListValid));
        builder.int32("chordDivisionListId", std::move(columns.chordList), std::move(columns.chordListValid));
        builder.int32("interferenceGroupId", std::move(columns.group), std::move(columns.groupValid));
        builder.real("x1", std::move(columns.point1[0]));
        builder.real("y1", std::move(columns.point1[1]));
        builder.real("z1", std::move(columns.point1[2]));
        builder.real("x12", std::move(columns.chord1));
        builder.real("x4", std::move(columns.point4[0]));
        builder.real("y4", std::move(columns.point4[1]));
        builder.real("z4", std::move(columns.point4[2]));
        builder.real("x43", std::move(columns.chord4));
        builder.real("span", std::move(columns.span));
        builder.real("area", std::move(columns.area));
        builder.int32("firstBox", std::move(columns.first));
        builder.int64("boxCount", std::move(columns.boxCount), std::move(columns.boxCountValid));
        return builder.finish();
    }

private:
    // The leading edge points in the basic system, the chords, and the span and area they make; NaN (a null) for what
    // the card does not give or what needs a coordinate system that does not exist.
    static void addOutline(analysis::PanelFrame& frame, const Panel& panel, PanelColumns& columns)
    {
        std::array<double, 3> inboard = {NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
        std::array<double, 3> outboard = inboard;
        double span = NOT_A_NUMBER;
        double area = NOT_A_NUMBER;
        try
        {
            const base::Vec3 point1 =
                frame.toBasic(panel.cp, base::Vec3{panel.point1[0], panel.point1[1], panel.point1[2]});
            inboard = {point1.x, point1.y, point1.z};
            if (panel.point4)
            {
                const base::Vec3 point4 =
                    frame.toBasic(panel.cp, base::Vec3{(*panel.point4)[0], (*panel.point4)[1], (*panel.point4)[2]});
                outboard = {point4.x, point4.y, point4.z};
                span = frame.spanBetween(point1, point4);
                area = 0.5 * (panel.chord1 + panel.chord4) * span;
            }
        }
        catch (const base::OorError&)
        {
            // a coordinate system the model lacks: the ID is reported by `ref.unresolved`; the cells stay null
        }
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            columns.point1[axis].push_back(inboard[axis]);
            columns.point4[axis].push_back(outboard[axis]);
        }
        columns.chord1.push_back(panel.chord1);
        columns.chord4.push_back(panel.point4 ? panel.chord4 : NOT_A_NUMBER);
        columns.span.push_back(span);
        columns.area.push_back(area);
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroPanelsTable";
        info.family = "T10";
        info.description =
            "The aerodynamic macro elements (CAERO1 to CAERO5), one row for each. The leading edge points 1 and 4 are "
            "taken from the system `positionFrameId` of the card to the basic system; `x12` and `x43` are the chords "
            "of the card. "
            "`span` is the distance between the two chords (the part of the vector from point 1 to point 4 that is "
            "perpendicular to the flow) and `area` the trapezoid (x12 + x43) / 2 times the span, which is the sum of "
            "the areas of its boxes when their division points run from 0 to 1 (an AEFACT list may leave a strip "
            "out). A CAERO2 body has its nose and length only (point 4, the second chord, the span "
            "and the area are null); its `chordBoxCount` and `chordDivisionListId` are NSB and LSB and its "
            "`spanBoxCount` and `spanDivisionListId` NINT and "
            "LINT. A CAERO3 has no divisions on the card. A CAERO4 or CAERO5 strip panel has no chord divisions. A "
            "blank division number or list is null; `interferenceGroupId` is null for a card with no IGID. The boxes "
            "of a panel are "
            "numbered from its EID on: `firstBox` is the EID and `boxCount` the number of IDs the card owns (null "
            "when the AEFACT that cuts it is missing or has fewer than two points).";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "the panels to list")};
        info.defaultSort = "elementId";
        ColumnListBuilder columns;
        columns.int32("elementId")
            .dictionary("type")
            .int32("propId")
            .int32("positionFrameId")
            .int32("spanBoxCount")
            .int32("chordBoxCount")
            .int32("spanDivisionListId");
        columns.int32("chordDivisionListId").int32("interferenceGroupId").reals("x1 y1 z1 x12 x4 y4 z4 x43 span area");
        columns.int32("firstBox").int64("boxCount");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addAeroPanelsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<AeroPanelsTableQuery>());
}

} // namespace oor::query
