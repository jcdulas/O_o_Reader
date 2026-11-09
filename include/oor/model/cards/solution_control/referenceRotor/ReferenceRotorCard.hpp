#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One REFROT card: the reference rotor of the non-reference rotors of the residual structure, for a complex
// eigenvalue analysis.
//
// Card layout: REFROT  RID  REFROTID  NFROTID1  NFROTID2  NFROTID3  ...   the rotors go on over the continuation lines
//        or:  REFROT  RID  REFROTID  NFROTID1  THRU  NFROTIDn  BY  INC
// RID is the number of an RGYRO set that the Case Control command RGYRO selects: several cards share it, so
// it is the ID of the card in the group space, and it is not looked up (no card of the library is "an RGYRO set"). The
// rotors, REFROTID and the NFROTIDs, are in the residual structure, so all of them are rotors of the model
// (ReferenceTarget::Rotor). The non-reference rotors are single IDs and ascending "a THRU b [BY n]" ranges (the
// `ContactRun` of the contact cards: a range stores its increment resolved, 1 when BY is left out).
struct ReferenceRotorCard
{
    base::EntityId id = 0;
    base::EntityId referenceRotorId = 0;
    std::vector<ContactRun> rotors; // NFROTIDi

    [[nodiscard]] bool operator==(const ReferenceRotorCard&) const noexcept = default;
};

} // namespace oor::model
