#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <vector>

namespace oor::model
{

// One GENEL card: a general element, given by a stiffness [K] or flexibility [Z] matrix on a list of degrees of
// freedom, and optionally a second list of degrees of freedom {UD} with a matrix [S] that relates the two.
//
// Card layout (each part starts on a new line; the pairs and the values follow one another, eight fields to a line):
//   GENEL  EID       UI1  CI1  UI2  CI2  UI3  CI3         the {UI} list (grid or scalar point, component) from field 3
//          UI4  CI4  UI5  CI5  ...
//          "UD"      UD1  CD1  UD2  CD2  ...              optional: the {UD} list
//          "K"/"Z"   KZ11 KZ21 KZ31 ... KZ22 KZ32 ... KZ33 ...   the lower triangle of [K] or [Z], by columns
//          "S"       S11  S12  ...  S21  ...              optional: [S], by rows
//
// The values are kept as the card has them: `stiffnessMatrix` holds the lower triangle by columns from the diagonal (n
// (n + 1) / 2 values for n degrees of freedom in {UI}), `dependencyMatrix` the n x m values of [S] by rows (empty: no
// [S] on the card).
struct GeneralMatrixCard
{
    base::EntityId id = 0;
    // A degree of freedom is (grid point or scalar point ID, component); the component is 0 at a scalar point.
    std::vector<std::array<base::EntityId, 2>> independentDofs;
    std::vector<std::array<base::EntityId, 2>> dependentDofs;
    bool isFlexibility = false; // "Z" instead of "K"
    std::vector<double> stiffnessMatrix;
    std::vector<double> dependencyMatrix;
};

} // namespace oor::model
