#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MATTORT card: the temperature dependent properties of a MATORT (the 3D orthotropic material of SOL 400). Each
// field holds the ID of a TABLEMi (0 when blank: the property does not depend on the temperature) that modifies the
// property of the same name on the MATORT with the same MID.
//
// Card layout:
//   MATTORT  MID     T(E1)   T(E2)   T(E3)   T(NU12)  T(NU23)  T(NU31)  (blank)
//            T(G12)  T(G23)  T(G31)  T(A1)   T(A2)    T(A3)
// There is no T(RHO): field 8 is blank, as it is on the MATORT card that has the density there.
struct OrthoMat3DNonlinearTemperatureCard
{
    base::EntityId id = 0; // the MATORT that the card extends
    base::EntityId youngsModulus1TableId = 0;
    base::EntityId youngsModulus2TableId = 0;
    base::EntityId youngsModulus3TableId = 0;
    base::EntityId poissonRatio12TableId = 0;
    base::EntityId poissonRatio23TableId = 0;
    base::EntityId poissonRatio31TableId = 0;
    base::EntityId shearModulus12TableId = 0;
    base::EntityId shearModulus23TableId = 0;
    base::EntityId shearModulus31TableId = 0;
    base::EntityId thermalExpansion1TableId = 0;
    base::EntityId thermalExpansion2TableId = 0;
    base::EntityId thermalExpansion3TableId = 0;
};

} // namespace oor::model
