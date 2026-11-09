#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The type of connection of an MDWELD (field 4), which decides the layout of the rest of the card. The enumerators are
// numbered in the order of MODULE_WELD_TYPE_WORDS.
enum class ModuleWeldType : std::uint8_t
{
    PartPatch,    // PARTPAT: two patches, each defined by a PSHELL property (PIDA in MODA, PIDB in MODB)
    ElementPatch, // ELPAT: two patches, each defined by a shell element (SHIDA in MODA, SHIDB in MODB)
    ElementId,    // ELEMID: one shell element on each patch (or one patch and the point GS)
    GridId,       // GRIDID: two lists of grid points, one for each patch (GAi in MODA, GBi in MODB)
    Align         // ALIGN: two vertex grid points, one for each patch (GA in MODA, GB in MODB)
};

inline constexpr std::array<std::string_view, 5> MODULE_WELD_TYPE_WORDS{"PartPatch", "ElemPatch", "ElemIdPatch",
                                                                        "NodeIdPatch", "AlignedNodes"};

// The types of the two patches of a GRIDID connection (SPTYP, field 7): Q is a quadrilateral and T a triangle, one
// letter for a patch and two for the two patches (a single letter connects a point to a patch). The enumerators are
// numbered in the order of MODULE_WELD_PATCH_TYPE_WORDS.
enum class ModuleWeldPatchTypes : std::uint8_t
{
    QuadQuad,         // QQ
    TriangleTriangle, // TT
    QuadTriangle,     // QT
    TriangleQuad,     // TQ
    Quad,             // Q: a point to a quadrilateral patch
    Triangle          // T: a point to a triangular patch
};

inline constexpr std::array<std::string_view, 6> MODULE_WELD_PATCH_TYPE_WORDS{
    "QuadQuadPatch", "TriaTriaPatch", "QuadTriaPatch", "TriaQuadPatch", "QuadPatch", "TriaPatch"};

// One MDWELD card: a weld or fastener that connects two surface patches, a point and a patch, or two points, between
// two modules (the multi-module counterpart of the CWELD, whose rules apply). A module is a BEGIN MODULE section of
// the deck, a part of the deck; module 0 (a blank field) is the main Bulk Data.
//
// Card layout, the first line and the fields 13 and 14 that every format has:
//   MDWELD  EWID  PWID  GS  TYPE  GA  GB  (SPTYP)  MCID           fields 1 to 8 (SPTYP: GRIDID only)
//           ...                                   MODA  MODB     fields 13 and 14
// and what depends on TYPE (a blank cell is not used and must be blank):
//   PARTPAT   fields 9 and 10: PIDA  PIDB, fields 17 to 19: XS  YS  ZS
//   ELPAT     fields 9 and 10: SHIDA SHIDB, fields 17 to 19: XS  YS  ZS
//   ELEMID    fields 9 to 11: SHIDA  SHIDB  MODS (no line of coordinates)
//   GRIDID    fields 17 to 24: GA1 to GA8, fields 25 to 32: GB1 to GB8; SPTYP is field 7
//   ALIGN     GS (field 3) is blank, GA and GB are the vertex points; nothing else
// `ida` and `idb` hold PIDA and PIDB or SHIDA and SHIDB. The members of the formats a TYPE does not have stay blank.
//
// A blank MCID is -1 (the format reference default: the element system of the weld; the format takes -1 or more, or a
// blank). A blank module is 0. PWID names a PWELD of the main Bulk Data. The grid points GS, GA and GB are in the main
// Bulk Data (module 0) except where the format reference says otherwise: for ELEMID GS is in the module MODS (the
// format reference says so for a point to patch connection, which is an ELEMID with a blank SHIDB) and for ALIGN GA is
// in MODA and GB in MODB; the points GAi and GBi of a GRIDID are in MODA and MODB. XS, YS and ZS locate the weld in the
// basic system, are all given or all blank and are required when neither GS nor GA is given (as for the CWELD:
// `location` is empty when blank). The card belongs in the main Bulk Data (a rule of the deck, not of the
// card).
struct ModuleWeldCard
{
    static constexpr base::EntityId NO_MAT_FRAME_ID = -1;
    static constexpr std::size_t GRID_COUNT = 8;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    base::EntityId spotNode = 0;
    ModuleWeldType type = ModuleWeldType::PartPatch;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    std::optional<ModuleWeldPatchTypes> patchTypes; // GRIDID only; empty otherwise
    base::EntityId matFrameId = NO_MAT_FRAME_ID;
    base::EntityId patchIdA = 0;     // PIDA or SHIDA
    base::EntityId patchIdB = 0;     // PIDB or SHIDB; blank for an ELEMID that connects the point GS to the patch A
    base::EntityId spotModuleId = 0; // the module of GS, ELEMID only
    base::EntityId moduleIdA = 0;    // the module of the patch A
    base::EntityId moduleIdB = 0;    // the module of the patch B
    std::optional<std::array<double, 3>> location;      // XS YS ZS: PARTPAT and ELPAT only
    std::array<base::EntityId, GRID_COUNT> nodesA = {}; // GRIDID: GA1 to GA8
    std::array<base::EntityId, GRID_COUNT> nodesB = {}; // GRIDID: GB1 to GB8
};

} // namespace oor::model
