#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The pair of axes of a coordinate system that span the mirror plane of an MDMIR2. The enumerators are numbered in the
// order of MODULE_MIRROR2_AXES_WORDS. The format reference lists the pair in both orders (XY and YX, ...) and gives no
// difference between them, so each is kept as written.
enum class ModuleMirror2Axes : std::uint8_t
{
    Xy,
    Xz,
    Yz,
    Yx,
    Zx,
    Zy
};

inline constexpr std::array<std::string_view, 6> MODULE_MIRROR2_AXES_WORDS{"PlaneXy", "PlaneXz", "PlaneYz",
                                                                           "PlaneYx", "PlaneZx", "PlaneZy"};

// One MDMIR2 card: a mirror image of a module in a plane given by two axes of a coordinate system. The move takes
// effect only when an MDMOVE names its MVID, in its first MVID field (a rule between cards).
//
// Card layout: MDMIR2  MVID  CID  RID  AXES            fields 1 to 4
// CID is a coordinate system of the main Bulk Data, blank or 0 for the basic system; the plane passes through its
// origin when RID is blank. RID is the GRID or POINT of module 0 the plane passes through, blank or 0 for the origin
// of CID. (The format reference description says an ID or coordinates of the point, and types the
// field as an integer: the card reads an integer.) AXES has no default, so it is required.
//
// The MVID is one number of the move kinds (see ModuleTranslationCard): IdSpace::PerCardType in the category
// ModuleMove.
struct ModuleMirror2Card
{
    base::EntityId id = 0;            // the move, referenced by an MDMOVE
    base::EntityId frameId = 0;       // the coordinate system, 0 for the basic system
    base::EntityId referenceNode = 0; // the point the plane passes through, 0 for the origin of CID
    ModuleMirror2Axes axes = ModuleMirror2Axes::Xy;
};

} // namespace oor::model
