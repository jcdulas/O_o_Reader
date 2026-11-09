#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One TIRE card (SOL 400): the selection of the external tire models that take part in a simulation. The
// Case Control command TIRE=ID selects the card (the command is text: not resolved).
//
// Card layout: TIRE  ID                                                                fields 1 of the first line
//                   "tireFileKeyword"  TNAME1  TNAME2  TNAME3  TNAME4  TNAME5  TNAME6  ...    fields 9 and on
// The tire-model word is the first field of the first continuation line, and the names of the tires follow it over as
// many fields and lines as needed (the format draws them seven and then eight to a line with as many as there are
// tires: the card reads every used field after the word, wherever it is, and writes the names one after the other).
// Each name is the one that the DTI entry of the tire library gives the tire model: one word of one to eight characters
// (a DTI name, which the format does not repeat). The ID is the number of the set (integer > 0, required) and the card
// lists all its tires, so it is unique among the TIRE cards. A card with no tire is refused: the format requires the
// word and the tires.
struct TireSelectionCard
{
    base::EntityId id = 0;
    std::vector<std::string> names; // TNAME1 to TNAMEn, as written

    [[nodiscard]] bool operator==(const TireSelectionCard&) const noexcept = default;
};

} // namespace oor::model
