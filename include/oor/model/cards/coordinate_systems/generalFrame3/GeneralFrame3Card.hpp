#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <string>
#include <string_view>

namespace oor::model
{

// One CORD3G card: a general coordinate system whose three rotation angles are functions of the coordinates in the
// reference system.
//
// Card layout:
//   CORD3G  CID  METHOD  FORM  THETAID1  THETAID2  THETAID3  CIDREF
//
// METHOD is EulerAngles313 (Euler angles in the 313 sequence) or SpaceFixed321 (space-fixed rotations in the 321
// sequence); the card accepts EulerAngles or SpaceFixed followed by any three digits. FORM says what THETAIDi names:
// EquationForm for a DesignEquation, Table for a MultiDimensionalTable.
// The angles are not evaluated here.
struct GeneralFrame3Card
{
    static constexpr std::string_view DEFAULT_METHOD_NAME = "EulerAngles313";
    static constexpr std::string_view DEFAULT_MATRIX_FORM = "EquationForm";

    base::EntityId id = 0;
    std::string methodName{DEFAULT_METHOD_NAME};
    std::string matrixForm{DEFAULT_MATRIX_FORM};
    std::array<base::EntityId, 3> angleFunctionIds =
        {};                              // THETAID1..3: the DEQATN or TABLE3D entries that give the angles
    base::EntityId referenceFrameId = 0; // the coordinate system the orientation is measured from (0 = basic)
};

} // namespace oor::model
