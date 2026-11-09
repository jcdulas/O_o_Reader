#include "ElementProvider.hpp"
#include "SolidVolume.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/model/cards/elements/solidPenta/SolidPentaTable.hpp"

// The pentahedron (wedge): six corners and up to nine mid-side nodes. The volume is the isoparametric one (mid-side
// nodes included). The centre is the average of the six corners.
//
// The element coordinate system is the one the format reference gives by formula for the PSOLID option CORDM = -2
// (CPENTA figure 9-46): the origin is the middle of the edge G1-G4, X points to the middle of the other four corners, V
// joins the middle of the face G1-G3-G4-G6 to the middle of the face G1-G2-G4-G5, Z = X x V and Y = Z x X. (The format
// reference describes the internal system in words only; this formula is the one it draws.)

namespace oor::analysis::detail
{

namespace
{

struct SolidPentaShape
{
    static void nodes(const model::SolidPentaTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const auto card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }

    static void evaluate(const model::SolidPentaTable&, const Evaluation& evaluation, ElementValues& values)
    {
        const std::span<const base::Vec3> p = evaluation.points;
        for (std::size_t corner = 0; corner < 6; ++corner)
        {
            if (!isFinite(p[corner]))
                return;
        }
        if (evaluation.wants(Quantity::Center))
            values.center = (p[0] + p[1] + p[2] + p[3] + p[4] + p[5]) / 6.0;
        if (evaluation.wants(Quantity::Volume))
            values.volume = wedgeVolume(p);
        if (evaluation.wants(Quantity::Frame))
        {
            const base::Vec3 origin = 0.5 * (p[0] + p[3]);
            const base::Vec3 x = unitOrNan(0.25 * (p[1] + p[2] + p[4] + p[5]) - origin);
            const base::Vec3 v = 0.25 * (p[0] + p[2] + p[3] + p[5]) - 0.25 * (p[0] + p[1] + p[3] + p[4]);
            const base::Vec3 z = unitOrNan(cross(x, v));
            values.frame = frameFromXAndZ(x, z);
        }
    }
};

} // namespace

void registerPentaProviders(ProviderRegistry& registry)
{
    registry.add<model::SolidPentaTable, SolidPentaShape>();
}

} // namespace oor::analysis::detail
