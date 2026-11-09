#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/InitialVariables.hpp"

#include <vector>

namespace oor::model
{

// One TICEL card (SOL 700): the initial values of element variables of the elements of a SET1, at the start of the
// analysis. As the TIC, it is selected by the IC command of Case Control and the cards of a set share its SID. Elements
// that no TICEL names start from zero, except the density, which starts at the reference density. An Euler
// element that a TICEL and a TICEUL1 both name takes the TICEL.
//
// Card layout: TICEL  SID  SETID  NAME1  VALUE1  NAME2  VALUE2  NAME3  VALUE3        fields 1 to 8
//             then, on any number of continuation lines, pairs NAMEi VALUEi          fields 9 on
// The format draws the first two pairs and "-etc.-"; the pairs go on in the fields that follow (NAME3 and VALUE3 end
// the first line, the next pair opens the continuation line), and a blank field ends the list: the reader refuses a
// pair after a blank one instead of dropping it. SETID is the SET1 of the elements (a reference to a set). The names
// are those the format gives for the Eulerian regions (XVEL, YVEL, ZVEL, DENSITY, SIE, Q, DIV, VOID, FMAT, XMOM, YMOM,
// ZMOM) and, in this order, the radial velocity field (X-CENTER, Y-CENTER, Z-CENTER, R-VEL, DECAY), all numbers. A
// TICEL may name the variables of Lagrangian elements too, which the format does not list, so any name of up to eight
// characters is kept as written (in capitals); the order of the radial names is not checked.
struct InitialElemValuesCard
{
    base::EntityId id = 0;           // the set ID that IC selects, shared by the cards of the set
    base::EntityId elementSetId = 0; // a SET1 of the elements to initialize
    std::vector<InitialVariable> variables;
};

} // namespace oor::model
