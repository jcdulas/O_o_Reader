#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One ABSTRCT card: entities of one kind added to a named group of the meta data written to the results files (it has
// no effect on the solution).
//
// Card layout: ABSTRCT  ABSTNM  ABSTID  NASTNM  NASTID1  NASTID2  NASTID3  -etc.-
// ABSTNM names the group (up to eight characters) and ABSTID is its ID; the group is made of the entities of several
// cards that share the ABSTID (a common deck: three cards, for a CBEAM, a CBUSH and an RBE3, all of the group PILLAR,
// ID 50). NASTNM is the name of the kind of entity (up to eight characters: "CBEAM", "SPC", ...) and the NASTIDi are
// the IDs of entities of that kind, or of sets of them.
struct MetadataGroupCard
{
    std::string groupName;  // ABSTNM
    base::EntityId id = 0;  // ABSTID
    std::string entityName; // NASTNM
    std::vector<base::EntityId> entities;
};

} // namespace oor::model
