#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One MDLABEL card: the label a module carries in the headings of the results output.
//
// Card layout: MDLABEL  MID  LABEL                                         fields 1 and 2 to 8
// MID is the module (zero or more, 0 when blank: the main Bulk Data). LABEL is text of up to 56 characters, written
// over the seven fields after the MID: in the small format the characters of the line from the first field of the label
// to the last are the label, blanks between the pieces included, so it reads back as it was drawn; the large and free
// formats hold it whole in one field. It begins and ends with a character that is not a blank and holds no '$', comma
// or tab, because the line would not read back (a common deck, `LEFT REAR FENDER, MODEL XYZ2000`, has a comma, which
// makes the line a free format card and splits the label: a label with a comma cannot be kept, as for the SELABEL).
// Only one MDLABEL per module is allowed: the MID is the ID of the card and unique. The card belongs in the
// main Bulk Data and works only when the deck has modules; the reader keeps it wherever it
// stands. The MID is a module number, a plain integer: no card is named.
struct ModuleLabelCard
{
    base::EntityId id = 0; // the module; 0 is the main Bulk Data
    std::string entryLabel;

    [[nodiscard]] bool operator==(const ModuleLabelCard&) const noexcept = default;
};

} // namespace oor::model
