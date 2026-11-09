#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsCard.hpp"

#include <vector>

namespace oor::model
{

// A connection of an SECONCT: a grid or scalar point of superelement SEIDA and the point of SEIDB it is joined to, or
// two THRU ranges that join their points one to one (101 THRU 110 to 201 THRU 210 joins 101 to 201, 102 to 202, ...).
struct SuperelementConnectionPair
{
    IdRun a; // the points of SEIDA: one point (first == last) or a THRU range
    IdRun b; // the points of SEIDB, as many as the points of `a`

    [[nodiscard]] bool operator==(const SuperelementConnectionPair&) const noexcept = default;
};

// One SECONCT card: the boundary connection between two superelements, point by point.
//
// Card layout: SECONCT  SEIDA  SEIDB  TOL  LOC
//                      GIDA1  GIDB1  GIDA2  GIDB2  GIDA3  GIDB3  GIDA4  GIDB4
//                      -etc.-
// or:         SECONCT  SEIDA  SEIDB  TOL  LOC
//                      GIDA1  "THRU"  GIDA2  GIDB1  "THRU"  GIDB2
// The connections start on the first continuation line (field 9) and run over as many lines as needed; a card that puts
// the first of them in fields 5 to 8 is read the same, and written from field 9. A connection is one pair of points or
// one pair of THRU ranges of the same length, and the card keeps them in the order they are written. SEIDA is a
// partitioned superelement and SEIDB another one or 0, the residual structure; the points of each side are those of
// that superelement (a part has its own ID space, a list superelement shares the one of the main Bulk Data). TOL is the
// location tolerance; 0.0 is the default and takes the one of the SEBULK of the superelement, which takes CONFAC. LOC
// is YES (the default: the points must be within TOL of each other) or NO (connected wherever they are). The card
// belongs in the main Bulk Data and is a fatal error after a BEGIN SUPER.
struct SuperelementConnectionCard
{
    base::EntityId superelementIdA = 0;
    base::EntityId superelementIdB = 0; // 0 is the residual structure
    double tolerance = 0.0;
    SuperelementBulkOptionsLocation location = SuperelementBulkOptionsLocation::Yes;
    std::vector<SuperelementConnectionPair> pairs;

    [[nodiscard]] bool operator==(const SuperelementConnectionCard&) const noexcept = default;
};

} // namespace oor::model
