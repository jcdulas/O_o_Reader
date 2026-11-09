#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One LSEQ card: the static load vector a dynamic analysis starts from (selected by the Case Control command LOADSET).
//
// Card layout: LSEQ  SID  EXCITEID  LID  TID
// SID is the set of LSEQ cards that LOADSET = SID picks. EXCITEID is the excitation set the vector is given (the
// EXCITEID of an RLOADi or TLOADi); LID names a set of static loads and TID a set of temperatures (TEMP, TEMPD, ...),
// either may be blank (0 here), both only in a superelement analysis.
struct StaticLoadSequenceCard
{
    base::EntityId id = 0;
    base::EntityId excitationId = 0;
    base::EntityId loadId = 0;
    base::EntityId tableId = 0;
};

} // namespace oor::model
