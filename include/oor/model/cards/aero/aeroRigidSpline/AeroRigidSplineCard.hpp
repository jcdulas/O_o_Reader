#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <cstddef>
#include <vector>

namespace oor::model
{

// One SPLINRB card: a rigid body spline that moves the boxes (or the aerodynamic grid points) of an AELIST like a rigid
// body whose motion is given by six structural degrees of freedom, whatever the aerodynamic geometry is.
//
// Card layout: SPLINRB  SID  CAERO  AELIST  USAGE  G1  C1  G2  C2
//                      G3   C3     G4      C4     G5  C5  G6  C6
// SID is the number of the spline, in the category AeroSpline with SPLINE1 to SPLINE7 (unique per card type, a check
// compares the types). CAERO (ReferenceTarget::AeroElement) is the aerodynamic component that defines the interpolation
// surface and may be blank: the format reference takes a positive integer or a blank. AELIST
// (ReferenceTarget::AeroList) lists the boxes, or the aerodynamic grid points of an AEGRID mesh, or the grid points of
// a Mach box AEFACT. USAGE is FORCE, DISP or BOTH, BOTH when blank.
//
// The up to six (grid point, components) pairs are kept in the order of the card, each as a DofPair (the components are
// the digits of Ci in ascending order). A pair that is blank on both fields is padding and is dropped. The format says
// the pairs select exactly six degrees of freedom, so the digits of all the Ci together are six; that the six must
// make a statically determinate support is a property of the structure and is not checked here.
struct AeroRigidSplineCard
{
    static constexpr std::size_t MAX_PAIRS = 6;
    static constexpr int TOTAL_COMPONENTS = 6;

    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0;
    base::EntityId aeroBoxListId = 0;
    SplineUsage usage = SplineUsage::Both;
    std::vector<DofPair> dofs;
};

} // namespace oor::model
