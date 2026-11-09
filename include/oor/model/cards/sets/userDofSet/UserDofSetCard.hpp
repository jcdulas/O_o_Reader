#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One USET card: degrees of freedom of a set that the user names.
//
// Card layout: USET  SNAME  ID1  C1  ID2  C2  ID3  C3
// and, for more than three points, four pairs ID C on each continuation line. SNAME is one to four characters (or
// "ZERO" followed by a set name: the degrees of freedom are then taken out of that set), kept in upper case. IDi is a
// grid or scalar point, Ci the digits of its components as one number (0 for a scalar point). The card has no ID.
struct UserDofSetCard
{
    std::string setName;
    std::vector<DofPair> dofs;
};

} // namespace oor::model
