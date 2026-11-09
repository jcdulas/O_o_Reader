#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One SELABEL card: the label a superelement carries in the headings of the output.
//
// Card layout: SELABEL  SEID  LABEL                                         fields 1 and 2 to 8
// SEID is a partitioned superelement. LABEL is text of up to 56 characters, written over the seven fields after the
// SEID (fields 3 to 9 of the format reference): the characters of the line from the first field of the label to the
// last are the label, blanks between the pieces included (a label of two words is two pieces), so a label reads back as
// it was written in the fixed layout; the large and free formats hold it whole in one field. It begins and ends with a
// character that is not a blank and holds no '$', comma or tab, because the line would not read back. A superelement
// has one SELABEL, the card belongs in the main Bulk Data and is ignored after a BEGIN SUPER, and it works
// only when the deck has part superelements. a common deck, `LEFT REAR FENDER, MODEL XYZ2000`, has a
// comma, which makes the line a free format card and splits it; a label with a comma cannot be kept.
struct SuperelementLabelCard
{
    base::EntityId superelementId = 0;
    std::string entryLabel;

    [[nodiscard]] bool operator==(const SuperelementLabelCard&) const noexcept = default;
};

} // namespace oor::model
