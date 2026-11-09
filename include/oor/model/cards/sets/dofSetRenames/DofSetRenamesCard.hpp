#pragma once

#include <string>
#include <vector>

namespace oor::model
{

// One renaming of a degree-of-freedom set: the default name the solver gives it and the name the deck wants.
struct DofSetRenamesRename
{
    std::string oldName; // OLDi: one of the default set names (U1 to U6 are the ones the format reference recommends)
    std::string newName; // NEWi: the new name

    [[nodiscard]] bool operator==(const DofSetRenamesRename&) const noexcept = default;
};

// One DEFUSET card: new names for the degree-of-freedom sets.
//
// Card layout: DEFUSET  OLD1  NEW1  OLD2  NEW2  OLD3  NEW3  OLD4  NEW4
// From one to four pairs on a card, all in the first line; each name is one to four characters. The names are kept in
// upper case (the deck is case-insensitive). The format reference asks that the NEWi be unique among all the set names
// and that the OLDi be set names of its table of degree-of-freedom sets (never PA or PS, which the solver refuses): the
// card checks that its own NEWi are distinct, and leaves the rest, which depends on the table of sets, to the solver.
// The card has no ID (IdSpace::None); the format reference says it must be repeated in the Bulk Data of every restart.
struct DofSetRenamesCard
{
    std::vector<DofSetRenamesRename> renames;

    [[nodiscard]] bool operator==(const DofSetRenamesCard&) const noexcept = default;
};

} // namespace oor::model
