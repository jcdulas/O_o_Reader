#include "AeroQuerySupport.hpp"
#include "SplineEntries.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <algorithm>
#include <limits>
#include <optional>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The boxes a spline moves: the range its card gives, or the items of its AELIST. Nothing known when the AELIST is
// missing.
struct SplineBoxes
{
    std::optional<base::EntityId> first;
    std::optional<base::EntityId> last;
    std::optional<std::int64_t> count;
};

SplineBoxes boxesOf(const model::Model& model, const aero::SplineEntry& spline)
{
    SplineBoxes boxes;
    if (spline.boxRange)
    {
        boxes.first = spline.boxRange->first;
        boxes.last = spline.boxRange->second;
        // BOX2 is not below BOX1 (a check says otherwise); a card that breaks it has no count.
        if (spline.boxRange->second >= spline.boxRange->first)
            boxes.count = static_cast<std::int64_t>(spline.boxRange->second) - spline.boxRange->first + 1;
        return boxes;
    }
    if (spline.aelist == 0)
        return boxes;
    if (const std::optional<aero::IdRuns> items = aero::aelistItems(model, spline.aelist); items && !items->empty())
    {
        boxes.first = items->first();
        boxes.last = items->last();
        boxes.count = items->count();
    }
    return boxes;
}

// How many structural points a spline uses: the distinct IDs of its set, or the points of its own list. Nothing known
// when the card names no set or the set does not exist.
std::optional<std::int64_t> structuralPointCount(const model::Model& model, const aero::SplineEntry& spline)
{
    if (spline.hasPointList)
        return static_cast<std::int64_t>(spline.points.size());
    if (spline.setg == 0)
        return std::nullopt;
    const std::optional<aero::IdRuns> members = aero::setMembers(model, spline.setg);
    return members ? std::optional<std::int64_t>(members->count()) : std::nullopt;
}

template <typename T>
void put(const std::optional<T>& value, std::vector<T>& values, std::vector<std::uint8_t>& valid)
{
    values.push_back(value.value_or(T{}));
    valid.push_back(value ? 1 : 0);
}

// splinesTable: SPLINE1 to SPLINE7 and SPLINEX, one row for each.
class SplinesTableQuery final : public QueryBase
{
public:
    SplinesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::vector<aero::SplineEntry> splines = aero::readSplines(model);
        if (splines.empty())
            return emptyTable(info().defaultColumns());

        std::vector<std::int32_t> eid, caero, first, last, aelist, setg;
        std::vector<std::uint8_t> caeroValid, firstValid, lastValid, aelistValid, setgValid;
        std::vector<std::int64_t> nodeCount, boxCount;
        std::vector<std::uint8_t> nodeCountValid, boxCountValid;
        std::vector<double> dz;
        std::vector<std::string> type, method, usage;
        std::vector<std::uint8_t> methodValid;
        const auto nonZero = [](base::EntityId id)
        { return id != 0 ? std::optional<base::EntityId>(id) : std::nullopt; };
        for (const aero::SplineEntry& spline : splines)
        {
            const SplineBoxes boxes = boxesOf(model, spline);
            eid.push_back(spline.eid);
            type.emplace_back(spline.type);
            put(nonZero(spline.caero), caero, caeroValid);
            put(boxes.first, first, firstValid);
            put(boxes.last, last, lastValid);
            put(nonZero(spline.aelist), aelist, aelistValid);
            put(nonZero(spline.setg), setg, setgValid);
            put(structuralPointCount(model, spline), nodeCount, nodeCountValid);
            put(boxes.count, boxCount, boxCountValid);
            dz.push_back(spline.dz.value_or(NOT_A_NUMBER));
            method.emplace_back(spline.method);
            methodValid.push_back(spline.method.empty() ? 0 : 1);
            usage.emplace_back(spline.usage);
        }
        TableBuilder builder;
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        builder.int32("aeroPanelId", std::move(caero), std::move(caeroValid));
        builder.int32("firstBox", std::move(first), std::move(firstValid));
        builder.int32("lastBox", std::move(last), std::move(lastValid));
        builder.int32("aeroBoxListId", std::move(aelist), std::move(aelistValid));
        builder.int32("nodeSetId", std::move(setg), std::move(setgValid));
        builder.int64("structuralNodeCount", std::move(nodeCount), std::move(nodeCountValid));
        builder.int64("boxCount", std::move(boxCount), std::move(boxCountValid));
        builder.real("dz", std::move(dz));
        builder.dictionary("method", method, std::move(methodValid));
        builder.dictionary("usage", usage);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "splinesTable";
        info.family = "T10";
        info.description =
            "The splines (SPLINE1 to SPLINE7 and SPLINEX), one row for each. `firstBox` and `lastBox` are BOX1 and "
            "BOX2 of a SPLINE1, ID1 and ID2 of a SPLINE2, the BOXID of a SPLINE3 (both), and the lowest and highest "
            "item of the AELIST of the other kinds; `boxCount` is the number of boxes the spline moves (null when its "
            "AELIST is missing). `structuralNodeCount` is the number of distinct IDs of its SETG (the points G of a "
            "SPLINE3); null when the card names no set or the set does not exist. `aeroPanelId` is null for a SPLINEX "
            "and "
            "for a SPLINE6 or SPLINE7 that names none; `dz` is null for a SPLINE3 and a SPLINEX; `method` is the METH "
            "(IPS, TPS, FPS, RIS, BEAM) of a SPLINE1, SPLINE4 and SPLINE5, the METHOD (FPS3, FPS6, FBS3, FBS6) of a "
            "SPLINE6 and SPLINE7, and null for the others; `usage` is FORCE, DISP or BOTH.";
        info.defaultSort = "elementId";
        ColumnListBuilder columns;
        columns.int32("elementId")
            .dictionary("type")
            .int32("aeroPanelId")
            .int32("firstBox")
            .int32("lastBox")
            .int32("aeroBoxListId");
        columns.int32("nodeSetId").int64("structuralNodeCount").int64("boxCount").real("dz").dictionary("method");
        columns.dictionary("usage");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addSplinesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SplinesTableQuery>());
}

} // namespace oor::query
