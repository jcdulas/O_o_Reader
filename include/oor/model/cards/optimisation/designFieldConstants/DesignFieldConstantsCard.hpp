#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One constant of a DTABLE2: a label and the place in a card of the model the value is taken from.
struct DesignFieldConstantsEntry
{
    std::string label;    // LABLi, up to 8 characters
    std::string cardName; // PNAMEi: the property, material or connectivity card, upper case ("PSHELL", "MAT1", "CBAR")
    base::EntityId targetId = 0; // PIDi: its ID
    std::string field; // FNAMEi: the name of the field, as the format reference spells it on the card ("T", "E", "X1")

    [[nodiscard]] bool operator==(const DesignFieldConstantsEntry&) const noexcept = default;
};

// One DTABLE2 card: constants that are the value of a field of a property, a material or a connection of the analysis
// model (taken before the design variables change it).
//
// Card layout: DTABLE2  LABL1  PNAME1  PID1  FNAME1  LABL2  PNAME2  PID2  FNAME2     fields 1 to 8
//                      LABL3  PNAME3  PID3  FNAME3  ...                              fields 9 to 16, and so on
// An entry whose four fields are blank is skipped. The label is unique over the DTABLE and DTABLE2 cards.
struct DesignFieldConstantsCard
{
    std::vector<DesignFieldConstantsEntry> entries; // at least one
};

} // namespace oor::model
