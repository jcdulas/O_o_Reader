#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One SEUSET card: degrees of freedom of a superelement in a set the user names.
//
// Card layout: SEUSET  SEID  SNAME  ID1  C1  ID2  C2  ID3  C3
// and, for more than three points, the pairs ID C on the continuation lines, four to a line (field 9 on). SEID is the
// superelement, and the points are those of that superelement (its part, when the deck has one). SNAME is one to four
// characters, the name of one of the sets of the Degree-of-Freedom Sets table or the new name a DEFUSET gives it (the
// solution sequences 0 to 200 should use U1 to U6), or the word ZERO followed by a set name, which takes the degrees of
// freedom out of that set; it is kept in upper case. IDi is a grid or scalar point, Ci the digits of its
// components as one number (0 for a scalar point). The card has no ID of its own.
struct SuperelementUserDofSetCard
{
    base::EntityId superelementId = 0;
    std::string setName;
    std::vector<DofPair> dofs;

    [[nodiscard]] bool operator==(const SuperelementUserDofSetCard&) const noexcept = default;
};

} // namespace oor::model
