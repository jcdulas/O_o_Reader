#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One AESCALE card: the reference lengths that scale the coordinates of the aerodynamic grid points (AEGRID) which name
// it.
//
// Card layout: AESCALE  ASID  X1REF  X2REF  X3REF
// A grid point that gives this ASID has each coordinate Xi multiplied by XiREF (XiS = Xi * XiREF). The
// coordinates are the ones given in the system CP of the AEGRID, and the scaling comes before the transformation of the
// point to the basic system. A blank reference length is 1.0, which leaves the coordinate as it is.
//
// The format reference puts no limit on a reference length (it only says "real"): a factor of 0.0 flattens the mesh in
// that direction and a negative one mirrors it, so the card checks only that the numbers are finite. The AEGRID,
// AETRIA3, AEQUAD4 and AESCALE cards are a portable description of an aerodynamic mesh: no aerodynamics are built on
// it.
struct AeroNodeScaleCard
{
    static constexpr double DEFAULT_REFERENCE_LENGTH = 1.0;

    base::EntityId id = 0; // the number an AEGRID gives in its ASID field; unique among the AESCALE cards
    double referenceLength1 = DEFAULT_REFERENCE_LENGTH;
    double referenceLength2 = DEFAULT_REFERENCE_LENGTH;
    double referenceLength3 = DEFAULT_REFERENCE_LENGTH;
};

} // namespace oor::model
