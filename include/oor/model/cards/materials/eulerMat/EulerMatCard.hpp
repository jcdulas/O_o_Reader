#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// The bulk viscosity type of a MATDEUL card.
enum class EulerMatBulkType : std::uint8_t
{
    Standard, // the default: the standard model
    Enhanced  // the enhanced model
};

// One MATDEUL card: a general constitutive model for an Eulerian material (SOL 700): an equation of state, a shear
// model, a yield model, a failure model and a spall model.
//
// Card layout:
//   MATDEUL  MID    RHO    EID    SID    YID    FID    PID                        fields 1 to 7
//            BULKL  BULKQ  (blank) (blank) BULKTYP                                9 to 13
// RHO is the density (greater than zero) and EID the ID of an equation of state card (EOSDEF, EOSGAM, EOSIG, EOSJWL,
// EOSMG, EOSNA, EOSPOL or EOSUDS: the category EquationOfState). SID, YID, FID and PID (0 when blank) name a shear
// model (SHREL, SHRPOL, SHRUDS: ShearModel), a yield model (YLDHY, YLDJC, YLDMSS, YLDPOL, YLDRPL, YLDSG, YLDTM, YLDUDS,
// YLDVM, YLDZA: YieldModel; blank is the hydrodynamic one), a failure model (FAILJC, FAILMPS, FAILUDS: FailureModel)
// and a spall model (PMINC: SpallModel). Those cards are typed, so the five IDs are references (the YLDMC of the
// format reference, a Mohr-Coulomb model, is not typed yet: a YID that names it is reported as unresolved until it is).
// BULKL (0.0) and BULKQ (1.0) are the linear and quadratic bulk viscosity coefficients. The material ID is in the space
// every material type shares.
struct EulerMatCard
{
    base::EntityId id = 0;
    double density = 0.0;
    base::EntityId equationOfStateId = 0;
    base::EntityId shearModelId = 0;
    base::EntityId yieldModelId = 0;
    base::EntityId failureModelId = 0;
    base::EntityId spallModelId = 0;
    double linearBulkViscosity = 0.0;
    double quadraticBulkViscosity = 1.0;
    EulerMatBulkType bulkViscosityKind = EulerMatBulkType::Standard;
};

} // namespace oor::model
