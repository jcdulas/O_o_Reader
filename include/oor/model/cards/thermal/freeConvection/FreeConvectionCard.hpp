#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CONV card: the free convection boundary condition of a surface element.
//
// Card layout:
//   CONV  EID  PCONID  FLMND  CNTRLND  TA1  TA2  TA3  TA4
//         TA5  TA6     TA7    TA8
// EID is the CHBDYE, CHBDYG or CHBDYP element, PCONID the PCONV. FLMND (the film temperature point) and CNTRLND (the
// control point) are 0 when blank. TA1 is the ambient point and a blank TA2 to TA8 is TA1 (format reference): stored
// resolved and always written; an explicit 0 is no point. The points are grid points or scalar points (the ambient
// temperature of a convection is usually a scalar point).
struct FreeConvectionCard
{
    static constexpr std::size_t AMBIENT_COUNT = 8;

    base::EntityId id = 0;
    base::EntityId convectionPropId = 0;
    base::EntityId filmNode = 0;
    base::EntityId controlNode = 0;
    std::array<base::EntityId, AMBIENT_COUNT> ambientNodes = {};
};

} // namespace oor::model
