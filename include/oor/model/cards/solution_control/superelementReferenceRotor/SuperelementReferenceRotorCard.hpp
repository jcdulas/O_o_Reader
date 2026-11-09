#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// A non-reference rotor of a REFRTSE and the superelement it is defined in (NFROTIDi and NFROTSEi).
struct RotorInSuperelement
{
    base::EntityId rotorId = 0;
    std::int32_t superelement = 0; // 0: the residual structure

    [[nodiscard]] bool operator==(const RotorInSuperelement&) const noexcept = default;
};

// One REFRTSE card: the reference rotor of the non-reference rotors of any superelement, for a complex eigenvalue
// analysis.
//
// Card layout: REFRTSE  RID  REFROTID  REFROTSE  NFROTID1  NFROTSE1  NFROTID2  NFROTSE2  NFROTID3  NFROTSE3  ...
// The pairs of a rotor and its superelement follow each other with no break (the third pair has its rotor in field 8
// and its superelement in field 9, the first of the continuation line: the format draws the second line with the
// superelement of the third pair under the name NFROTSE2, a misprint). REFROTSE and the NFROTSEi are 0 when blank (the
// residual structure), a default that does not depend on another field: they are stored as 0 and written as blanks.
// RID is the number of an RGYRO set that the Case Control command RGYRO selects, shared by the cards of the set:
// it is the card's ID in the group space and is not looked up. A rotor is a reference to the rotors of the
// model only when its superelement is 0: the others live in the part of their superelement, which the card does not
// look into (`visitRotor`). That the same rotor and superelement are not on two REFRTSE of one RID is a rule
// between cards that the card does not check.
struct SuperelementReferenceRotorCard
{
    base::EntityId id = 0;
    base::EntityId referenceRotorId = 0;
    std::int32_t referenceSuperelementId = 0;
    std::vector<RotorInSuperelement> rotors; // NFROTIDi and NFROTSEi

    [[nodiscard]] bool operator==(const SuperelementReferenceRotorCard&) const noexcept = default;
};

} // namespace oor::model
