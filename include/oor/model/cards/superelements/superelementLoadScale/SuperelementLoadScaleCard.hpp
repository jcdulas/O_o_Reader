#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// What an entry of an SELOAD uses to find a column of the load matrix of an external superelement (LBLTYPi). The words
// are the ones of the format reference, in the order of the enumerators; the reader also takes SUBCASE for LABEL and
// COLUMN for COLNUM, the words the descriptions of the format use, and the card writes the first word of each.
enum class SuperelementLoadScaleSelector : std::uint8_t
{
    Label,      // the string the LDLABEL Case Control command gave the column in the creation run
    SubcaseId,  // the number of the subcase of the column
    LoadId,     // the number of the load case of the column
    ColumnIndex // the number of the column (the only way for the export files of versions before 2021.4)
};

inline constexpr std::array<std::string_view, 4> SUPERELEMENT_LOAD_SCALE_SELECTOR_WORDS{"Label", "SubcaseId", "LoadId",
                                                                                        "ColumnNumber"};

// One entry of an SELOAD: a superelement, the scale of its load vector and the column of its load matrix.
struct SuperelementLoadScaleEntry
{
    base::EntityId superelementId = 0;
    double scale = 1.0;
    SuperelementLoadScaleSelector selector = SuperelementLoadScaleSelector::Label;
    std::string label;         // the column, when the selector is Label
    base::EntityId number = 0; // the column, for the other selectors

    [[nodiscard]] bool operator==(const SuperelementLoadScaleEntry&) const noexcept = default;
};

// One SELOAD card: selects and scales the loads of external superelements.
//
// Card layout: SELOAD  SID  S                                                         the first line
//                     SEID1  S1  LBLTYP1                                             an entry: a line
//                     label, or subcase, load or column number                       and the line that names the column
//                     SEID2  S2  LBLTYP2  ...                                        the next entry
// SID is the number the Case Control LOAD command names (the EXCITEID of a TLOADi or RLOADi in a dynamic analysis), so
// the card is a load set (IdSpace::Group, category Load). S scales every entry and each Si its superelement's load
// vector; the format reference gives no default and a blank scale is read as 1.0, which leaves the vector as it is. The
// two lines of an entry start at fields 9, 25, ... (the first of each continuation line): the first holds SEIDi, Si and
// LBLTYPi (blank for LABEL), the second the label (text over the eight fields, 64 characters at most, with the rules of
// a label of SELABEL), or the integer of the other selectors. The card of the format reference replaced the one of
// older versions (SELOAD LIDS0 SEID LIDSE, one line); that layout is not read, and the card is kept as text in a
// tolerant model.
struct SuperelementLoadScaleCard
{
    base::EntityId id = 0;
    double scale = 1.0;
    std::vector<SuperelementLoadScaleEntry> entries;

    [[nodiscard]] bool operator==(const SuperelementLoadScaleCard&) const noexcept = default;
};

} // namespace oor::model
