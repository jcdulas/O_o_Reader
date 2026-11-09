#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// The subroutine of the standard user-defined equation of state, which a blank UNAME means.
inline constexpr std::string_view USER_STATE_EQUATION_DEFAULT_ROUTINE_NAME = "ExternalStateEquation";

// One EOSUDS card (SOL 700): an equation of state computed by a user subroutine. A MATDEUL names it in its EID field.
//
// Card layout: EOSUDS  EID  GROUP  UNAME  VISC            fields 1 to 4
// GROUP is the name that a CONNECT SERVICE statement of the FMS section gives to the library of user routines
// (required, no default) and UNAME the name of the routine, EXEOS when blank: the format lists that standard routine as
// the only name and nothing in the card refuses another one (the library is the user's), so the card keeps any name of
// at most eight characters. Both are kept as written, in the case the deck gives them. The default UNAME is stored
// resolved and always written. VISC is the dynamic viscosity; blank means no viscosity, and the positive range the
// format gives it is read as not negative, so that a zero (also no viscosity) is accepted. No other entry of the format
// reference has this name.
struct UserStateEquationCard
{
    base::EntityId id = 0; // unique among the equations of state
    std::string groupName; // the name of the CONNECT SERVICE group, 1 to 8 characters
    std::string userRoutineName = std::string(USER_STATE_EQUATION_DEFAULT_ROUTINE_NAME);
    std::optional<double> viscosity; // dynamic viscosity, not negative; blank is no viscosity
};

} // namespace oor::model
