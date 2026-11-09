#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <string_view>

namespace oor::model
{

// The subroutine of the standard user-defined shear model, which a blank UNAME means.
inline constexpr std::string_view USER_SHEAR_MODEL_DEFAULT_ROUTINE_NAME = "ExternalShear";

// One SHRUDS card (SOL 700): a shear model whose modulus is computed by a user subroutine. A MATDEUL names it in its
// SID field.
//
// Card layout: SHRUDS  SID  GROUP  UNAME            fields 1 to 3
// GROUP is the name that a CONNECT SERVICE statement of the FMS section gives to the library of user routines
// (required, no default) and UNAME the name of the routine, EXSHR when blank. The format says that only EXSHR
// can be used; the card keeps any name of at most eight characters and leaves that rule to a check of the model,
// because the library is the user's. Both words are kept as written, in the case the deck gives them (a group
// may be in lower case). The default UNAME is stored resolved and always written. The format describes SID as a
// porosity model number, which is a slip of the text: it is the number of a shear model, unique among the SHRxxx cards.
// No other entry of the format reference has this name.
struct UserShearModelCard
{
    base::EntityId id = 0; // unique among the shear models
    std::string groupName; // the name of the CONNECT SERVICE group, 1 to 8 characters
    std::string userRoutineName = std::string(USER_SHEAR_MODEL_DEFAULT_ROUTINE_NAME);
};

} // namespace oor::model
