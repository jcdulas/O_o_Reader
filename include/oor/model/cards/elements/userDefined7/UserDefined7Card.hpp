#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/elements/DummyFieldList.hpp"

#include <vector>

namespace oor::model
{

// One CDUM7 card: the connection of a user dummy element of number 7, an element whose stiffness, mass and output are
// computed by routines the user links into the solver. The attributes of the element (how many grid points and
// additional fields it has) are on the ADUM7 card.
//
// Card layout: CDUM7  EID  PID  G1  G2  G3  G4  -etc.-          fields 1 to 8, then the continuation lines
//                    A1   A2   -etc.-
// The grid points G1 to GN come first and the additional fields A1, A2, ... after them. A card does not say where one
// ends and the other begins (the NG of the ADUM7 card does), so the card keeps what follows the PID as one list of
// fields in the positions of the card: each an integer, a real or blank, exactly as the deck gave it. A grid point that
// is left out of the middle of the connection is a blank field and the fields after it keep their positions (a blank
// grid point still counts in the connection, so the additional fields start where they would if it were given).
//
//   EID  the element ID: 0 < EID < 100,000,000, in the space of all the elements
//   PID  the PDUM7 property, or, for an element that needs no property, a material ID. The reader cannot
//        tell the two apart, so PID is a plain ID: it is neither looked up nor renumbered with the properties.
//   Gi   the grid points: integers greater than 0, which the format reference asks to differ
//   Ai   integers or reals; the user's routines define them and check them
//
// What the library takes for a grid point is the run of positive integers at the start of the list (it ends at the
// first blank, real or integer that is not positive): `nodes()`. An integer first additional field that follows the
// last grid point with no blank or real between them is taken for a grid point as well, unless the ADUM7 card says
// otherwise; `fields` is always exact, and a later step that has the ADUM7 card at hand can split it at NG. For the
// same reason the card cannot check that the grid points differ: it only requires that G1 is there.
struct UserDefined7Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    DummyFieldList fields; // G1 to GN, then A1, A2, ...: integer, real or blank; never ends with a blank

    // The grid points the model sees: the leading run of positive integers of `fields`.
    [[nodiscard]] std::vector<base::EntityId> nodes() const { return leadingGrids(fields); }
};

} // namespace oor::model
