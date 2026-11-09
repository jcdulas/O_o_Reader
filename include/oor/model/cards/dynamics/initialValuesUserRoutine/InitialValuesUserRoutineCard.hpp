#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace oor::model
{

// UNAME (field 3): the name of the user subroutine. The format says EXINIT is the only one that can be used, and a
// blank UNAME is EXINIT, so the card has one word.
enum class InitialValuesUserRoutineRoutine : std::uint8_t
{
    Exinit
};
inline constexpr std::array<std::string_view, 1> INITIAL_VALUES_ROUTINE_WORDS{"ExternalInitialValues"};

// COPT (field 5): what the variables the routine computes are applied to. A blank COPT is ELEMENT.
enum class InitialValuesUserRoutineTarget : std::uint8_t
{
    Element, // the initial variables go to Euler elements
    Grid     // the initial variables go to Lagrangian grid points
};
inline constexpr std::array<std::string_view, 2> INITIAL_VALUES_TARGET_WORDS{"Element", "Node"};

// One TICEUDS card (SOL 700): initial values of element or grid point variables that a user written subroutine
// computes. The FMS statement CONNECT SERVICE gives the group its name and the library of routines. As the TIC, it is
// selected by the IC command of Case Control (the format mentions a TIC command once, but every other initial
// condition is selected by IC, and the library files this card with them).
//
// Card layout: TICEUDS  SID  GROUP  UNAME  SETID  COPT                              fields 1 to 5
// SID is the set ID the card is filed under (the format calls it a unique output number: the library lets the cards of
// a set share it, as for every initial condition). GROUP is required and has no default; it is a name of one to eight
// characters (the field of the small format), kept in capitals as the GROUP of a FORCUDS is (a deck may write it in
// lower case). SETID is the SET1 of the elements or of the grid points, as COPT says (a reference to a set). No other
// field exists: the format draws blank cells after COPT and the reader refuses a value in them.
struct InitialValuesUserRoutineCard
{
    base::EntityId id = 0;
    std::string group;
    InitialValuesUserRoutineRoutine routineName = InitialValuesUserRoutineRoutine::Exinit;
    base::EntityId elementSetId = 0; // a SET1 of the elements or grid points
    InitialValuesUserRoutineTarget target = InitialValuesUserRoutineTarget::Element;
};

} // namespace oor::model
