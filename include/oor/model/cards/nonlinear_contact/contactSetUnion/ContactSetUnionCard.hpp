#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One BCTADD card: the union of contact sets (the old contact definition of SOL 600 and of the contact of the first SOL
// 400 decks, BCTSET cards) under one ID that the Case Control command BCSET selects. The format reference does not list
// this card.
//
// Layout: BCTADD  CSID  S1  S2  S3  ...   over as many continuation lines as needed (at least one set).
// The sets are the IDs of BCTSET cards, so they are references to the contact tables
// (ReferenceTarget::ContactTable). A set is not the ID of this or of any other BCTADD, which the card checks for its
// own ID and the validation of the contact tables checks across the cards.
struct ContactSetUnionCard
{
    base::EntityId id = 0;
    std::vector<base::EntityId> sets;
};

} // namespace oor::model
