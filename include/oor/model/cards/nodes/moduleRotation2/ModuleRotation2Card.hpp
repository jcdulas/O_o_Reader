#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The axis of a coordinate system that an MDROT2 turns about. The three words that start with Minus are the negative
// side of the axis. The enumerators are numbered in the order of MODULE_ROTATION2_AXIS_WORDS.
enum class ModuleRotation2Axis : std::uint8_t
{
    X,
    Y,
    Z,
    MinusX, // MX
    MinusY, // MY
    MinusZ  // MZ
};

inline constexpr std::array<std::string_view, 6> MODULE_ROTATION2_AXIS_WORDS{"XAxis",      "YAxis",      "ZAxis",
                                                                             "MinusXAxis", "MinusYAxis", "MinusZAxis"};

// One MDROT2 card: a rotation of a module about an axis of a coordinate system, optionally turning about a reference
// point. The move takes effect only when an MDMOVE names its MVID.
//
// Card layout: MDROT2  MVID  CID  RID  AXIS  MAG            fields 1 to 5
// CID is a coordinate system of the main Bulk Data, blank or 0 for the basic system; the module turns about its origin
// when RID is blank. RID is the GRID or POINT the module turns about (module 0), blank or 0 for the origin of CID. AXIS
// is the axis of CID, one of X, Y, Z, MX, MY and MZ, and has no default, so it is required. MAG is the angle in
// degrees, any real but zero, also required. (The format reference names the fourth field RIDS in its layout and RID in
// its description: it is the reference point.)
//
// The MVID is one number of the move kinds (see ModuleTranslationCard): IdSpace::PerCardType in the category
// ModuleMove.
struct ModuleRotation2Card
{
    base::EntityId id = 0;            // the move, referenced by an MDMOVE
    base::EntityId frameId = 0;       // the coordinate system, 0 for the basic system
    base::EntityId referenceNode = 0; // the reference point, 0 for the origin of CID
    ModuleRotation2Axis axis = ModuleRotation2Axis::X;
    double magnitude = 0.0; // the angle in degrees, never zero
};

} // namespace oor::model
