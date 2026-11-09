#include "ElementProvider.hpp"
#include "oor/model/cards/elements/pointMassMatrix/PointMassMatrixTable.hpp"
#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"
#include "oor/model/cards/explicit/markerNode/MarkerNodeTable.hpp"

// The concentrated mass elements. Their centre is where the mass is.
//
// The marker element CMARKN1 of the Euler domain sits at its grid point, like CONM1.
//
// CONM1 sits at its grid point. CONM2 sits at its center of gravity, which is the grid point shifted by the offset X1,
// X2, X3 read in the system CID (format reference): the components of the offset are along the axes of the system
// (along the directions of the coordinates at the grid point for a cylindrical or spherical one); CID 0 is the basic
// system; CID -1 says X1, X2, X3 are the coordinates, not an offset, of the center of gravity in the basic system.

namespace oor::analysis::detail
{

namespace
{

struct PointMassMatrixShape
{
    static void nodes(const model::PointMassMatrixTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        ids.push_back(table.card(row).node);
    }
    static void evaluate(const model::PointMassMatrixTable&, const Evaluation& evaluation, ElementValues& values)
    {
        if (evaluation.wants(Quantity::Center) && isFinite(evaluation.points[0]))
            values.center = evaluation.points[0];
    }
};

struct PointMassInertiaShape
{
    static void nodes(const model::PointMassInertiaTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        ids.push_back(table.card(row).node);
    }
    static void evaluate(const model::PointMassInertiaTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        if (!evaluation.wants(Quantity::Center) || !isFinite(evaluation.points[0]))
            return;
        const model::PointMassInertiaCard card = table.card(evaluation.row);
        const base::Vec3 offset{card.offset[0], card.offset[1], card.offset[2]};
        if (card.frameId == -1)
            values.center = offset;
        else if (card.frameId == 0)
            values.center = evaluation.points[0] + offset;
        else
            values.center =
                evaluation.points[0] + evaluation.context.systemAxesAt(card.frameId, evaluation.points[0]) * offset;
    }
};

// The one-noded marker element of the Euler domain: a point at its grid point.
struct MarkerNodeShape
{
    static void nodes(const model::MarkerNodeTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        for (const base::EntityId node : table.nodes(row))
            ids.push_back(node);
    }
    static void evaluate(const model::MarkerNodeTable&, const Evaluation& evaluation, ElementValues& values)
    {
        if (evaluation.wants(Quantity::Center) && isFinite(evaluation.points[0]))
            values.center = evaluation.points[0];
    }
};

} // namespace

void registerPointProviders(ProviderRegistry& registry)
{
    registry.add<model::PointMassMatrixTable, PointMassMatrixShape>();
    registry.add<model::PointMassInertiaTable, PointMassInertiaShape>();
    registry.add<model::MarkerNodeTable, MarkerNodeShape>();
}

} // namespace oor::analysis::detail
