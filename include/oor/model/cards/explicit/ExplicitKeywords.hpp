#pragma once

#include <array>
#include <cstdint>
#include <string_view>

// The keywords that several cards of the explicit solver use with the same words. Each enumeration is numbered in the
// order of its array of words (ExplicitSupport.hpp reads and writes them by position). A keyword that only one card (or
// one group of cards) uses is declared next to that card.
namespace oor::model
{

// ON or OFF: the REVERSE and CHECK switches of a coupling or initialization surface.
enum class Switch : std::uint8_t
{
    On,
    Off
};
inline constexpr std::array<std::string_view, 2> SWITCH_WORDS{"On", "Off"};

// Which Eulerian elements a surface processes (the COVER field of COUPLE and SURFINI): the elements inside or outside
// the closed volume of the surface, or those on both sides.
enum class CoverSide : std::uint8_t
{
    Inside,
    Outside,
    None
};
inline constexpr std::array<std::string_view, 3> COVER_SIDE_WORDS{"Inside", "Outside", "None"};

// The direction of a flow through a hole or a porous surface (the FLOW field of PORHOLE, PERMEAB, ...): both ways, only
// into the volume or only out of it.
enum class FlowDirection : std::uint8_t
{
    Both,
    In,
    Out
};
inline constexpr std::array<std::string_view, 3> FLOW_DIRECTION_WORDS{"Both", "In", "Out"};

// The size of a hole against the Euler mesh (the SIZE field of PORHOLE, PORFCPL and PORFGBG).
enum class HoleSize : std::uint8_t
{
    Small,
    Large
};
inline constexpr std::array<std::string_view, 2> HOLE_SIZE_WORDS{"Small", "Large"};

// A face of the box of an Euler mesh (the DIR field of BARRIER, FLOW, FLOWC, FLOWT and FLOWUDS).
enum class BoundaryFace : std::uint8_t
{
    NegativeX,
    PositiveX,
    NegativeY,
    PositiveY,
    NegativeZ,
    PositiveZ
};
inline constexpr std::array<std::string_view, 6> BOUNDARY_FACE_WORDS{"NegativeXDirection", "PositiveXDirection",
                                                                     "NegativeYDirection", "PositiveYDirection",
                                                                     "NegativeZDirection", "PositiveZDirection"};

} // namespace oor::model
