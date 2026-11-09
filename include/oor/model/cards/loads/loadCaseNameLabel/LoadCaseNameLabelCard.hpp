#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One term of a LOADCNAM: the name of a load of a part (the LOADNAME that the Case Control of the part gave it), the
// part that holds it and the factor it is multiplied by.
struct LoadCaseNameLabelItem
{
    double scale = 0.0;   // Si, not zero
    std::string loadName; // LOADNAMEi: the name of the load in the part, up to 64 characters
    std::string partName; // PARTNAMEi: the name of the part, up to 64 characters

    [[nodiscard]] bool operator==(const LoadCaseNameLabelItem&) const noexcept = default;
};

// One LOADCNAM card (PAA, the COMBINE and SOLVE steps of the partitioned analysis): a loading combination that adds up
// loads of the parts, found by the LOADNAME of the Case Control of each part.
//
// Card layout: LOADCNAM  LID    S                                         fields 1 and 2
//                       S1                                               fields 9 (the rest of the line is blank)
//                       LOADNAME1                                        fields 17 to 24, the name over the whole line
//                       PARTNAME1                                        fields 25 to 32, the name over the whole line
//                       S2       ...                                     fields 33, one item to every three lines
//                       LOADNAME2                                        fields 41 to 48, and so on
// LID is the load set that the Case Control LOAD command selects (IdSpace::Group, category Load: the loads of other
// cards share the number). S is the overall scale factor, as the S of a LOAD card, and Si scales one term; both are
// real numbers other than zero and have no default. LOADNAMEi and PARTNAMEi are names of up to 64 characters with no
// blank, the names that the parts were given (nothing in this model defines them, so no reference names them); they are
// kept as written: case counts. The format reference says all the loads with the same LID are combined, so two cards of
// the same LID add up. A card has at least one item.
struct LoadCaseNameLabelCard
{
    base::EntityId id = 0;
    double scale = 0.0; // S
    std::vector<LoadCaseNameLabelItem> items;

    [[nodiscard]] bool operator==(const LoadCaseNameLabelCard&) const noexcept = default;
};

} // namespace oor::model
