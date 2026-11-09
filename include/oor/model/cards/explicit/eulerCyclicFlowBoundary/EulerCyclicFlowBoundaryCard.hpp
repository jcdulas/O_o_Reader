#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/BoundaryBox.hpp"
#include "oor/model/cards/explicit/EulerFlowBoundaryProperty.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

#include <cstdint>
#include <optional>

namespace oor::model
{

// One FLOWC card (SOL 700): a cyclic (periodic) flow boundary of an Euler mesh. What flows out of this boundary comes
// in through the boundary of the card named by FID2, and the other way round. No other entry of the format reference
// has this name.
//
// Card layout: FLOWC  FID  BCID  TYPE  MESH  DIR  FID2                  fields 1 to 6
//             +      XMIN  XMAX  YMIN  YMAX  ZMIN  ZMAX               fields 9 to 14
//             +      METHOD  TID                                      fields 17 and 18
// FID numbers the card, TYPE (IN, OUT or BOTH) and FID2 are required. The cards come in pairs: the FID of one is the
// FID2 of the other (the format says so, and the pair is not checked by the card: a check of the whole model could).
// FID2 names a FLOWC, a card type that has no category of its own, so it is a plain ID that is neither visited nor
// looked up. The faces are chosen as for the other flow cards: BCID (a set of BCSEG cards), else DIR, else the box.
// METHOD (1 when blank) says how the properties and the pressure are carried from one boundary to the other: 1 both one
// to one, 2 the material flow one to one and the pressure from the elements, 3 the inflow is the average of the outflow
// and the pressure comes from the elements. TID is the TABLED1 of a time dependent factor on the inflow, 0 when not
// used. The format is not consistent about where FID2 is (field 3 or 4 in places, field 6 behind TYPE, MESH and DIR in
// the layout); the card follows the layout. The format also ties the card to a TLOAD1 through a LID that this card does
// not have, so the FID is taken as the number a TLOAD1 excites.
struct EulerCyclicFlowBoundaryCard
{
    static constexpr std::int32_t DEFAULT_METHOD = 1;
    static constexpr std::int32_t LAST_METHOD = 3;

    base::EntityId id = 0;            // unique among the FLOWC cards
    base::EntityId contactBodyId = 0; // a set of BCSEG cards, 0 when blank
    FlowBoundaryType type = FlowBoundaryType::Both;
    base::EntityId meshId = 0; // an Euler MESH, 0 when blank
    std::optional<BoundaryFace> meshFace;
    base::EntityId pairedBoundaryId = 0; // the FID of the other FLOWC of the pair
    BoundaryBox box;
    std::int32_t method = DEFAULT_METHOD;
    base::EntityId tableId = 0; // a TABLED1, 0 when blank
};

} // namespace oor::model
