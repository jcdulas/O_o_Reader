#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One term of a LOADCLID: a load that the Case Control LOAD or CLOAD command of a part gave a number (LIDi), the part
// that holds it and the factor it is multiplied by.
struct LoadCaseIdLabelItem
{
    double scale = 0.0; // Si, not zero
    base::EntityId loadCombinationId =
        0;                // LIDi: the number given during GENERATE by a LOAD or CLOAD command of the part
    std::string partName; // PARTNAMEi: up to 64 characters

    [[nodiscard]] bool operator==(const LoadCaseIdLabelItem&) const noexcept = default;
};

// One LOADCLID card (PAA, the COMBINE and SOLVE steps of the partitioned analysis): a loading combination that adds up
// loads of the parts, found by the load ID that each SUBCASE of a part was generated with.
//
// Card layout: LOADCLID  LID    S                                         fields 1 and 2
//                       S1     LID1                                      fields 9 and 10, one item to every two lines
//                       PARTNAME1                                        fields 17 to 24, the name over the whole line
//                       S2     LID2     ...                              fields 25 and 26
//                       PARTNAME2                                        fields 33 to 40, and so on
// LID is the load set that the Case Control LOAD command selects (IdSpace::Group, category Load: the loads of other
// cards share the number). S is the overall scale factor, as the S of a LOAD card, and Si scales one term; both are
// real numbers other than zero and have no default. LIDi is a number of the parts (nothing in this model defines it),
// so it is a plain ID that no reference names; PARTNAMEi is a name of up to 64 characters, with no blank (the name of a
// part of the PAA run, kept as written: case counts). The format reference says all the loads with the same LID are
// combined, so two cards of the same LID add up (it is one number space with the other loads). A card has at least one
// item.
struct LoadCaseIdLabelCard
{
    base::EntityId id = 0;
    double scale = 0.0; // S
    std::vector<LoadCaseIdLabelItem> items;

    [[nodiscard]] bool operator==(const LoadCaseIdLabelCard&) const noexcept = default;
};

} // namespace oor::model
