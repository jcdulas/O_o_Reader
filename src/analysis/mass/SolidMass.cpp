#include "MassProvider.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"
#include "oor/model/cards/elements/solidPenta/SolidPentaTable.hpp"
#include "oor/model/cards/elements/solidPyramid/SolidPyramidTable.hpp"
#include "oor/model/cards/elements/solidTetra/SolidTetraTable.hpp"

// The mass of the solid elements: the density of the material of PSOLID (or PLSOLID) times the volume of the element,
// the isoparametric volume of the geometry with the mid-side nodes. A solid of a fluid property (PSOLID with FCTN
// PFLUID or FFLUID) is not structural mass and counts for nothing, as the format reference says the fluid masses are
// ignored.

namespace oor::analysis::detail
{

namespace
{

template <typename TableT>
struct SolidShape
{
    static constexpr std::uint8_t QUANTITIES = bit(Quantity::Center) | bit(Quantity::Volume);

    static void evaluate(const TableT& table, std::uint32_t row, MassContext& context, const ElementValues& geometry,
                         MassValue& value)
    {
        const base::EntityId pid = table.card(row).propId;
        const Owner owner{TableT::NAME.data(), table.idAtRow(row)};
        const SolidData& data = context.solid(pid, owner);
        if (!data.found)
        {
            throw MassError(owner.text(context.model()) + " refers to property " + std::to_string(pid) +
                            ", which is not a solid property");
        }
        value.property = pid;
        value.material = data.material;
        value.center = geometry.center;
        value.structural = data.massless ? 0.0 : data.density * geometry.volume;
    }
};

} // namespace

void registerSolidMass(MassRegistry& registry)
{
    registry.add<model::SolidTetraTable, SolidShape<model::SolidTetraTable>>();
    registry.add<model::SolidPentaTable, SolidShape<model::SolidPentaTable>>();
    registry.add<model::SolidHexaTable, SolidShape<model::SolidHexaTable>>();
    registry.add<model::SolidPyramidTable, SolidShape<model::SolidPyramidTable>>();
}

} // namespace oor::analysis::detail
