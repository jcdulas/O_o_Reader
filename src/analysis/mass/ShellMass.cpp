#include "MassProvider.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/model/cards/elements/axisymQuad/AxisymQuadTable.hpp"
#include "oor/model/cards/elements/axisymQuad4/AxisymQuad4Table.hpp"
#include "oor/model/cards/elements/axisymQuad8/AxisymQuad8Table.hpp"
#include "oor/model/cards/elements/axisymTria/AxisymTriaTable.hpp"
#include "oor/model/cards/elements/axisymTria6/AxisymTria6Table.hpp"
#include "oor/model/cards/elements/planeStrainTria3/PlaneStrainTria3Table.hpp"
#include "oor/model/cards/elements/planeStrainQuad4/PlaneStrainQuad4Table.hpp"
#include "oor/model/cards/elements/planeStrainTria6/PlaneStrainTria6Table.hpp"
#include "oor/model/cards/elements/planeStrainQuad8/PlaneStrainQuad8Table.hpp"
#include "oor/model/cards/elements/planeStressTria3/PlaneStressTria3Table.hpp"
#include "oor/model/cards/elements/planeStressQuad4/PlaneStressQuad4Table.hpp"
#include "oor/model/cards/elements/planeStressTria6/PlaneStressTria6Table.hpp"
#include "oor/model/cards/elements/planeStressQuad8/PlaneStressQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/planeStrainQuad/PlaneStrainQuadTable.hpp"
#include "oor/model/cards/elements/shearPanel4/ShearPanel4Table.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"

// The mass of the shells, the shear panels, the plane elements and the axisymmetric elements.
//
//   PSHELL, PSHEAR   area times (density times the thickness of the element + NSM); the thickness is the effective one
//                    of the element (the mean of its corner thicknesses when it has some, else the property's), the
//                    density is the one of MID1 (MID2 when there is no MID1)
//   PCOMP, PCOMPG    area times (the sum over the plies of density times thickness + NSM)
//   PLPLANE          density times the volume of the element: a plane strain element is one unit deep and an
//   axisymmetric
//                    one is a ring of 360 degrees (the geometry gives both)
//   CTRIAX6          the same, with the material on the element card

namespace oor::analysis::detail
{

namespace
{

constexpr std::uint8_t SHELL_QUANTITIES = bit(Quantity::Center) | bit(Quantity::Area) | bit(Quantity::Volume) |
                                          bit(Quantity::Thickness) | bit(Quantity::Normal);

// The offset ZOFFS of the card in `row` (the distance from the plane of the grid points to the reference plane of the
// shell, along its normal; format reference CQUAD4), 0 for a card that has none.
template <typename TableT>
double offsetOf(const TableT& table, std::uint32_t row)
{
    if constexpr (requires { table.card(row).offset; })
        return table.card(row).offset;
    else
        return 0.0;
}

// An element with a property: a shell, a shear panel, a plane or axisymmetric element.
template <typename TableT>
struct ShellShape
{
    static constexpr std::uint8_t QUANTITIES = SHELL_QUANTITIES;

    static void evaluate(const TableT& table, std::uint32_t row, MassContext& context, const ElementValues& geometry,
                         MassValue& value)
    {
        const base::EntityId pid = table.card(row).propId;
        const Owner owner{TableT::NAME.data(), table.idAtRow(row)};
        const ShellData& data = context.shell(pid, owner);
        value.property = pid;
        value.material = data.material;
        value.center = geometry.center;
        switch (data.kind)
        {
        case ShellData::Kind::Plate:
            value.structural = data.density * geometry.thickness * geometry.area;
            value.nonStructural = data.nsm * geometry.area;
            break;
        case ShellData::Kind::Laminate:
            value.structural = data.massPerArea * geometry.area;
            value.nonStructural = data.nsm * geometry.area;
            break;
        case ShellData::Kind::Volume:
            value.structural = data.density * geometry.volume;
            break;
        case ShellData::Kind::None:
            throw MassError(owner.text(context.model()) + " refers to property " + std::to_string(pid) +
                            ", which is not a shell, shear panel or plane property");
        }
        // Under MDLPRM OFFDEF ELMOFF, the solver's default, the mass stays on the plane of the grid points (an
        // offset leaves the mass properties as they are);
        // under LROFF and ELMZ it is on the reference plane, ZOFFS from the grid points along the normal (the element
        // normal: LROFF's normals at the grid points are not computed; MassCalculator::offsetMassOf). The Z0 of a PCOMP
        // or PCOMPG moves the plies, not the mass, whatever OFFDEF: the solver uses ELMOFF for Z0 whatever MDLPRM says
        // (format reference MDLPRM OFFDEF note, Z0 does not change the reference plane), and its grid
        // point weight generator keeps the mass of a Z0 laminate on the grid point plane (the solver 2018.2, a public
        // wing model with Z0 = 0).
        const double height = offsetOf(table, row);
        if (context.massOnOffsets() && height != 0.0 && isDefined(geometry.normal))
            value.center = geometry.center + height * geometry.normal;
    }
};

// AxisymTria6: the material is on the element.
struct AxisymTria6Shape
{
    static constexpr std::uint8_t QUANTITIES = bit(Quantity::Center) | bit(Quantity::Volume);

    static void evaluate(const model::AxisymTria6Table& table, std::uint32_t row, MassContext& context,
                         const ElementValues& geometry, MassValue& value)
    {
        const model::AxisymTria6Card card = table.card(row);
        value.material = card.matId;
        value.center = geometry.center;
        value.structural =
            context.density(card.matId, Owner{model::AxisymTria6Table::NAME.data(), card.id}) * geometry.volume;
    }
};

} // namespace

void registerShellMass(MassRegistry& registry)
{
    registry.add<model::ShellTria3Table, ShellShape<model::ShellTria3Table>>();
    registry.add<model::ShellTria6Table, ShellShape<model::ShellTria6Table>>();
    registry.add<model::ShellTria3DrillingTable, ShellShape<model::ShellTria3DrillingTable>>();
    registry.add<model::ShellQuad4Table, ShellShape<model::ShellQuad4Table>>();
    registry.add<model::ShellQuad8Table, ShellShape<model::ShellQuad8Table>>();
    registry.add<model::ShellQuad4DrillingTable, ShellShape<model::ShellQuad4DrillingTable>>();
    registry.add<model::PlaneStrainQuadTable, ShellShape<model::PlaneStrainQuadTable>>();
    registry.add<model::ShearPanel4Table, ShellShape<model::ShearPanel4Table>>();
    registry.add<model::PlaneStrainTria3Table, ShellShape<model::PlaneStrainTria3Table>>();
    registry.add<model::PlaneStrainQuad4Table, ShellShape<model::PlaneStrainQuad4Table>>();
    registry.add<model::PlaneStrainTria6Table, ShellShape<model::PlaneStrainTria6Table>>();
    registry.add<model::PlaneStrainQuad8Table, ShellShape<model::PlaneStrainQuad8Table>>();
    registry.add<model::PlaneStressTria3Table, ShellShape<model::PlaneStressTria3Table>>();
    registry.add<model::PlaneStressQuad4Table, ShellShape<model::PlaneStressQuad4Table>>();
    registry.add<model::PlaneStressTria6Table, ShellShape<model::PlaneStressTria6Table>>();
    registry.add<model::PlaneStressQuad8Table, ShellShape<model::PlaneStressQuad8Table>>();
    registry.add<model::AxisymQuadTable, ShellShape<model::AxisymQuadTable>>();
    registry.add<model::AxisymQuad4Table, ShellShape<model::AxisymQuad4Table>>();
    registry.add<model::AxisymQuad8Table, ShellShape<model::AxisymQuad8Table>>();
    registry.add<model::AxisymTriaTable, ShellShape<model::AxisymTriaTable>>();
    registry.add<model::AxisymTria6Table, AxisymTria6Shape>();
}

} // namespace oor::analysis::detail
