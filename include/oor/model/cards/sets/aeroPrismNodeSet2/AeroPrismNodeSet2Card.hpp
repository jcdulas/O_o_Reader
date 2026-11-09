#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One SET2 card: a list of structural grid points defined by a prism over an aerodynamic macro element (a CAERO), which
// a SPLINEi names as its structural set.
//
// Card layout: SET2  SID  MACRO  SP1  SP2  CH1  CH2  ZMAX  ZMIN
// MACRO is the element ID of the CAERO panel. SP1 and SP2 are the lower and higher span division points of the prism
// and CH1 and CH2 the chord ones, as fractions of the panel (the format reference suggests -0.01 and 1.01 to catch the
// points that lie on the boundary); ZMAX and ZMIN are the heights of its top and bottom. A ZMAX or ZMIN of 0.0, which
// is what a blank field means, stands for infinity. The four division points are required reals; the format
// does not require SP1 < SP2 or CH1 < CH2, so a reversed pair is kept (it selects no point). The SID is shared with the
// SET1 and SET3 cards (a SPLINEi names any of the three), so the card is in the category Set with IdSpace::Group.
struct AeroPrismNodeSet2Card
{
    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0;
    double spanStart = 0.0;
    double spanEnd = 0.0;
    double chordStart = 0.0;
    double chordEnd = 0.0;
    double zMaximum = 0.0;
    double zMinimum = 0.0;
};

} // namespace oor::model
