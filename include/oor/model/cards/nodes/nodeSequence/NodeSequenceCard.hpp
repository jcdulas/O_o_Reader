#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// A point and the sequence number it is given.
struct SequencedPoint
{
    base::EntityId node = 0; // IDi: a grid or scalar point
    double sequence = 0.0;   // SEQIDi: an integer, or a real that puts the point between two others (15.6 follows 15)

    [[nodiscard]] bool operator==(const SequencedPoint&) const noexcept = default;
};

// One SEQGP card: the sequence in which grid and scalar points are ordered (manual resequencing).
//
// Card layout: SEQGP  ID1  SEQID1  ID2  SEQID2  ID3  SEQID3  ID4  SEQID4
// From one to four points. SEQID is an integer or a real greater than zero; both are kept as a real, and the writer
// writes a whole number as an integer (a SEQID of 20. and one of 20 are the same sequence number). The card has no ID.
struct NodeSequenceCard
{
    std::vector<SequencedPoint> points;
};

} // namespace oor::model
