#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// A real name and value pair of a meta data group.
struct MetadataRealPairsPair
{
    std::string name;   // NAMEi: up to 8 characters, as written
    double value = 0.0; // VALi

    [[nodiscard]] bool operator==(const MetadataRealPairsPair&) const noexcept = default;
};

// One ABSNMVR card: real name and value pairs associated with a meta data group (the ABSTRCT cards of its ABSTID). It
// has no effect on the solution.
//
// Card layout: ABSNMVR  ABSTID  (blank)  NAME1  VAL1  NAME2  VAL2  -etc.-
// In this library's numbering (field 1 is ABSTID) field 2 is blank and the pairs follow from field 3, two fields to a
// pair and going on over the continuation lines (the third pair is fields 7 and 8, the fourth fields 9 and 10 of the
// first continuation line). A pair whose two fields are blank is padding. A value written as an integer is the real
// that is whole. At least one pair is required. ABSTID is the ID of the group: the card is one of the cards that share
// it (IdSpace::Group, category Other) and the ID is a reference to the ABSTRCT cards of the group (`MetadataGroup`),
// which a renumbering of the groups follows. The names are meta data and kept as written.
struct MetadataRealPairsCard
{
    base::EntityId metadataGroupId = 0;
    std::vector<MetadataRealPairsPair> pairs;

    [[nodiscard]] bool operator==(const MetadataRealPairsCard&) const noexcept = default;
};

} // namespace oor::model
