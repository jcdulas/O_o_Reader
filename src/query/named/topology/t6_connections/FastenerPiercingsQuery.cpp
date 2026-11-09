#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// fastenerPiercings: where each side of each CFAST and CWELD pierces the shells, with the element pierced, its natural
// coordinates and the values of its shape functions there (analysis::FastenerPiercings).
class FastenerPiercingsQuery final : public QueryBase
{
public:
    FastenerPiercingsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const auto piercings = analysis::FastenerPiercings::of(model, run.context.pool());
        const std::vector<analysis::FastenerPiercing>& sides = piercings->sides();
        if (sides.empty())
            return emptyTable(info().defaultColumns());
        const std::size_t count = sides.size();
        std::vector<std::int32_t> eid(count);
        std::vector<std::string_view> card(count);
        std::vector<std::string_view> form(count);
        std::vector<std::string> side(count);
        std::vector<std::int32_t> pointGrid(count);
        std::vector<std::uint8_t> pointGridValid(count);
        std::vector<double> x(count, NOT_A_NUMBER);
        std::vector<double> y(count, NOT_A_NUMBER);
        std::vector<double> z(count, NOT_A_NUMBER);
        std::vector<std::int32_t> element(count);
        std::vector<std::uint8_t> elementValid(count);
        std::vector<std::string_view> elementType(count);
        std::vector<std::uint8_t> elementTypeValid(count);
        std::vector<double> xi(count, NOT_A_NUMBER);
        std::vector<double> eta(count, NOT_A_NUMBER);
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> nids;
        std::vector<double> shapeValues;
        std::vector<std::string> field(count);
        std::vector<std::uint8_t> fieldValid(count);
        std::vector<std::string> problem(count);
        std::vector<std::uint8_t> problemValid(count);
        for (std::size_t row = 0; row < count; ++row)
        {
            const analysis::FastenerPiercing& piercing = sides[row];
            eid[row] = piercing.eid;
            card[row] = typeName(piercing.card.table);
            form[row] = analysis::fastenerFormName(piercing.form);
            side[row] = std::string(1, piercing.side);
            pointGrid[row] = piercing.pointGrid;
            pointGridValid[row] = piercing.pointGrid != 0 ? 1 : 0;
            if (piercing.isFound)
            {
                x[row] = piercing.position.x;
                y[row] = piercing.position.y;
                z[row] = piercing.position.z;
                xi[row] = piercing.xi;
                eta[row] = piercing.eta;
                if (piercing.hasElement)
                {
                    element[row] = piercing.elementId;
                    elementValid[row] = 1;
                    elementType[row] = typeName(piercing.element.table);
                }
                else
                    elementType[row] = "NodeIdPatch";
                elementTypeValid[row] = 1;
                for (std::size_t k = 0; k < piercing.nids.size(); ++k)
                {
                    nids.push_back(piercing.nids[k]);
                    shapeValues.push_back(piercing.shapeValues[k]);
                }
            }
            else
            {
                field[row] = piercing.field;
                fieldValid[row] = 1;
                problem[row] = piercing.problem;
                problemValid[row] = 1;
            }
            offsets.push_back(static_cast<std::int64_t>(nids.size()));
        }
        TableBuilder builder;
        builder.int32("elementId", std::move(eid));
        builder.dictionary("card", card);
        builder.dictionary("form", form);
        builder.dictionary("side", side);
        builder.int32("pointGrid", std::move(pointGrid), std::move(pointGridValid));
        builder.real("x", std::move(x));
        builder.real("y", std::move(y));
        builder.real("z", std::move(z));
        builder.int32("element", std::move(element), std::move(elementValid));
        builder.dictionary("elementType", elementType, std::move(elementTypeValid));
        builder.real("xi", std::move(xi));
        builder.real("eta", std::move(eta));
        builder.intList("nodeIds", offsets, std::move(nids));
        builder.realList("shapeValues", std::move(offsets), std::move(shapeValues));
        builder.dictionary("field", field, std::move(fieldValid));
        builder.text("problem", problem, std::move(problemValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "fastenerPiercings";
        info.family = "T6";
        info.description =
            "Where each CFAST and CWELD pierces the shells, from the geometry alone: one row per fastener and side "
            "(A, then B; a point-to-patch CWELD has side A only, an ALIGN CWELD no row: its ends are GA and GB). card "
            "and form the card and its TYPE (its neutral word: Elements, ElemPatch, ...); pointGrid GA or GB when the "
            "card gives it (the point is then that grid "
            "point); x y z the piercing point in the basic system; element and elementType the shell pierced "
            "(element null and elementType NodeIdPatch for the face of grid points of a CWELD GRIDID); xi eta the "
            "natural "
            "coordinates of the point on it (area coordinates L2 L3 on a triangle), within it; nodeIds the grid points "
            "of "
            "the element in its order and shapeValues its shape functions there (they sum to 1). Side A is the "
            "projection of GS (or XS, YS, ZS) on the shells of side A, side B the crossing of the normal to side A at "
            "A with the shells of side B; a point is accepted within PROJTOL of SWLDPRM (0.02) of an element's edges "
            "in natural coordinates and within 10 element sizes. A side not found has null coordinates, the field "
            "of the card concerned and the problem.";
        info.defaultSort = "elementId, side";
        ColumnListBuilder columns;
        columns.int32("elementId").dictionary("card").dictionary("form").dictionary("side").int32("pointGrid");
        columns.reals("x y z").int32("element").dictionary("elementType").reals("xi eta");
        columns.intList("nodeIds").realList("shapeValues").dictionary("field").text("problem");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFastenerPiercingsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FastenerPiercingsQuery>());
}

} // namespace oor::query
