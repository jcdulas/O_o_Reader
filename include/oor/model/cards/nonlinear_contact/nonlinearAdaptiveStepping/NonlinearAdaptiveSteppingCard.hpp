#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSectionFields.hpp"

#include <vector>

namespace oor::model
{

// One NLADAPT card: the extra parameters of the automatic load or time stepping of SOL 400 (obsolete in the later
// versions of the format, where NLSTEP takes its place).
//
// Card layout: NLADAPT  ID
//   then keyword lines, in any order:
//     STEP   RSMALL  RBIG  TSMIN  TSMAX  NSMAX  NRECYC  SFACT / IDAMP  DAMP
//     CREEP  RAC  TCSTRN  TCSTRS  TCOFF
// (the values of a keyword are in the fields after it; the second line of STEP has no keyword).
// The ID is the ID of the NLPARM (or NLSTEP) the card extends.
struct NonlinearAdaptiveSteppingCard
{
    base::EntityId id = 0;
    std::vector<ContactNamedSection> sections; // the keyword lines with their fields
};

} // namespace oor::model
