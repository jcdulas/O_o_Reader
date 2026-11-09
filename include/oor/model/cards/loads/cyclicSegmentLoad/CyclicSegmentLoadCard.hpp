#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// SEGTYPE: which half of a segment of a dihedral cyclic symmetry model the load is on (R or L), blank for both.
enum class SegmentType : std::uint8_t
{
    Both, // blank
    Right,
    Left
};

// One LOADCYN card: a physical load on a segment of a cyclic symmetry model, from one or two load sets.
//
// Card layout: LOADCYN  SID  S  SEGID  SEGTYPE  S1  L1  S2  L2
// The load on the segment is S times (S1 times the load set L1 plus S2 times the load set L2). The second pair is
// optional (l2 is 0 and s2 0.0 when it is not there). S must not be zero, nor S1 and S2 both.
struct CyclicSegmentLoadCard
{
    base::EntityId id = 0;
    double scaleFactor = 0.0;
    std::int32_t segmentId = 0; // SEGID
    SegmentType segmentType = SegmentType::Both;
    double scaleFactor1 = 0.0;
    base::EntityId loadSetId1 = 0;
    double scaleFactor2 = 0.0;
    base::EntityId loadSetId2 = 0;

    [[nodiscard]] bool hasSecond() const noexcept { return loadSetId2 != 0; }
};

} // namespace oor::model
