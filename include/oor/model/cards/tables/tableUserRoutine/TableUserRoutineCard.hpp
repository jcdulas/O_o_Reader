#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <string_view>

namespace oor::model
{

// The subroutine a TABLUDS names when its UNAME is blank: the standard user defined function.
inline constexpr std::string_view TABLE_USER_ROUTINE_DEFAULT_ROUTINE = "ExternalFunction";

// One TABLUDS card (SOL 700): a function that a user subroutine computes where a table would give it, so that a card
// of the explicit solver can name it by number like a table. The FMS statement CONNECT SERVICE names the group of the
// routines; the card gives the group and the routine.
//
// Card layout: TABLUDS  TID  GROUP  UNAME            fields 1 to 3
// TID is the number of the table. The format says that tables and user defined functions belong to one group, so the
// number is unique among them: the card is in the space of the TABLEDi (IdSpace::DynamicTable), and a TABLUDS that has
// the number of a TABLED1 is a duplicate. GROUP is required and UNAME is the name of the subroutine (EXFUNC when blank,
// the one routine the format reference lists); both are names of eight characters at most (checkNameWord). The card
// does not match UNAME against the list: a name the solver does not know is the solver's error. The case of both is
// kept as written (the group of a common deck is in lower case).
struct TableUserRoutineCard
{
    base::EntityId id = 0; // unique among the dynamic tables
    std::string groupName; // the group of the CONNECT SERVICE statement
    std::string routineName{TABLE_USER_ROUTINE_DEFAULT_ROUTINE};
};

} // namespace oor::model
