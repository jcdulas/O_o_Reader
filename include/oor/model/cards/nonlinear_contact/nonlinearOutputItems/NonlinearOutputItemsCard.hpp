#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One NLOUT card: the additional nonlinear output quantities of SOL 400 (selected by the NLOUT keyword of the NLSTRESS
// Case Control command).
//
// Card layout: NLOUT  ID  AIO1  AIO2  AIO3  AIO4  AIO5  AIO6  AIO7
//                        AIO8  AIO9  -etc.-
//                        ESV   SV2   SV3   -etc.-
// The output requests are words (CCASTRSS, CTOTSTRN, TOTTEMP, FAILND1, ...), in any order and any place: the fields
// between them may be blank. After the keyword ESV come the names of the user state variables to output. The words of
// the format reference differ from one release to the next and an item that the element cannot give is ignored, so the
// card keeps any name.
struct NonlinearOutputItemsCard
{
    base::EntityId id = 0;            // the NLOUT keyword of NLSTRESS names it
    std::vector<std::string> outputs; // AIO1, AIO2, ... in upper case
    std::vector<std::string>
        stateVariables; // SV names after ESV, in upper case (the keyword is written when there are some)
};

} // namespace oor::model
