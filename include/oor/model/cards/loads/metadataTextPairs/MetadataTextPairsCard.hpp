#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// A string name and value pair of a meta data group.
struct MetadataTextPairsPair
{
    std::string name;  // NAMEi: up to 8 characters, as written
    std::string value; // VALi: up to 56 characters, blanks inside

    [[nodiscard]] bool operator==(const MetadataTextPairsPair&) const noexcept = default;
};

// One ABSNMVS card: string name and value pairs associated with a meta data group (the ABSTRCT cards of its ABSTID). It
// has no effect on the solution, and the pairs may be in any order.
//
// Card layout: ABSNMVS  ABSTID
//                      NAME1  VAL1 (fields 3 to 9: up to 56 characters)
//                      NAME2  VAL2
//                      etc.
// In this library's numbering (field 1 is ABSTID) the fields 2 to 8 of the first line are blank and each pair is one
// line from field 9 on: NAME in the first field and the value over the next seven (`FieldReader::spanText`, which gives
// the text as the line has it, blanks inside; the writer cuts it in pieces of eight characters in the small format and
// writes it whole in the others). A deck may have `ABSNMVS 1` and, on the continuation line, `ABSNAME Constraint 1`:
// the value "Constraint 1" crosses two fields. A value may hold any character A-Z, digits, underscore,
// dash and blanks; the card refuses the characters no line can carry (a
// '$', a comma, a tab) and a value that begins or ends with a blank. A pair with a name and a blank value is kept (the
// value is the empty text); at least one pair is required. ABSTID is the ID of the group: the card is one of the cards
// that share it (IdSpace::Group, category Other) and the ID is a reference to the ABSTRCT cards of the group
// (`MetadataGroup`), which a renumbering of the groups follows. The names are meta data and kept as written.
struct MetadataTextPairsCard
{
    base::EntityId metadataGroupId = 0;
    std::vector<MetadataTextPairsPair> pairs;

    [[nodiscard]] bool operator==(const MetadataTextPairsCard&) const noexcept = default;
};

} // namespace oor::model
