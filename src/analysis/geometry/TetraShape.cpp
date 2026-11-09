#include "ElementProvider.hpp"
#include "SolidVolume.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/model/cards/elements/solidTetra/SolidTetraTable.hpp"

// The tetrahedron: four corners and up to six mid-side nodes. The volume is the isoparametric one (mid-side nodes
// included; for straight edges, the sixth of the determinant of the three edges at G1). The centre is the average of
// the four corners.
//
// The element coordinate system is the one of the format reference: the vectors R, S and T join the
// middles of opposite edges (R: G1-G2 to G3-G4, S: G1-G3 to G2-G4, T: G1-G4 to G2-G3), and the axes are the rotation
// closest to them, the one for which R, S and T described in the element system form a symmetric positive definite
// matrix (the polar decomposition). The corners alone give the vectors; the mid-side nodes do not change them.

namespace oor::analysis::detail
{

namespace
{

struct SolidTetraShape
{
    static void nodes(const model::SolidTetraTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const auto card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }

    static void evaluate(const model::SolidTetraTable&, const Evaluation& evaluation, ElementValues& values)
    {
        const std::span<const base::Vec3> p = evaluation.points;
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            if (!isFinite(p[corner]))
                return;
        }
        if (evaluation.wants(Quantity::Center))
            values.center = (p[0] + p[1] + p[2] + p[3]) / 4.0;
        if (evaluation.wants(Quantity::Volume))
            values.volume = tetrahedronVolume(p);
        if (evaluation.wants(Quantity::Frame))
        {
            const base::Vec3 r = 0.5 * (p[2] + p[3]) - 0.5 * (p[0] + p[1]);
            const base::Vec3 s = 0.5 * (p[1] + p[3]) - 0.5 * (p[0] + p[2]);
            const base::Vec3 t = 0.5 * (p[1] + p[2]) - 0.5 * (p[0] + p[3]);
            values.frame = polarRotation(base::Mat3::fromColumns(r, s, t));
        }
    }
};

} // namespace

void registerTetraProviders(ProviderRegistry& registry)
{
    registry.add<model::SolidTetraTable, SolidTetraShape>();
}

} // namespace oor::analysis::detail
