#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One SPRELAX card: the relaxation of a spline on its neighbour. The displacements of spline 1 are pulled towards those
// of spline 2 as the aerodynamic grid point gets close to a curve: the weight of spline 2 falls linearly from 1 on the
// curve to 0 at the reference distance DREF.
//
// Card layout: SPRELAX  SID1  SID2  LIST2  DREF  LIST1
// SID1 is the spline that is modified and SID2 the adjacent spline; both are splines of any type of the category
// AeroSpline and either may be a blend (ReferenceTarget::AeroSpline). The format reference says SID1 need not be
// unique: several cards may relax the same spline, so the card has no ID space (IdSpace::None) and its SID1 is a
// reference, not an identity. LIST2 (ReferenceTarget::AeroList) lists the aerodynamic grid points of the curve, DREF is
// the reference distance (positive, no default). LIST1 (ReferenceTarget::AeroList) is optional: when it is given only
// its grid points are relaxed, otherwise all the aerodynamic grid points of spline 1 are. A blank LIST1 is 0, which is
// the format reference default.
struct AeroSplineRelaxationCard
{
    base::EntityId splineId1 = 0;
    base::EntityId splineId2 = 0;
    base::EntityId curvePointListId = 0;
    double referenceDistance = 0.0;
    base::EntityId relaxedPointListId = 0;
};

} // namespace oor::model
