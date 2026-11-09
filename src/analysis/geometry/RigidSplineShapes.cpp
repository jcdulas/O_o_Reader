#include "ElementProvider.hpp"
#include "oor/model/cards/rigid_elements/rigidAxisymConnection/RigidAxisymConnectionTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodySearched/RigidBodySearchedTable.hpp"

// The RBE2GS and the RBAX3D, the two element cards of the rigid element family that no other file of this folder
// holds.
//
// An RBE2GS has no connectivity: it names the grid or point GS where the solver starts its search for the two grids of
// the RBE2 it builds (or, with GS blank, a search location that is three numbers). The grids the solver picks are not
// on the card, and the exclusion lists name grids that may not be picked, not grids the element connects. So the nodes
// of the element are GS alone (0 when GS is blank), which puts the element in the node to element index of its search
// grid, and every quantity (centre, area, volume, length, normal) is left NaN: the format reference gives no shape for
// it. The search radius R is not a length of the element either.
//
// An RBAX3D connects two grid points, G3D on the axis of symmetry and GAX on an axisymmetric harmonic element, like an
// RBAR does: its centre is the middle of the two points and its length their distance, which is the radius of GAX when
// both have the same axial coordinate. A point that is not a grid point leaves the quantities NaN.

namespace oor::analysis::detail
{

namespace
{

struct RigidBodySearchedShape
{
    static void nodes(const model::RigidBodySearchedTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        ids.push_back(table.gsAt(row));
    }
    static void evaluate(const model::RigidBodySearchedTable&, const Evaluation&, ElementValues&) {}
};

struct RigidAxisymConnectionShape
{
    static void nodes(const model::RigidAxisymConnectionTable& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        ids.push_back(table.node3DAt(row));
        ids.push_back(table.axisymNodeAt(row));
    }
    static void evaluate(const model::RigidAxisymConnectionTable&, const Evaluation& evaluation, ElementValues& values)
    {
        const base::Vec3& independent = evaluation.points[0];
        const base::Vec3& dependent = evaluation.points[1];
        if (!isFinite(independent) || !isFinite(dependent))
            return;
        if (evaluation.wants(Quantity::Center))
            values.center = (independent + dependent) / 2.0;
        if (evaluation.wants(Quantity::Length))
            values.length = base::norm(dependent - independent);
    }
};

} // namespace

void registerRigidSplineProviders(ProviderRegistry& registry)
{
    registry.add<model::RigidBodySearchedTable, RigidBodySearchedShape>();
    registry.add<model::RigidAxisymConnectionTable, RigidAxisymConnectionShape>();
}

} // namespace oor::analysis::detail
