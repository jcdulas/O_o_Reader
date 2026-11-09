#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <string>

namespace oor::model
{

// The longest name a UDNAME may hold. The format reference gives 255 characters (four lines of data in fields 2 to 9)
// for a file of a fatigue analysis or of a material model (MATDIGI) and no limit for the file name of an HDF5DA, whose
// The format computes at most 64 lines and 4095 characters for a path (the limit of a Linux path); it asks 128 for a
// .rsp file and 64 for the CONFIG of an HDF5DA. The card cannot know what names it, so it accepts the longest and the
// shorter limits are left to the cards that name it.
inline constexpr std::size_t USER_FILE_NAME_MAX_LENGTH = 4095;

// One UDNAME card: the name of a file that other entries name by the UDID (an FTGLOAD, a TableTimeHistoryFile, a
// MATDIGI or an HDF5DA).
//
// Card layout:
//   UDNAME  UDID                                               fields 1 and 2
//           NAME                                               fields 2 to 9 of the continuation line, 64 characters
// UDID is alone on the first line and the NAME is on the next ones, in fields 2 to 9 of each, as for a
// TableTimeHistoryFile. The card reads that, and also a name that starts in field 3 of the first line (a free format
// deck often writes `UDNAME,10,C:/file.h5`); it always writes the layout of the format.
//
// The name has no blank (the format refuses a blank inside it), so the pieces of the small format fields are
// put together without a separator; it keeps the case and the characters as written. A comma, a dollar sign, a percent
// sign, a bar and a quote are refused because the solver drops or cuts the line that has one and the free
// format would not read them back. The ID is unique among the UDNAME cards (IdSpace::PerCardType, the category
// UserDefinedName that the cards naming a file use).
struct UserFileNameCard
{
    base::EntityId id = 0;
    std::string entryName;
};

} // namespace oor::model
