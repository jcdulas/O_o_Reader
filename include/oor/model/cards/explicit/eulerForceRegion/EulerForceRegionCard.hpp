#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/EulerRegionType.hpp"

namespace oor::model
{

// One EULFREG card (SOL 700): one Eulerian region (a shape and a material) with the time dependent acceleration that
// acts in it. The cards that share an ESID form the set of regions that an EULFOR1 names, so the ESID is the ID the
// model files the card under (IdSpace::Group, category EulerRegionSet) and the ERID is only the number of the card. The
// format reference says the combination of region and material decides where the acceleration is applied and
// that only part of the Euler domain may be covered.
//
// Card layout: EULFREG  ERID  ESID  TYPE1  VID1  MID1  ACCX  ACCY  ACCZ            fields 1 to 8
//                      LEVEL                                                        field 9
// One region per card. The descriptions of the format reference number the region fields (TYPEi, VIDi, MIDi, LEVELi)
// and the grid shows the first group only, with nothing after LEVEL; the TICREG entry of the same shape has one region
// per card and is grouped by the same kind of ID, and the ERID is called unique. The card therefore reads the nine
// documented fields and refuses a tenth: how a second group would be laid out is not documented. The format names the
// fields TYPEi and so on, the card calls them TYPE, VID and MID. VID is what the TYPE says it is: the number of a
// SURFINI, SPHERE or CYLINDR (the category EulerRegionShape), of a BCBOX (BOX) or of a SET1 (ELEM). MID is the MATDEUL
// the acceleration is applied to. ACCX, ACCY and ACCZ are the TABLED1 that give the acceleration in each direction over
// time; all three are required. LEVEL is the level indicator of the region (default 0.0): the format reference says the
// levels of all the regions of one ESID must differ, a rule across cards that this card cannot check.
struct EulerForceRegionCard
{
    base::EntityId id = 0;    // the number of the card
    base::EntityId setId = 0; // the set of regions, which an EULFOR1 names
    EulerRegionType type = EulerRegionType::Surface;
    base::EntityId shapeId = 0; // the shape, as TYPE says
    base::EntityId matId = 0;   // a MATDEUL
    base::EntityId accelX = 0;  // the TABLED1 of the acceleration in each direction
    base::EntityId accelY = 0;
    base::EntityId accelZ = 0;
    double level = 0.0; // the level indicator of the region
};

} // namespace oor::model
