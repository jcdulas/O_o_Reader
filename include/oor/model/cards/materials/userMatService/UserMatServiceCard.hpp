#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One MATUDS card: the user defined service or subroutine of a material of SOL 400 (the FMS statement CONNECT SERVICE
// names the group).
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MATUDS  MID    MTYPE  GROUP  UNAME                                              fields 1 to 4
// then up to three blocks of data, each starting on a line of its own with a keyword in the first field of the line and
// seven values to a line (the next lines of a block leave the first field blank):
//   "INT"   IDATA1 IDATA2 ... IDATAn                                                 integers
//   "REAL"  RDATA1 RDATA2 ... RDATAn                                                 reals
//   "CHAR"  CDATA1 CDATA2 ... CDATAn                                                 words
// The blocks may come in any order and each at most once; the card is written with INT, REAL and CHAR in that order.
// MID is the ID of the material the subroutine serves (a reference to a material), MTYPE the name of the card of that
// material (MAT1, MATEP, MATHE, MATORT, MATUSR, MATVE, MATVP, MATF or MCOHE), GROUP the group name of the CONNECT
// SERVICE statement and UNAME the subroutine name (8 characters at most). A CDATAi is never the word INT, REAL or CHAR.
struct UserMatServiceCard
{
    base::EntityId id = 0; // the material that the card serves
    std::string materialCardType;
    std::string groupName;
    std::string userRoutineName;
    std::vector<int> integers;
    std::vector<double> reals;
    std::vector<std::string> words;
};

} // namespace oor::model
