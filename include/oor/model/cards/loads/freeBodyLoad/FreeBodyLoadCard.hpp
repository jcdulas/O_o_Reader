#pragma once

#include <string>

namespace oor::model
{

// One FBODYLD card: an equilibrated free-body applied load case. It gives the load case a name and says which
// subsystem (an FBODYSB) the free-body loads are taken on.
//
// Card layout: FBODYLD  NAMEL  FBODYSB
//                      .      LABEL (the second line, fields 2 to 9: up to 64 characters)
// In this library's numbering (field 1 is NAMEL): FBODYSB is field 2 and the LABEL is fields 9 to 16, the second line.
// Fields 3 to 8 of the first line are not used and must be blank.
//
// A load case is named, not numbered (IdSpace::None, category Other): the name is the key of the card
// (`model.fbodyld["WINGLD"]`), is unique among the FBODYLD cards (a repeat is reported when the deck is
// read) and is what the Case Control names. The FBODYSB field names an FBODYSB card by its NAMES. A name is not an ID:
// both names are kept as the deck wrote them and are matched without regard to the case, and no reference resolves
// them (the check `load.free-body-subsystem` looks the FBODYSB up). Each name is one to sixteen characters (the format
// reference gives no length: see FreeBodySupport.hpp).
struct FreeBodyLoadCard
{
    std::string name;          // the name of the load case
    std::string subsystemName; // the NAMES of the FBODYSB card that defines the subsystem
    std::string label;         // up to 64 characters, may be empty

    [[nodiscard]] bool operator==(const FreeBodyLoadCard&) const noexcept = default;
};

} // namespace oor::model
