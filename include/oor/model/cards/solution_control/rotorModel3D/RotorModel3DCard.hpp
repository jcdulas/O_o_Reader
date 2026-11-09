#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// The frame of reference of a ROTOR (FRAME): fixed (FIX, the default) or rotating with the rotor (ROT).
enum class RotorModel3DFrame : std::uint8_t
{
    Fixed,   // FIX
    Rotating // ROT
};

// The words of FRAME, in the order of RotorModel3DFrame.
inline constexpr std::array<std::string_view, 2> ROTOR_FRAME_WORDS{"FixedEnd", "Rotation"};

// One ROTOR card: the 3D model of a rotor, as the elements or the properties that make it and the grid points of its
// axis of rotation.
//
// Card layout: ROTOR  ROTORID  FRAME
//                    ELEM  ID1  ID2  ID3  ...             ELEM, PROP and AXIS each start a continuation line
//                    PROP  ID1  THRU  ID2  ...            (their keyword is the first field of the line: field 9,
//                    AXIS  GID1  GID2  ...                 17, ...) and their lists go on over the lines that follow
//                                                          with a blank first field
// FRAME is FIX unless it says ROT; a blank is FIX, a default that does not depend on another field, so it is stored as
// FIX and written as a blank. A rotor lists elements, properties or both, and then ELEM comes first (the format
// makes the order of the LTYPE groups matter), so a PROP before an ELEM is refused. The IDs of ELEM and PROP are single
// IDs and ascending "a THRU b" ranges (the format has no BY here); a range is kept as the `ContactRun` of the contact
// cards. The AXIS lists at least two grid points that need not belong to the rotor. That ROTORAX, ROTORG and ROTORSE go
// only with the frame FIX is a rule between cards that the card does not check.
struct RotorModel3DCard
{
    base::EntityId id = 0;
    RotorModel3DFrame frame = RotorModel3DFrame::Fixed;
    std::vector<ContactRun> elements;   // ELEM; empty when the card has no such line
    std::vector<ContactRun> properties; // PROP
    std::vector<base::EntityId> axis;   // AXIS: the grid points of the axis of rotation

    [[nodiscard]] bool operator==(const RotorModel3DCard&) const noexcept = default;
};

} // namespace oor::model
