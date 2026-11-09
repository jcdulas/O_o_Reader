#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The coordinate system the load vector N is measured in (the CID field): the words BASIC, LOCAL and ELEMENT, or the ID
// of a coordinate system. The first three enumerators are numbered in the order of BEAM3_LOAD_FRAME_WORDS.
enum class Beam3DistributedLoadFrame : std::uint8_t
{
    Basic,     // BASIC, 0 or blank (the default)
    Local,     // LOCAL: the local coordinate system
    Element,   // ELEMENT: the element coordinate system
    Coordinate // the ID of a coordinate system, in Beam3DistributedLoadCard::cid
};

inline constexpr std::array<std::string_view, 3> BEAM3_LOAD_FRAME_WORDS{"BasicFrame", "LocalFrame", "Element"};

// What the load is: a force, a moment or a bimoment per unit length (the TYPE field, required). The enumerators follow
// BEAM3_LOAD_TYPE_WORDS.
enum class Beam3DistributedLoadType : std::uint8_t
{
    Force,
    Moment,
    Bimoment
};

inline constexpr std::array<std::string_view, 3> BEAM3_LOAD_TYPE_WORDS{"Force", "Moment", "WarpingBimoment"};

// One PLOADB3 card: a distributed load on a CBEAM3 element, over the whole length of the beam axis, along the line of
// the shear center.
//
// Card layout: PLOADB3  SID  EID  CID  N1  N2  N3  TYPE  SCALE        fields 1 to 8
//                      P(A) P(B) P(C)                                 fields 9 to 11
// The load at the node j of the element (j = A, B, C) is SCALE x P(j) x N: N gives the direction, and its
// length counts in the magnitude (it is not normalised). At least one of N1, N2 and N3 is not zero; the format gives no
// default for them, so a blank one is 0.0. CID is BASIC when blank; the words and the integer are the `frame` and, for
// a coordinate system, `cid` (0 for the three words). SCALE is 1.0 and each P(j) is 0.0 when blank; TYPE is required.
// EID is a CBEAM3, an element of the structural model (the card visits it as any element: that it is a CBEAM3 is a rule
// between cards).
struct Beam3DistributedLoadCard
{
    base::EntityId id = 0;
    base::EntityId elemId = 0;
    Beam3DistributedLoadFrame frame = Beam3DistributedLoadFrame::Basic;
    base::EntityId frameId = 0;             // the coordinate system when frame is Coordinate, else 0
    std::array<double, 3> n{0.0, 0.0, 0.0}; // N1, N2, N3
    Beam3DistributedLoadType kind = Beam3DistributedLoadType::Force;
    double scaleFactor = 1.0;
    std::array<double, 3> p{0.0, 0.0, 0.0}; // P(A), P(B), P(C)

    [[nodiscard]] bool operator==(const Beam3DistributedLoadCard&) const noexcept = default;
};

} // namespace oor::model
