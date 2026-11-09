#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/EulerRegionType.hpp"

namespace oor::model
{

// One TICREG card (SOL 700): one Eulerian region (a shape and a material) with the set of initial values that its
// material starts with. The cards that share a TSID form the group of regions that a TICEUL1 names, so the TSID of
// field 2 is the ID the model files the card under (IdSpace::Group, category EulerInitialRegions) and the TRID is only
// the number of the card, as the ERID of an EULFREG is.
//
// Card layout: TICREG  TRID  TSID  TYPE  VID  MID  TSID  LEVEL                          fields 1 to 7
// The format gives two fields the name TSID. The first (field 2) is the group a TICEUL1 names; the second (field 6,
// called `tsid2` here, as a PEULER1 has a SID2) is the TICVAL that holds the initial values of this material. TYPE says
// what VID is, as for an EULFREG (the two cards share EulerRegionType): the number of a SURFINI, SPHERE or CYLINDR
// (category EulerRegionShape), of a BCBOX (BOX, the contact surface category) or of a SET1 (ELEM). MID is a MATDEUL (0
// or blank: none) and TSID2 a TICVAL (0 or blank: none), and LEVEL is the level indicator of this material and its
// initial values (default 0.0). The card has one region: the format draws no second group, and the reader refuses a
// field after LEVEL.
struct InitialConditionRegionCard
{
    base::EntityId id = 0;      // the number of the card
    base::EntityId groupId = 0; // the group of regions, which a TICEUL1 names
    EulerRegionType type = EulerRegionType::Surface;
    base::EntityId shapeId = 0;  // the shape, as TYPE says
    base::EntityId matId = 0;    // a MATDEUL, 0 for none
    base::EntityId valuesId = 0; // a TICVAL with the initial values, 0 for none
    double level = 0.0;          // the level indicator of the region
};

} // namespace oor::model
