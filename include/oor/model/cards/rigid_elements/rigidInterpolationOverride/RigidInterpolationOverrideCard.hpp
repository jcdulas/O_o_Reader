#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// The perturbation values of a GMOVR3 when a field is blank (0.01 for each of the three).
inline constexpr double RIGID_OVERRIDE_DEFAULT_PERTURBATION = 0.01;

// One RBE3 of a GMOVR3: the element and the values that override the MDLPRM,GMOVR3 perturbation of the coincident grid
// points of its Lagrange formulation.
struct RigidInterpolationOverrideGroup
{
    base::EntityId elemId = 0;                                  // ELEMi: the RBE3 element
    double perturbationX = RIGID_OVERRIDE_DEFAULT_PERTURBATION; // the perturbation of the X coordinate
    double perturbationY = RIGID_OVERRIDE_DEFAULT_PERTURBATION; // ... of the Y coordinate
    double perturbationZ = RIGID_OVERRIDE_DEFAULT_PERTURBATION; // ... of the Z coordinate
    double toleranceAdjustment = 0.0;                           // the adjustment to the MDLPRM,GMOVR3T value (GMOVR3Ti)

    [[nodiscard]] bool operator==(const RigidInterpolationOverrideGroup&) const noexcept = default;
};

// One GMOVR3 card (SOL 400): overrides, for the RBE3 elements it lists, of the perturbation that MDLPRM,GMOVR3 gives to
// the grid points that coincide in a Lagrange (rigid = Lagrange) formulation.
//
// Card layout: GMOVR3  ELEM1  dX1  dY1  dZ1  dGMOVR3T1                                  fields 1 to 5 of each line
//                     ELEM2  dX2  dY2  dZ2  dGMOVR3T2                                  fields 9 to 13 of the next line
//                     -etc.-                                                           one RBE3 to a line
// Each line has an RBE3 element and four reals: the perturbations of the three coordinates (0.01 when blank) and the
// adjustment to the MDLPRM,GMOVR3T value (0.0 when blank; the zero tolerance, GMOVR3T + dGMOVR3Ti, must not be
// negative, which depends on the MDLPRM and is not checked). The first line of the format's grid has the first RBE3
// beside the card name and the second line starts at its first field, so a group is at fields 1, 9, 17, ... and its
// three last fields are blank. The entry acts only when MDLPRM,GMOVR3 is 3 or 4; it affects an RBE3
// only when every connected grid point coincides with the reference grid point within the tolerance (an entry that does
// not meet that is ignored). The card names the RBE3 elements (`Element`: an ID that is not an RBE3 is a rule between
// two cards). The card has no ID of its own and any number of cards may be in a deck.
struct RigidInterpolationOverrideCard
{
    std::vector<RigidInterpolationOverrideGroup> groups; // one RBE3 each, in the order of the card

    [[nodiscard]] bool operator==(const RigidInterpolationOverrideCard&) const noexcept = default;
};

} // namespace oor::model
