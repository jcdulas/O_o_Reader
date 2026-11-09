#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// One PCONV card: the free convection property of the surface elements a CONV names.
//
// Card layout, FTYPE 0 to 2:
//   PCONV  PCONID  MID  FORM  EXPF  FTYPE  TID
//          CHLEN   GIDIN  CE  E1  E2  E3
// Alternate layout, FTYPE 3 (the coefficient is given for each of up to eight ambient points):
//   PCONV  PCONID  MID  FORM  EXPF  3  H1  H2  H3
//          H4  H5  H6  H7  H8
// A card has one layout or the other, told by FTYPE (field 5). FORM is 0 (blank), 1, 10, 11, 20 or 21 and EXPF is 0.0
// when blank. MID is a MAT4 (the coefficient for FTYPE 0, the conductivity for FTYPE 2); it may be blank for FTYPE 1
// and
// 3. TID is a TABLEHT of the two-variable function of FTYPE 1 and 2. A blank H2 to H8 is H1 (format reference): stored
// resolved and always written; `h` is all zeros for the other FTYPEs.
struct FreeConvectionPropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0; // 0: blank
    int matrixForm = 0;
    double convectionExponent = 0.0;
    int formulaKind = 0;
    base::EntityId tableId = 0; // 0: blank
    std::optional<double> characteristicLength;
    base::EntityId inletNode = 0; // 0: blank
    base::EntityId directionFrameId = 0;
    std::optional<double> direction1;
    std::optional<double> direction2;
    std::optional<double> direction3;
    std::array<double, 8> convectionCoefficient = {};
};

} // namespace oor::model
