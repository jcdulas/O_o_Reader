#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MATTG card: the temperature dependence of the properties of a MATG (a gasket of SOL 400). Each field holds the ID
// of a TABLEMi (0 when blank: the property does not depend on the temperature) that gives the variation of the property
// of the MATG with the same MID.
//
// Card layout:
//   MATTG  MID     IDYM    IDVM    IDDM    IDLD    IDU1    IDU2    IDU3
//          IDU4    IDU5    IDU6    IDU7    IDU8    IDU9    IDU10   IDYPR
//          IDEPL   IDGPL   IDGAP
// IDYM, IDVM and IDDM are the Young's modulus, Poisson's ratio and mass density of the membrane, IDLD the loading curve
// and IDU1 to IDU10 the unloading curves, IDYPR, IDEPL, IDGPL and IDGAP the yield pressure, tensile modulus, transverse
// shear modulus and initial gap of the out-of-plane behaviour.
struct GasketMatTemperatureCard
{
    static constexpr std::size_t UNLOADING_TABLES = 10;

    base::EntityId id = 0; // the MATG that the card extends
    base::EntityId membraneYoungsModulusTableId = 0;
    base::EntityId membranePoissonRatioTableId = 0;
    base::EntityId membraneDensityTableId = 0;
    base::EntityId loadingTableId = 0;
    std::array<base::EntityId, UNLOADING_TABLES> unloadingTableIds{};
    base::EntityId yieldPressureTableId = 0;
    base::EntityId tensileModulusTableId = 0;
    base::EntityId transverseShearModulusTableId = 0;
    base::EntityId initialGapTableId = 0;
};

} // namespace oor::model
