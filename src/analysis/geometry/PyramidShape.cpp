#include "ElementProvider.hpp"
#include "SolidVolume.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/model/cards/elements/solidPyramid/SolidPyramidTable.hpp"

// The pyramid: five corners (G1..G4 the base, G5 the apex) and up to eight mid-side nodes. The volume is the
// isoparametric one of the hexahedron whose top face is collapsed to the apex (mid-side nodes included); for a planar
// base it is a third of the base area times the height. The centre is the average of the five corners.
//
// The element coordinate system is the one of the format reference: Xe = (G1+G2+G5)/3 - (G3+G4+G5)/3,
// V = (G2+G3+G5)/3 - (G1+G4+G5)/3, Ze = Xe x V and Ye = Ze x Xe.

namespace oor::analysis::detail
{

namespace
{

struct SolidPyramidShape
{
    static void nodes(const model::SolidPyramidTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const auto card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }

    static void evaluate(const model::SolidPyramidTable&, const Evaluation& evaluation, ElementValues& values)
    {
        const std::span<const base::Vec3> p = evaluation.points;
        for (std::size_t corner = 0; corner < 5; ++corner)
        {
            if (!isFinite(p[corner]))
                return;
        }
        if (evaluation.wants(Quantity::Center))
            values.center = (p[0] + p[1] + p[2] + p[3] + p[4]) / 5.0;
        if (evaluation.wants(Quantity::Volume))
            values.volume = pyramidVolume(p);
        if (evaluation.wants(Quantity::Frame))
        {
            const base::Vec3 x = unitOrNan((p[0] + p[1] + p[4]) / 3.0 - (p[2] + p[3] + p[4]) / 3.0);
            const base::Vec3 v = (p[1] + p[2] + p[4]) / 3.0 - (p[0] + p[3] + p[4]) / 3.0;
            const base::Vec3 z = unitOrNan(cross(x, v));
            values.frame = frameFromXAndZ(x, z);
        }
    }
};

} // namespace

void registerPyramidProviders(ProviderRegistry& registry)
{
    registry.add<model::SolidPyramidTable, SolidPyramidShape>();
}

} // namespace oor::analysis::detail
