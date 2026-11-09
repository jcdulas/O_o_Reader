#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One term of a LOADCSUB: the number of a subcase of a part, the part that holds it and the factor it is multiplied by.
struct LoadCaseSubcaseLabelItem
{
    double scale = 0.0;         // Si, not zero
    base::EntityId subcase = 0; // SUBi: the subcase number given during the GENERATE or COMBINE run
    std::string partName;       // PARTNAMEi: up to 64 characters

    [[nodiscard]] bool operator==(const LoadCaseSubcaseLabelItem&) const noexcept = default;
};

// One LOADCSUB card (PAA, the COMBINE and SOLVE steps of the partitioned analysis): a loading combination that adds up
// loads of the parts, found by the number of the SUBCASE of the part that carries them.
//
// Card layout: LOADCSUB  LID    S                                         fields 1 and 2
//                       S1     SUB1                                      fields 9 and 10, one item to every two lines
//                       PARTNAME1                                        fields 17 to 24, the name over the whole line
//                       S2     SUB2     ...                              fields 25 and 26
//                       PARTNAME2                                        fields 33 to 40, and so on
// LID is the load set that the Case Control LOAD command selects (IdSpace::Group, category Load: the loads of other
// cards share the number). S is the overall scale factor, as the S of a LOAD card, and Si scales one term; both are
// real numbers other than zero and have no default. SUBi is a subcase number of a part (nothing in this model defines
// it), so it is a plain positive integer that no reference names; PARTNAMEi is the name of a part, up to 64 characters
// with no blank (kept as written: case counts). The format reference says all the loads with the same LID are combined,
// so two cards of the same LID add up. A card has at least one item. The format prints the same description of the card
// as the LOADCNAM
// (load names of the Case Control), but its field list says SUBi is a subcase number: the card follows the list.
struct LoadCaseSubcaseLabelCard
{
    base::EntityId id = 0;
    double scale = 0.0; // S
    std::vector<LoadCaseSubcaseLabelItem> items;

    [[nodiscard]] bool operator==(const LoadCaseSubcaseLabelCard&) const noexcept = default;
};

} // namespace oor::model
