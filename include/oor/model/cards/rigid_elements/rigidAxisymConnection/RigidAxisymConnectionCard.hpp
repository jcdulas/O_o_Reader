#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One RBAX3D card: a rigid connection between a point of an axisymmetric harmonic element (CQUADX or CTRIAX) and a
// point of the three-dimensional model on the axis of symmetry.
//
// Card layout:  RBAX3D  EID  G3D  GAX        fields 1 to 3
// EID is an element number from 1 to 99,999,999 (the format says 0 < EID < 100,000,000), unique among all the elements.
// GAX is a grid point that an axisymmetric harmonic element references, from 1 to 99,999,999 too; it is
// the dependent point and need not be on the axis. G3D is the independent point, on the axis of
// symmetry and not referenced on an axisymmetric harmonic element: it is meant to be above 100,000,000, but small
// numbers are common, so the card does not refuse a G3D below that limit and accepts the large ones too. The two points
// are different, and have the same axial coordinate. Whether GAX is on an element of NHARM = 1, whether G3D is on none
// and the coordinates are rules between cards: they are not looked at here.
//
// A rigid element is an element: the ID is in the space shared by all the elements, and both points are grid points.
struct RigidAxisymConnectionCard
{
    static constexpr base::EntityId ID_LIMIT = 100000000; // EID and GAX are below it

    base::EntityId id = 0;
    base::EntityId node3D = 0;     // the independent point, on the axis of symmetry
    base::EntityId axisymNode = 0; // the dependent point, a point of an axisymmetric harmonic element
};

} // namespace oor::model
