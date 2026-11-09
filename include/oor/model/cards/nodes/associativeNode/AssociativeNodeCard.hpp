#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <string>

namespace oor::model
{

// One GRIDA card: an associative grid point of the COMBINE step of a Part Assembly Analysis (PAA). It lets the Bulk
// Data of the assembly name a grid point or a scalar point of a Part that is being combined: the program connects the
// GRIDA to that point and treats it as a point defined in the current run.
//
// Card layout (the name is on the continuation line, in the fields the format spans):
//   GRIDA  GID  ORIG_GID
//          PARTNAME
//
// GID is the number the point has in the current run, in the node ID space (a GRID or an SPOINT cannot have it).
// ORIG_GID is the number of the grid point in the Part. It exists in that Part, which is not a model of this
// run (a Part is a different run), so it is a plain ID and no reference is visited. PARTNAME is the name of the Part,
// up to 64 characters with no blank (the format's C64): a word of one field in the free and large formats, written in
// pieces of eight characters over the eight fields of the continuation line in the small format, as the label of a
// monitor point is.
struct AssociativeNodeCard
{
    base::EntityId id = 0;
    base::EntityId origNodeId = 0; // the point in the Part
    std::string partitionName;
};

} // namespace oor::model
