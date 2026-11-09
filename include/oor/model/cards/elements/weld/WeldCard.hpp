#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace oor::model
{

// One CWELD card: a weld or fastener between two surface patches, a point and a patch, or two points. The format of
// the rest of the card depends on TYPE (field 4):
//
//   PARTPAT / ELPAT / ELEMID:
//     CWELD  EWID  PWID  GS  TYPE  GA  GB      MCID
//            IDA   IDB                                   IDA and IDB: PSHELL properties (PARTPAT) or shell elements
//            XS    YS    ZS                              XS YS ZS: PARTPAT and ELPAT only (ELEMID has no third line)
//   GRIDID:
//     CWELD  EWID  PWID  GS  GRIDID  GA  GB  SPTYP  MCID
//            GA1   GA2   GA3  GA4  GA5  GA6  GA7  GA8
//            GB1   GB2   GB3  GB4  GB5  GB6  GB7  GB8
//   ALIGN:
//     CWELD  EWID  PWID      ALIGN  GA  GB      MCID
//
// `patchIdA`/`patchIdB` hold PIDA/PIDB or SHIDA/SHIDB; they and the members of the other formats stay at their blank
// value when the format does not use them. A blank MCID is -1 (the format reference default). GS, GA, GB, IDB (ELEMID:
// a point-to-patch connection) and the list entries may be blank (0).
struct WeldCard
{
    static constexpr double NO_COORDINATE = std::numeric_limits<double>::quiet_NaN();
    static constexpr base::EntityId NO_MAT_FRAME_ID = -1;
    static constexpr std::size_t GRID_COUNT = 8;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    base::EntityId spotNode = 0;
    std::string kind = "PartPatch"; // PartPatch, ElemPatch, ElemIdPatch, NodeIdPatch or AlignedNodes
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    std::string patchShapes; // GRIDID only: QQ, QT, TT, TQ, Q or T; empty otherwise
    base::EntityId matFrameId = NO_MAT_FRAME_ID;
    base::EntityId patchIdA = 0;                                                    // PIDA or SHIDA
    base::EntityId patchIdB = 0;                                                    // PIDB or SHIDB
    std::array<double, 3> location = {NO_COORDINATE, NO_COORDINATE, NO_COORDINATE}; // XS YS ZS
    std::array<base::EntityId, GRID_COUNT> patchNodesA = {};                        // GRIDID: GA1 to GA8
    std::array<base::EntityId, GRID_COUNT> patchNodesB = {};                        // GRIDID: GB1 to GB8

    [[nodiscard]] bool hasLocation() const noexcept
    {
        return !std::isnan(location[0]) || !std::isnan(location[1]) || !std::isnan(location[2]);
    }
};

} // namespace oor::model
