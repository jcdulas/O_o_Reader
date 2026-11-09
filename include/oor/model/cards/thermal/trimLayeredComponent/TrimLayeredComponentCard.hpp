#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// The default of the scaling factor of a layer.
inline constexpr double TRIM_LAYERED_DEFAULT_SCALE = 1.0;

// One layer of a TCOMPG: its global (external) number, the material, the thickness and the scaling factor.
struct TrimLayeredComponentLayer
{
    base::EntityId globalPlyId = 0; // the global layer ID, unique among the layers of the card
    base::EntityId matId = 0;       // a MATPE1, MAT1 or MAT10
    double thickness = 0.0;         // greater than zero
    double scaleFactor =
        TRIM_LAYERED_DEFAULT_SCALE; // how much the thickness follows a change of the total one, 0.0 or more

    [[nodiscard]] bool operator==(const TrimLayeredComponentLayer&) const noexcept = default;
};

// One TCOMPG card: the layers of a 1D analytical trim component (a stack of porous or elastic materials with no mesh of
// its own), listed from the bottom one. The card goes in the Bulk Data of the trim component (`BEGIN BULK TRMC`, with
// the SET3 and PLTSURF cards) and takes precedence over a finite element definition of the component. The
// SET3 names the PLTSURF surface mesh that is the bottom surface the layers are stacked up from; another PLTSURF mesh
// is the top surface, and an ACPMCP1 gives the interface coupling of both.
//
// Card layout: TCOMPG  SET3ID                                       field 1
//                     GPLYID1  MID1  T1  SCALE1                    fields 9 to 12 (the second line)
//                     GPLYID2  MID2  T2  SCALE2                    fields 17 to 20 (the third line)
//                     ...                                          one layer to a line
// The format draws a layer to a line, in the first four fields of the line; the fifth to eighth are blank cells.
//
// SET3ID is the ID of a SET3 of PLTSURF cards: the card is a reference to it and its own ID is the number of that SET3.
// Several TCOMPG cards may exist for different SET3 cards, and the format only recommends that the SET3ID be
// unique, so no ID space is unique (IdSpace::Group) and the repeat of a SET3ID is a rule between cards.
// GPLYIDi is the number the user gives the layer, unique within the card; the layers are in the order of
// their stack, from the bottom layer, and the order of the lines is that order (the GPLYID is not the position). MIDi
// is a material and Ti a thickness greater than zero; SCALEi is the factor that decides how much of a change of the
// total thickness the layer takes, 0.0 or more and 1.0 when blank. A card has at least one layer.
struct TrimLayeredComponentCard
{
    base::EntityId id = 0; // the SET3 of PLTSURF cards, which is also the ID of the card
    std::vector<TrimLayeredComponentLayer> layers;

    [[nodiscard]] bool operator==(const TrimLayeredComponentCard&) const noexcept = default;
};

} // namespace oor::model
