#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/accelFieldLoad/AccelFieldLoadTable.hpp"
#include "oor/model/cards/loads/accelNodeLoad/AccelNodeLoadTable.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"
#include "oor/model/cards/loads/rotationalForceLoad/RotationalForceLoadTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/LoadVectors.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <array>
#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// One row of inertialLoadsTable: one inertial load card of a set.
struct InertialRow
{
    std::int32_t sid = 0;
    std::int32_t source = 0;
    std::string card;
    double scale = 1.0;
    std::int32_t cid = 0;
    base::Vec3 direction{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
    double acceleration = NOT_A_NUMBER;
    std::optional<std::int32_t> centerNode;
    base::Vec3 center{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
    double angularVelocity = NOT_A_NUMBER;
    double angularAcceleration = NOT_A_NUMBER;
    std::optional<std::int32_t> method;
    std::optional<std::vector<std::int32_t>> nodes;
    std::optional<std::string> locationDirection;
    std::optional<std::vector<double>> locations;
    std::optional<std::vector<double>> values;
    std::optional<std::int32_t> mb;
};

// The direction of a vector given in the system `frameId`, in the basic system, as a unit vector, and its length.
std::pair<base::Vec3, double> unitInBasic(LoadVectors& vectors, std::int32_t cid, const base::Vec3& point,
                                          const std::array<double, 3>& components)
{
    const base::Vec3 vector = vectors.inBasic(cid, point, base::Vec3{components[0], components[1], components[2]});
    const double length = base::norm(vector);
    if (!(length > 0.0))
        return {base::Vec3{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER}, 0.0};
    return {vector / length, length};
}

// inertialLoadsTable: the GRAV, ACCEL, ACCEL1 and RFORCE cards of the load sets, LOAD factors applied.
class InertialLoadsTableQuery final : public QueryBase
{
public:
    InertialLoadsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // the cards of every load set, read once for all the sets
        const analysis::LoadSetIndex loadSets(model);
        std::vector<base::EntityId> asked =
            askedSets(model, run.arguments, model::LOAD_SELECTION).value_or(allLoadSets(model));
        const LoadSetListing listing(model, loadSets, run.arguments, std::move(asked));
        LoadVectors vectors(model, run.context.pool());
        const std::vector<std::uint16_t> types =
            sortedTypesOf<model::GravityLoadTable, model::AccelFieldLoadTable, model::AccelNodeLoadTable,
                          model::RotationalForceLoadTable>();
        std::vector<InertialRow> rows;
        for (const base::EntityId set : listing.sets())
        {
            for (const analysis::ElementaryLoad& load : listing.loadsOf(set, types))
            {
                run.context.checkCancelled();
                InertialRow row;
                row.sid = set;
                row.source = load.sourceSid;
                row.scale = load.scale;
                const std::uint16_t type = load.card.table;
                const std::uint32_t at = load.card.row;
                if (type == model::typeIndexOf<model::GravityLoadTable>())
                {
                    // format reference GRAV: the acceleration is A times the vector (N1, N2, N3) of CID.
                    const model::GravityLoadCard card = model.findTable<model::GravityLoadTable>()->card(at);
                    const auto [direction, length] = unitInBasic(vectors, card.frameId, base::Vec3{}, card.direction);
                    row.card = "GravityLoad";
                    row.cid = card.frameId;
                    row.direction = direction;
                    row.acceleration = load.scale * card.magnitude * length;
                    row.mb = card.mainBulkFlag;
                }
                else if (type == model::typeIndexOf<model::AccelFieldLoadTable>())
                {
                    // format reference ACCEL: the acceleration (N1, N2, N3) of CID times a factor that varies along the
                    // DIR axis of CID, interpolated in the (LOCi, VALi) table.
                    const model::AccelFieldLoadCard card = model.findTable<model::AccelFieldLoadTable>()->card(at);
                    const auto [direction, length] = unitInBasic(vectors, card.frameId, base::Vec3{},
                                                                 {card.directionX, card.directionY, card.directionZ});
                    row.card = "AccelFieldLoad";
                    row.cid = card.frameId;
                    row.direction = direction;
                    row.acceleration = load.scale * length;
                    row.locationDirection =
                        std::string(model::keywordText(model::ACCEL_FIELD_DIRECTION_WORDS, card.direction));
                    row.locations.emplace();
                    row.values.emplace();
                    for (const auto& [location, value] : card.points)
                    {
                        row.locations->push_back(location);
                        row.values->push_back(value);
                    }
                }
                else if (type == model::typeIndexOf<model::AccelNodeLoadTable>())
                {
                    // format reference ACCEL1: the acceleration A times the vector (N1, N2, N3) of CID on the grid
                    // points listed.
                    const model::AccelNodeLoadCard card = model.findTable<model::AccelNodeLoadTable>()->card(at);
                    const auto [direction, length] = unitInBasic(vectors, card.frameId, base::Vec3{},
                                                                 {card.directionX, card.directionY, card.directionZ});
                    row.card = "AccelNodeLoad";
                    row.cid = card.frameId;
                    row.direction = direction;
                    row.acceleration = load.scale * card.magnitude * length;
                    row.nodes = std::vector<std::int32_t>(load.nodes.begin(), load.nodes.end());
                }
                else if (type == model::typeIndexOf<model::RotationalForceLoadTable>())
                {
                    // format reference RFORCE: the rotation about the axis (R1, R2, R3) of CID through grid point G
                    // (the origin of the basic system when G is blank), A revolutions per unit time and RACC per unit
                    // time squared.
                    const model::RotationalForceLoadCard card =
                        model.findTable<model::RotationalForceLoadTable>()->card(at);
                    const base::Vec3 point = card.node != 0 ? vectors.positionOf(card.node) : base::Vec3{};
                    const auto [direction, length] = unitInBasic(vectors, card.frameId, point, card.axis);
                    (void)length;
                    row.card = "RotationalForceLoad";
                    row.cid = card.frameId;
                    row.direction = direction;
                    if (card.node != 0)
                        row.centerNode = card.node;
                    row.center = point;
                    row.angularVelocity = card.magnitude;
                    row.angularAcceleration = card.angularAcceleration;
                    row.method = card.methodName;
                    row.mb = card.mainBulkFlag;
                }
                else
                    continue;
                rows.push_back(std::move(row));
            }
        }
        return tableOf(rows);
    }

private:
    static std::shared_ptr<const QueryTable> tableOf(const std::vector<InertialRow>& rows)
    {
        const std::size_t count = rows.size();
        std::vector<std::int32_t> sid(count);
        std::vector<std::int32_t> source(count);
        std::vector<std::string> card(count);
        std::vector<double> scale(count);
        std::vector<std::int32_t> cid(count);
        std::array<std::vector<double>, 3> direction;
        std::array<std::vector<double>, 3> center;
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            direction[axis].resize(count);
            center[axis].resize(count);
        }
        std::vector<double> acceleration(count);
        std::vector<std::int32_t> centerNode(count);
        std::vector<std::uint8_t> centerValid(count);
        std::vector<double> angularVelocity(count);
        std::vector<double> angularAcceleration(count);
        std::vector<std::int32_t> method(count);
        std::vector<std::uint8_t> methodValid(count);
        std::vector<std::int64_t> nodeOffsets{0};
        std::vector<std::int32_t> nodes;
        std::vector<std::uint8_t> nodesValid(count);
        std::vector<std::string> locationDirection(count);
        std::vector<std::uint8_t> locationValid(count);
        std::vector<std::int64_t> locationOffsets{0};
        std::vector<double> locations;
        std::vector<std::int64_t> valueOffsets{0};
        std::vector<double> values;
        std::vector<std::uint8_t> tableValid(count);
        std::vector<std::int32_t> mb(count);
        std::vector<std::uint8_t> mbValid(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const InertialRow& row = rows[index];
            sid[index] = row.sid;
            source[index] = row.source;
            card[index] = row.card;
            scale[index] = row.scale;
            cid[index] = row.cid;
            const std::array<double, 3> axes{row.direction.x, row.direction.y, row.direction.z};
            const std::array<double, 3> point{row.center.x, row.center.y, row.center.z};
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                direction[axis][index] = axes[axis];
                center[axis][index] = point[axis];
            }
            acceleration[index] = row.acceleration;
            centerNode[index] = row.centerNode.value_or(0);
            centerValid[index] = row.centerNode ? 1 : 0;
            angularVelocity[index] = row.angularVelocity;
            angularAcceleration[index] = row.angularAcceleration;
            method[index] = row.method.value_or(0);
            methodValid[index] = row.method ? 1 : 0;
            if (row.nodes)
                nodes.insert(nodes.end(), row.nodes->begin(), row.nodes->end());
            nodeOffsets.push_back(static_cast<std::int64_t>(nodes.size()));
            nodesValid[index] = row.nodes ? 1 : 0;
            locationDirection[index] = row.locationDirection.value_or(std::string());
            locationValid[index] = row.locationDirection ? 1 : 0;
            if (row.locations)
            {
                locations.insert(locations.end(), row.locations->begin(), row.locations->end());
                values.insert(values.end(), row.values->begin(), row.values->end());
            }
            locationOffsets.push_back(static_cast<std::int64_t>(locations.size()));
            valueOffsets.push_back(static_cast<std::int64_t>(values.size()));
            tableValid[index] = row.locations ? 1 : 0;
            mb[index] = row.mb.value_or(0);
            mbValid[index] = row.mb ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.int32("sourceSetId", std::move(source));
        builder.dictionary("card", card);
        builder.real("scale", std::move(scale));
        builder.int32("frameId", std::move(cid));
        builder.real("directionX", std::move(direction[0]));
        builder.real("directionY", std::move(direction[1]));
        builder.real("directionZ", std::move(direction[2]));
        builder.real("acceleration", std::move(acceleration));
        builder.int32("centerNodeId", std::move(centerNode), std::move(centerValid));
        builder.real("centerX", std::move(center[0]));
        builder.real("centerY", std::move(center[1]));
        builder.real("centerZ", std::move(center[2]));
        builder.real("angularVelocity", std::move(angularVelocity));
        builder.real("angularAcceleration", std::move(angularAcceleration));
        builder.int32("method", std::move(method), std::move(methodValid));
        builder.intList("nodeIds", std::move(nodeOffsets), std::move(nodes), std::move(nodesValid));
        builder.dictionary("locationDirection", locationDirection, std::move(locationValid));
        builder.realList("locations", std::move(locationOffsets), std::move(locations), tableValid);
        builder.realList("values", std::move(valueOffsets), std::move(values), std::move(tableValid));
        builder.int32("motionSystem", std::move(mb), std::move(mbValid));
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "inertialLoadsTable";
        info.family = "T8";
        info.description =
            "The inertial loads of the load sets asked for, one row for each GRAV, ACCEL, ACCEL1 and RFORCE card a set "
            "reaches, LOAD combinations expanded: setId is the set asked for, sourceSetId the set of the card, scale "
            "the product of the factors of the LOAD cards it was reached through. directionX/y/z is a unit vector in "
            "the basic system: the acceleration of a GRAV, ACCEL or ACCEL1, the axis of an RFORCE (a vector of a "
            "cylindrical or spherical CID is taken at the origin, or at G for an RFORCE). acceleration is the "
            "magnitude of the acceleration, scaled: A |N| of a GRAV and of an ACCEL1, |N| of an ACCEL (whose factor "
            "then varies along the locationDirection axis of CID as the table of locations and values says, format "
            "reference "
            "ACCEL). An RFORCE gives centerNodeId (its G, null when blank), centerX/y/z (the point of the axis in the "
            "basic system: G, or the origin), angularVelocity and angularAcceleration (A and RACC as given, in "
            "revolutions per unit time: the load they make grows with A squared, so `scale` multiplies the load, not "
            "them) and method. nodeIds are the grid points of an ACCEL1 that the model "
            "has (a THRU "
            "range gives those in it, format reference ACCEL1). mb is the MB of a GRAV or an RFORCE. A column that "
            "does not apply "
            "to a card is null. "
            "expandCombinations=False leaves the LOAD combinations unexpanded: each load set once, "
            "its own cards (setId and sourceSetId the set of the card, scale 1, the values as the card gives them), a "
            "combination asked for replaced by the sets it reaches; loadCombinationsTable gives its factors, and "
            "the rows of a set times its combinedFactor are the expanded rows of a LOAD of sets: to the bit for "
            "the scale and an ACCEL, within a rounding for the acceleration of a GRAV or an ACCEL1 (the expansion "
            "multiplies A by the factors before |N|).";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the load sets"),
                           parameter("subcases", ParameterKind::IdList, "None", "the sets these subcases select"),
                           expandCombinationsParameter()};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId").int32("sourceSetId").dictionary("card").real("scale").int32("frameId");
        columns.vector3("direction", "XYZ").real("acceleration").int32("centerNodeId").vector3("center", "XYZ");
        columns.reals("angularVelocity angularAcceleration").int32("method").intList("nodeIds");
        columns.dictionary("locationDirection").realList("locations").realList("values").int32("motionSystem");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addInertialLoadsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<InertialLoadsTableQuery>());
}

} // namespace oor::query
