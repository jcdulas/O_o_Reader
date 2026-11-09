#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// The values of the MODEL field of an MCOHE: the law of the cohesive zone. A positive value is a model of the nonlinear
// solution (SOL 400), -1 is the user subroutine UCOHES (a MATUDS card must go with it) and -2 is the linear material
// that the linear solution sequences use.
inline constexpr int COHESIVE_MAT_MODEL_BILINEAR = 1;
inline constexpr int COHESIVE_MAT_MODEL_EXPONENTIAL = 2;
inline constexpr int COHESIVE_MAT_MODEL_LINEAR_EXPONENTIAL = 3;
inline constexpr int COHESIVE_MAT_MODEL_USER = -1;
inline constexpr int COHESIVE_MAT_MODEL_LINEAR = -2;

// One MCOHE card: the material of the interface cohesive zone elements that simulate the onset and the growth of a
// delamination. SOL 400 uses it with a fully nonlinear element, and the linear solution sequences when MODEL is -2.
//
// Card layout (fields numbered over the lines, eight to a line):
//   MCOHE  MID    MODEL  (blank) TID    (four blank fields)                  fields 1 to 8
//          COHE   CRTOD  MAXOD   SNSR   EXP    VED    RRRD   SFC             fields 9 to 16
//          SNER   (blank) ET     EN                                          fields 17 to 20
// Fields 3, 5 to 8 and 18 are blank in the grid of the format reference, and the card refuses a value there.
//
// MID is a material ID: the format reference says it must differ from the ID of every other material card (MAT1, MAT2,
// MAT3, MAT8, MATDIGI, MATG, MATHE, MATHP, MATNLE, MATORT, MATPE1, MATSMA, MATUSR, MIXTURE, MATD*), so the card is a
// primary material in the shared material space (category Material, IdSpace::Material), although the format reference
// files it with the elements. MODEL is 1 (bilinear), 2 (exponential), 3 (linear-exponential), -1 (user subroutine) or
// -2 (linear), 1 when blank: the description reads an integer of -2 or more but names only these five
// values, and the card accepts those. TID (0 when blank) is the number of the TABLES1 or TABLEST that gives the
// cohesive energy against the temperature (a stress table). COHE (the cohesive energy), CRTOD (the critical opening
// distance) and MAXOD (the maximum opening displacement, bilinear model) are not negative and have no default: a blank
// one is `std::nullopt`. SNSR, EXP and SNER (1.0) are greater than zero, SFC (1.0), VED (0.0), RRRD (0.0), ET (0.0) and
// EN (0.0) are not negative.
//
// Which fields a model uses is not enforced: EXP serves the linear-exponential model, MAXOD the bilinear one, RRRD only
// a VED that is not zero, ET and EN the linear material only, and an EXP given with another
// model is not an error. An unused field is a value the solver ignores.
struct CohesiveMatCard
{
    base::EntityId id = 0;
    int cohesiveModel = COHESIVE_MAT_MODEL_BILINEAR;
    base::EntityId tableId = 0;             // a TABLES1 or TABLEST of cohesive energy against temperature, 0 for none
    std::optional<double> cohesiveEnergy;   // cohesive energy
    std::optional<double> criticalOpening;  // critical opening distance
    std::optional<double> maxOpening;       // maximum opening displacement
    double shearNormalStressRatio = 1.0;    // shear normal stress ratio, greater than zero
    double exponent = 1.0;                  // exponential decay factor, greater than zero
    double viscousDissipation = 0.0;        // factor of viscous energy dissipation
    double referenceDisplacementRate = 0.0; // reference rate of relative displacement, 0.0 computes it
    double compressionStiffening = 1.0;     // stiffening factor in compression
    double shearNormalEnergyRatio = 1.0;    // shear normal energy ratio, greater than zero
    double inPlaneStiffness = 0.0;          // stiffness in the plane of the element (linear solutions)
    double normalStiffness = 0.0;           // stiffness normal to the plane of the element (linear solutions)
};

} // namespace oor::model
