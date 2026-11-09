#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One MDMOVE card: the sequence of moves that repositions a module, given as the numbers of MDTRAN, MDROT1, MDROT2,
// MDMIR1 and MDMIR2 cards, applied in the order of the list.
//
// Card layout: MDMOVE  MODID  MVID1  MVID2  MVID3  -etc.-            fields 1, 2 and so on
// The list goes on over the continuation lines, with no end marker, and has at least one move (MVID1). A blank field
// between two moves is skipped. MODID is the number of a primary or copied module, a plain integer (a BEGIN MODULE
// section is not a card of the model); the format reference asks for an MDBULK entry of the same MODID with TYPE=MOVE,
// a rule between cards. The format says that an MDMIR1 or MDMIR2 may only be named in MVID1: the card does not know
// which kind each MVID names, so that is a check of the model, not of the card. The format reference puts the card in
// the main Bulk Data only.
//
// One MDMOVE per module: the MODID is unique per card type (IdSpace::PerCardType, category Other).
struct ModuleMoveSequenceCard
{
    base::EntityId id = 0;               // the module that the moves reposition
    std::vector<base::EntityId> moveIds; // MVID1, MVID2, ...: the moves, in order
};

} // namespace oor::model
