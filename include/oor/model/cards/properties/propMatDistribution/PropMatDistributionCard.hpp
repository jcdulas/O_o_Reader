#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/DistributedPropertyType.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace oor::model
{

// One PDISTBM card: a spatial distribution of one material of a property. For each element it lists (EIDi) the material
// (MIDVi) that replaces the material number MIDNUM of the property PID, so the elements of one property can be made of
// different materials.
//
// Card layout: PDISTBM  ID  TYPE  PID  MIDNUM                                   fields 1 to 4
//                      EID1  MIDV1  EID2  MIDV2  EID3  MIDV3  EID4  MIDV4     fields 9 to 16
//                      EID5  MIDV5  EID6  MIDV6  ...                           fields 17 on
// Four pairs make a line and a card goes on for as many lines as it has pairs; fields 5 to 8 are blank cells of the
// layout.
//
// ID is the number of the entry, unique among the PDISTBM cards only. TYPE is the card name of the property (the same
// ten as the PDISTB: PBAR, PBEAM, PBEAM3, PCOMP, PCOMPG, PROD, PSHEAR, PSHELL, PSOLID or PTUBE) and PID names a
// property of that kind. MIDNUM says which of the materials of the property is replaced: a PROD has one, so
// 1 is right; a PSHELL has MID1 to MID4, so 3 replaces MID3; for a PCOMP or PCOMPG it is the number of the ply, counted
// from 1 in the order of the stack and not the GPLYID. The format gives it the range 0 and up and the default 1, which
// the card keeps (the property decides what a number means, and the card cannot see its property). Each MIDV is a
// material; the format asks for a MATi that is unique among the MATi cards.
//
// A card with no pair gives the elements nothing, so the property keeps its own materials: the card is kept.
// An element is listed at most once in a card; that another PDISTBM of the same PID and MIDNUM does not list
// it again is a rule between cards that the card cannot see.
struct PropMatDistributionCard
{
    base::EntityId id = 0; // unique among the PDISTBM cards
    DistributedPropertyType type = DistributedPropertyType::ShellProp;
    base::EntityId propId = 0;
    std::int32_t materialIndex = 1;
    std::vector<std::pair<base::EntityId, base::EntityId>> entries; // (EID, MIDV)

    [[nodiscard]] bool operator==(const PropMatDistributionCard&) const noexcept = default;
};

} // namespace oor::model
