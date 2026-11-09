#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/elements/sol700_scalar/ScalarFollow.hpp"

#include <cstdint>

namespace oor::model
{

// One CELAS2D card: the scalar spring of SOL 700 that holds its own stiffness (no property), with the coordinate
// system its direction is given in.
//
// Card layout (fields numbered from the first data field; the second line is the continuation line):
//   CELAS2D  EID  K  G1  C1  G2  C2                   fields 1 to 6
//            CORD FOLLOW                              fields 9 and 10
// It is the CELAS2 without GE and S (the format has no such fields) plus the two fields of the continuation line. K is
// a real with no default: the card needs it. G1 and G2 are grid points or scalar points, 0 (or blank) is a grounded
// end. Each of C1 and C2 is 0 (a scalar point, ground) or the digits 1 to 6 each at most once: the format prints the
// range 0 to 6 and then allows up to six distinct digits, so both are read, in ascending order like every set of
// components. CORD is the coordinate system the degree of freedom (C1, C2) is defined in (0 or blank: none given).
// FOLLOW is blank or CORD.
struct SpringDofInlineExplicitCard
{
    base::EntityId id = 0;
    double stiffness = 0.0; // stiffness
    base::EntityId node1 = 0;
    std::int32_t components1 = 0;
    base::EntityId node2 = 0;
    std::int32_t components2 = 0;
    base::EntityId frameId = 0; // 0: no coordinate system given
    ScalarFollow follow = ScalarFollow::Fixed;
};

} // namespace oor::model
