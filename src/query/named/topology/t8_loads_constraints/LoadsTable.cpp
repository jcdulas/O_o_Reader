#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad/PointForceLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad1/PointForceLoad1Table.hpp"
#include "oor/model/cards/loads/pointForceLoad2/PointForceLoad2Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad/PointMomentLoadTable.hpp"
#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad2/PointMomentLoad2Table.hpp"
#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"
#include "oor/model/cards/loads/scalarPointLoad/ScalarPointLoadTable.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/LoadVectors.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <array>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// One row of loadsTable while it is built.
struct Row
{
    std::int32_t sid = 0;
    std::int32_t source = 0;
    std::string_view card; // the name of the card type, which outlives the table
    double scale = 1.0;
    std::string_view target = "node";
    std::int32_t targetId = 0;
    bool targetValid = true;
    std::array<double, 6> vector{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
    double pressure = NOT_A_NUMBER;
    std::array<double, 3> acceleration{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
    std::int32_t cid = 0;
    bool cidValid = false;
    std::array<std::int32_t, 4> directionNodes{}; // G1 to G4 of a FORCE1, FORCE2, MOMENT1, MOMENT2; 0 for none
};

// The columns of the table, a row appended to each at a time: no row is kept whole (a table of millions of loads would
// hold them twice).
struct Columns
{
    std::vector<std::int32_t> sid;
    std::vector<std::int32_t> source;
    std::vector<std::string_view> card;
    std::vector<double> scale;
    std::vector<std::string_view> target;
    std::vector<std::int32_t> targetId;
    std::vector<std::uint8_t> targetValid;
    std::array<std::vector<double>, 6> vector;
    std::vector<double> pressure;
    std::array<std::vector<double>, 3> acceleration;
    std::vector<std::int32_t> cid;
    std::vector<std::uint8_t> cidValid;
    std::array<std::vector<std::int32_t>, 4> direction;
    std::array<std::vector<std::uint8_t>, 4> directionValid;

    void reserve(std::size_t rows)
    {
        sid.reserve(rows);
        source.reserve(rows);
        card.reserve(rows);
        scale.reserve(rows);
        target.reserve(rows);
        targetId.reserve(rows);
        targetValid.reserve(rows);
        for (auto& values : vector)
            values.reserve(rows);
        pressure.reserve(rows);
        for (auto& values : acceleration)
            values.reserve(rows);
        cid.reserve(rows);
        cidValid.reserve(rows);
        for (std::size_t item = 0; item < 4; ++item)
        {
            direction[item].reserve(rows);
            directionValid[item].reserve(rows);
        }
    }

    void push(const Row& row)
    {
        sid.push_back(row.sid);
        source.push_back(row.source);
        card.push_back(row.card);
        scale.push_back(row.scale);
        target.push_back(row.target);
        targetId.push_back(row.targetId);
        targetValid.push_back(row.targetValid ? 1 : 0);
        for (std::size_t item = 0; item < 6; ++item)
            vector[item].push_back(row.vector[item]);
        pressure.push_back(row.pressure);
        for (std::size_t item = 0; item < 3; ++item)
            acceleration[item].push_back(row.acceleration[item]);
        cid.push_back(row.cid);
        cidValid.push_back(row.cidValid ? 1 : 0);
        for (std::size_t item = 0; item < 4; ++item)
        {
            direction[item].push_back(row.directionNodes[item]);
            directionValid[item].push_back(row.directionNodes[item] != 0 ? 1 : 0);
        }
    }
};

// The grid points that define the direction of a FORCE1, FORCE2, MOMENT1 or MOMENT2 (format reference: the direction
// from G1 to G2, or the cross product of G1 to G2 and G3 to G4); none for another card. A solver that moves grid points
// derives the direction again from them.
std::array<std::int32_t, 4> directionNodesOf(const model::Model& model, model::CardLocation card)
{
    using namespace oor::model;
    const std::uint16_t type = card.table;
    if (type == typeIndexOf<PointForceLoad1Table>())
    {
        const PointForceLoad1Card value = model.findTable<PointForceLoad1Table>()->card(card.row);
        return {value.node1, value.node2, 0, 0};
    }
    if (type == typeIndexOf<PointMomentLoad1Table>())
    {
        const PointMomentLoad1Card value = model.findTable<PointMomentLoad1Table>()->card(card.row);
        return {value.node1, value.node2, 0, 0};
    }
    if (type == typeIndexOf<PointForceLoad2Table>())
    {
        const PointForceLoad2Card value = model.findTable<PointForceLoad2Table>()->card(card.row);
        return {value.node1, value.node2, value.node3, value.node4};
    }
    if (type == typeIndexOf<PointMomentLoad2Table>())
    {
        const PointMomentLoad2Card value = model.findTable<PointMomentLoad2Table>()->card(card.row);
        return {value.node1, value.node2, value.node3, value.node4};
    }
    return {};
}

// loadsTable: one row for each elementary load of the sets asked for, LOAD combinations expanded.
class LoadsTableQuery final : public QueryBase
{
public:
    LoadsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // the cards of every load set, read once for all the sets
        const analysis::LoadSetIndex loadSets(model);
        std::vector<base::EntityId> asked =
            askedSets(model, run.arguments, model::LOAD_SELECTION).value_or(allLoadSets(model));
        const LoadSetListing listing(model, loadSets, run.arguments, std::move(asked));
        // the elements by ID, listed the first time a load gives a range of them (PLOAD2, PLOAD4)
        std::optional<CardList> elementList;
        const auto elementIds = [&]() -> const std::vector<std::int32_t>&
        {
            if (!elementList)
                elementList = listCards(model, typesOfCategory(model::CardCategory::Element));
            return elementList->ids;
        };
        LoadVectors vectors(model, run.context.pool());
        Columns rows;
        // unexpanded: a row a card of these types at least (millions of PLOAD2 of one element)
        rows.reserve(listing.cardCount(
            sortedTypesOf<model::PointForceLoadTable, model::PointForceLoad1Table, model::PointForceLoad2Table,
                          model::PointMomentLoadTable, model::PointMomentLoad1Table, model::PointMomentLoad2Table,
                          model::ScalarPointLoadTable, model::PointsPressureLoadTable, model::ShellPressureLoadTable,
                          model::FacePressureLoadTable, model::GravityLoadTable>()));
        for (const base::EntityId sid : listing.sets())
        {
            for (const analysis::ElementaryLoad& load : listing.loadsOf(sid))
            {
                run.context.checkCancelled();
                const std::string_view card = typeName(load.card.table);
                Row base;
                base.sid = sid;
                base.source = load.sourceSid;
                base.card = card;
                base.scale = load.scale;
                if (load.isConcentrated() && load.kind != analysis::LoadKind::EnforcedMotion)
                {
                    const auto cid = vectors.systemOf(load);
                    base.directionNodes = directionNodesOf(model, load.card);
                    for (const NodalLoad& nodal : vectors.nodal(load))
                    {
                        Row row = base;
                        row.targetId = nodal.node;
                        row.vector = {nodal.force.x,  nodal.force.y,  nodal.force.z,
                                      nodal.moment.x, nodal.moment.y, nodal.moment.z};
                        row.cid = cid.value_or(0);
                        row.cidValid = cid.has_value();
                        rows.push(row);
                    }
                }
                else if (load.kind == analysis::LoadKind::Gravity)
                {
                    Row row = base;
                    row.target = "model";
                    row.targetValid = false;
                    if (const auto acceleration = vectors.acceleration(load))
                        row.acceleration = {acceleration->x, acceleration->y, acceleration->z};
                    const auto cid = vectors.systemOf(load);
                    row.cid = cid.value_or(0);
                    row.cidValid = cid.has_value();
                    rows.push(row);
                }
                else if (load.kind == analysis::LoadKind::Rotation || load.kind == analysis::LoadKind::Acceleration ||
                         load.kind == analysis::LoadKind::PointAcceleration)
                {
                    // RFORCE, ACCEL and ACCEL1: listed so that no card of the set is left out, their content is in
                    // inertialLoadsTable.
                    Row row = base;
                    row.target = "model";
                    row.targetValid = false;
                    rows.push(row);
                }
                else if (load.kind == analysis::LoadKind::Pressure)
                {
                    const std::uint16_t type = load.card.table;
                    if (type == model::typeIndexOf<model::PointsPressureLoadTable>())
                    {
                        const model::PointsPressureLoadCard data =
                            static_cast<const model::PointsPressureLoadTable&>(*model.findTable(type))
                                .card(load.card.row);
                        for (const base::EntityId node : data.nodes)
                        {
                            if (node == 0)
                                continue;
                            Row row = base;
                            row.targetId = node;
                            row.pressure = load.scale * data.pressure;
                            rows.push(row);
                        }
                        continue;
                    }
                    double pressure = NOT_A_NUMBER;
                    std::int32_t cid = 0;
                    bool cidValid = false;
                    if (type == model::typeIndexOf<model::ShellPressureLoadTable>())
                        pressure =
                            load.scale * static_cast<const model::ShellPressureLoadTable&>(*model.findTable(type))
                                             .pressureAt(load.card.row);
                    else if (type == model::typeIndexOf<model::FacePressureLoadTable>())
                    {
                        const model::FacePressureLoadCard data =
                            static_cast<const model::FacePressureLoadTable&>(*model.findTable(type))
                                .card(load.card.row);
                        pressure = load.scale * data.pressures[0];
                        cid = data.frameId;
                        cidValid = true;
                    }
                    for (const analysis::IdRun& run2 : load.elements)
                    {
                        for (const std::int32_t eid : idsInRun(elementIds(), run2.first, run2.last))
                        {
                            Row row = base;
                            row.target = "element";
                            row.targetId = eid;
                            row.pressure = pressure;
                            row.cid = cid;
                            row.cidValid = cidValid;
                            rows.push(row);
                        }
                    }
                }
            }
        }
        TableBuilder builder;
        builder.int32("setId", std::move(rows.sid));
        builder.int32("sourceSetId", std::move(rows.source));
        builder.dictionary("card", rows.card);
        builder.real("scale", std::move(rows.scale));
        builder.dictionary("targetType", rows.target);
        builder.int32("targetId", std::move(rows.targetId), std::move(rows.targetValid));
        static const char* const NAMES[6] = {"fx", "fy", "fz", "mx", "my", "mz"};
        for (std::size_t item = 0; item < 6; ++item)
            builder.real(NAMES[item], std::move(rows.vector[item]));
        builder.real("pressure", std::move(rows.pressure));
        builder.real("ax", std::move(rows.acceleration[0]));
        builder.real("ay", std::move(rows.acceleration[1]));
        builder.real("az", std::move(rows.acceleration[2]));
        builder.int32("frameId", std::move(rows.cid), std::move(rows.cidValid));
        static const char* const DIRECTION[4] = {"g1", "g2", "g3", "g4"};
        for (std::size_t item = 0; item < 4; ++item)
            builder.int32(DIRECTION[item], std::move(rows.direction[item]), std::move(rows.directionValid[item]));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "loadsTable";
        info.family = "T8";
        info.description =
            "One row for each elementary load of the sets asked for, LOAD combinations expanded: setId is the set "
            "asked "
            "for, "
            "sourceSetId the set of the card, scale the product of the factors of the LOAD cards it was reached "
            "through. "
            "Concentrated loads (FORCE, FORCE1, FORCE2, MOMENT, MOMENT1, MOMENT2, SLOAD) give a row for each node with "
            "the force "
            "and moment in the basic system, scaled (an SLOAD puts its value in fx); GRAV gives the acceleration "
            "(ax ay az, scaled, in the basic system) on the target `model`; RFORCE, ACCEL and ACCEL1 are listed on the "
            "target `model` with no values (inertialLoadsTable gives them); PLOAD, "
            "PLOAD2 and "
            "PLOAD4 give the pressure of the card (the first one P1 of a PLOAD4, scaled) for each node or element they "
            "name. "
            "Line loads (PLOAD1), temperatures, enforced motions and imposed deformations are in lineLoadsTable, "
            "temperatureTable and singlePointConstrTable. setIds and subcases name the sets (a subcase selects the one "
            "its LOAD "
            "command names); "
            "with neither, every load set. g1 to g4 are the grid points that define the direction of a FORCE1, "
            "MOMENT1 (g1 and g2), FORCE2 or MOMENT2 (g1 to g4), null for the other cards: a solver that moves grid "
            "points derives the direction again from them. "
            "expandCombinations=False leaves the LOAD combinations unexpanded: each load set once, "
            "its own cards (setId and sourceSetId the set of the card, scale 1, the values as the card gives them), a "
            "combination asked for replaced by the sets it reaches; loadCombinationsTable gives its factors, and "
            "the rows of a set times its combinedFactor are the expanded rows of a LOAD of sets: to the bit for "
            "the pressure and the scale, within a rounding for a force, a moment or an acceleration (the expansion "
            "multiplies its magnitude by the factors before its direction).";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the load sets to list"),
                           parameter("subcases", ParameterKind::IdList, "None", "the sets these subcases select"),
                           expandCombinationsParameter()};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId")
            .int32("sourceSetId")
            .dictionary("card")
            .real("scale")
            .dictionary("targetType")
            .int32("targetId");
        columns.reals("fx fy fz mx my mz pressure ax ay az").int32("frameId");
        columns.int32("g1").int32("g2").int32("g3").int32("g4");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addLoadsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<LoadsTableQuery>());
}

} // namespace oor::query
