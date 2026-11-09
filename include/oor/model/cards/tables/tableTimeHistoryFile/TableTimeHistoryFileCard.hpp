#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// How an axis of the table is interpolated (the XAXIS and YAXIS fields): linearly or in a logarithmic scale.
enum class TableTimeHistoryFileAxis : std::uint8_t
{
    Linear,
    Log
};
inline constexpr std::array<std::string_view, 2> TIME_HISTORY_FILE_AXIS_WORDS{"Linear", "Logarithmic"};

// The format of the external file the UID names (the TYPE field): a load-history file of channels or a data
// acquisition file.
enum class TableTimeHistoryFileFileType : std::uint8_t
{
    LoadHistoryFile,
    DataAcquisitionFile
};
inline constexpr std::array<std::string_view, 2> TIME_HISTORY_FILE_TYPE_NAMES{"LoadHistoryFile", "DataAcquisitionFile"};

// One end of a window of the signal: blank, a time, or the word SKIP.
struct TableTimeHistoryFileBound
{
    bool skip = false;          // the word SKIP: the pair is ignored
    std::optional<double> time; // the time, absent when the field is blank or SKIP

    [[nodiscard]] bool operator==(const TableTimeHistoryFileBound&) const noexcept = default;
};

// One pair of the filter fields (xi and xj): the start and the end of a part of the channel that is read (or, for a
// negative UID, left out). A blank start begins at the first point, a blank end goes on to the last one.
struct TableTimeHistoryFileWindow
{
    TableTimeHistoryFileBound start;
    TableTimeHistoryFileBound end;

    [[nodiscard]] bool operator==(const TableTimeHistoryFileWindow&) const noexcept = default;
};

// One time-history-file table card: a tabular function of time (a TABLED1 that a dynamic load such as a TLOAD1 can
// name) whose points come from a channel of an external file, a load-history file or a data acquisition file that a
// UDNAME names.
//
// Card layout (the card keyword first):
//   (card)  TID  XAXIS  YAXIS  UID  TYPE  CHAN  TOTIM  PUNCH                         fields 1 to 8
//           xi1  xj1  xi2  xj2  xi3  xj3  -etc.-  ENDT                               the next lines: four pairs on each
// The XAXIS and YAXIS are LINEAR or LOG (LINEAR by default), the TYPE the word of a load-history file (the default) or
// of a data acquisition file, PUNCH YES or NO (NO); these static defaults are stored resolved and the writer leaves
// them blank. UID is the number of a UDNAME and is not zero: a negative UID makes the pairs the parts of the file to
// leave out instead of the parts to read (the sign stays on the card and the reference is to its absolute value). CHAN
// is the channel of a load-history file and is left blank for a data acquisition file; its default, 1, depends on the
// type, so a blank stays blank on the card (a card of a data acquisition file must leave it blank, so a 1 written there
// would be wrong). TOTIM, the total time of the signal, is optional and positive.
//
// The pairs are optional: a card without continuation lines takes the whole channel. When there are some, the word ENDT
// ends them, in the field after the last pair or in the field of its end, and nothing may follow it; the
// card keeps the pairs and writes ENDT after the last one. Any field of a pair may be the word SKIP; a blank
// start or end is allowed and a pair of two blank fields kept as a pair. The values of the pairs do not
// decrease, in the order of the file.
//
// The card is a table of the space the TABLED1 to TABLED5 share (IdSpace::DynamicTable, category Table), so a TLOAD1
// that names it finds it, and a number may not be a TABLED1's too, as for a TABLUDS.
struct TableTimeHistoryFileCard
{
    base::EntityId id = 0;
    TableTimeHistoryFileAxis xAxisKind = TableTimeHistoryFileAxis::Linear;
    TableTimeHistoryFileAxis yAxisKind = TableTimeHistoryFileAxis::Linear;
    base::EntityId externalFileId = 0; // a UDNAME; negative: the pairs are the parts to leave out
    TableTimeHistoryFileFileType fileType = TableTimeHistoryFileFileType::LoadHistoryFile;
    base::EntityId channel = 0; // the channel of a load-history file, 0 when blank
    std::optional<double> totalTime;
    bool punch = false; // PUNCH YES: the equivalent TABLED1 is written to the punch file
    std::vector<TableTimeHistoryFileWindow> windows;
};

} // namespace oor::model
