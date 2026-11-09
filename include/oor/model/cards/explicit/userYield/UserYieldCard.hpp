#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <string_view>

namespace oor::model
{

// The subroutine a YLDUDS names when its UNAME is blank: the standard user defined yield model.
inline constexpr std::string_view USER_YIELD_DEFAULT_ROUTINE_NAME = "ExternalYield";

// One YLDUDS card (SOL 700): a yield model that a user subroutine computes. The FMS statement CONNECT SERVICE names the
// group of the routines; the card gives the group and the routine. The model serves Euler elements with shear strength
// and is named by a MATDEUL (the format reference adds a MAT1). The yield strength may depend on the damage a FAILJC or
// a FAILUDS (with EXFAIL2) computes, and on the VOLPLS and SOFTE element variables.
//
// Card layout: YLDUDS  YLD  GROUP  UNAME            fields 1 to 3
// YLD is the number of the yield model (the field the other YLDxxx cards call YID). GROUP is required and UNAME is the
// name of the subroutine (EXYLD when blank, the one routine the format reference lists); both are names of eight
// characters at most (checkYieldUserWord). The card does not match UNAME against the list: a name the solver does not
// know is its error.
struct UserYieldCard
{
    base::EntityId id = 0; // unique among the yield models
    std::string groupName; // the group of the CONNECT SERVICE statement
    std::string userRoutineName{USER_YIELD_DEFAULT_ROUTINE_NAME};
};

} // namespace oor::model
