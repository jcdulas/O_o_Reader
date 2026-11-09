#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One SETREE card: the superelements that are upstream of one, which gives the order of the reduction.
//
// Card layout: SETREE  SEID  SEUP1  SEUP2  SEUP3  SEUP4  SEUP5  SEUP6  SEUP7
//                     SEUP8  SEUP9  -etc.-
// SEID is the downstream superelement (0 is the residual structure: the format asks for an entry for every superelement
// that is not a tip, the residual structure included), SEUPi the superelements upstream of it. Several cards may have
// the same SEID. A superelement is upstream of at most one other, so it appears once in the SEUPi of all the cards.
// The card is the alternate form of the DTI,SETREE record, which it makes unnecessary. A
// card that lists no upstream superelement is refused. The SEID of a card is not its ID: the tree is made of the cards
// together.
struct SuperelementTreeCard
{
    base::EntityId superelementId = 0;
    std::vector<base::EntityId> upstream;

    [[nodiscard]] bool operator==(const SuperelementTreeCard&) const noexcept = default;
};

} // namespace oor::model
