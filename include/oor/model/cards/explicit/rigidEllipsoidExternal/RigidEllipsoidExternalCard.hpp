#pragma once

#include <string>

namespace oor::model
{

// One RELEX card (SOL 700): the external definition of a rigid ellipsoid, whose properties and motion come from another
// program. The only program the format reference names is an external occupant model, and then the NAME is the name
// of one of its segments. The ellipsoid is what the contact cards (METHOD=RELLIPS of a BCTABLE or BCONPRG, a BCELIPS
// list of names) and the output requests refer to by name.
//
// Card layout: RELEX  NAME  PROG  etc      fields 1 and 2 (the "etc" is in the drawing of the format, in field 7, and
// nothing says what it holds: the card refuses any field after PROG). NAME is a word of up to eight characters, PROG
// the name of the external program; both are required. The card has no number: it is found by its NAME, as a
// monitor point is.
struct RigidEllipsoidExternalCard
{
    std::string name;        // the ellipsoid, upper case
    std::string programName; // the external program, upper case
};

} // namespace oor::model
