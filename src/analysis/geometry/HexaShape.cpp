#include "ElementProvider.hpp"
#include "SolidVolume.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"

// The hexahedron: eight corners and up to twelve mid-side nodes. The volume is the isoparametric one (mid-side nodes
// included). The centre is the average of the eight corners.
//
// The element coordinate system is the one of the format reference: the vectors R, S and T join the
// centroids of opposite faces (R: the face G4-G1-G5-G8 to the face G3-G2-G6-G7, S: G1-G2-G6-G5 to G4-G3-G7-G8, T:
// G1-G2-G3-G4 to G5-G6-G7-G8), and the axes are the rotation closest to them (the polar decomposition of the matrix of
// R, S and T), so that for a box with its edges along the basic axes the element system is the basic one.

namespace oor::analysis::detail
{

namespace
{

struct SolidHexaShape
{
    static void nodes(const model::SolidHexaTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const auto card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }

    static void evaluate(const model::SolidHexaTable&, const Evaluation& evaluation, ElementValues& values)
    {
        const std::span<const base::Vec3> p = evaluation.points;
        for (std::size_t corner = 0; corner < 8; ++corner)
        {
            if (!isFinite(p[corner]))
                return;
        }
        if (evaluation.wants(Quantity::Center))
            values.center = (p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7]) / 8.0;
        if (evaluation.wants(Quantity::Volume))
            values.volume = hexahedronVolume(p);
        if (evaluation.wants(Quantity::Frame))
        {
            const base::Vec3 r = 0.25 * (p[2] + p[1] + p[5] + p[6]) - 0.25 * (p[3] + p[0] + p[4] + p[7]);
            const base::Vec3 s = 0.25 * (p[3] + p[2] + p[6] + p[7]) - 0.25 * (p[0] + p[1] + p[5] + p[4]);
            const base::Vec3 t = 0.25 * (p[4] + p[5] + p[6] + p[7]) - 0.25 * (p[0] + p[1] + p[2] + p[3]);
            values.frame = polarRotation(base::Mat3::fromColumns(r, s, t));
        }
    }
};

} // namespace

void registerHexaProviders(ProviderRegistry& registry)
{
    registry.add<model::SolidHexaTable, SolidHexaShape>();
}

} // namespace oor::analysis::detail
