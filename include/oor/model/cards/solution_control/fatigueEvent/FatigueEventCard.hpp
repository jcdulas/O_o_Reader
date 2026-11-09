#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace oor::model
{

// The longest event name: fields 3 to 9 of one line, 56 characters.

// One FTGEVNT card: a loading event of a fatigue analysis, the group of FTGLOAD cards that act at the same time (SOL
// 101 and 103: pseudo-static loads and modal participation factors; SOL 108 and 111: random, sine and static loads).
//
// Card layout:
//   FTGEVNT  ID  FLOAD1  FLOAD2  FLOAD3  FLOAD4  FLOAD5  FLOAD6  FLOAD7            fields 1 to 8
//            (blank)  FLOAD8  -etc.-                                                the next lines: fields 3 to 9
//            NAME  EVNTNAM                                                          the last line: fields 2 and 3 to 9
// The FLOADi fill fields 3 to 9 of the first line and of each continuation line, whose field 2 is blank, except that
// the line that holds NAME has no FLOAD. The event name is optional and has up to 56 characters (it is cut into pieces
// of 8 in the small format, and the lexer takes each piece for a field, so a piece that starts with a digit would be
// read as a number by the solver, which the check `compat.fatigue-event-name` reports; a blank inside the
// name ends it, so a blank is refused here). The format lists one set of fields for SOL 101, 103, 108 and 111,
// so there is one layout.
//
// The event number is unique among the FTGEVNT and FTGSEQ cards together: each type is its own table
// (IdSpace::PerCardType, the category FatigueEvent that an FTGSEQ names), and the check
// `compat.fatigue-event-id-repeated` reports a number that both types have. An FTGSEQ of SOL 112 holds a subcase number
// where an event is named, which resolves to nothing: the format reference says so on the FIDi field.
struct FatigueEventCard
{
    base::EntityId id = 0;
    std::vector<base::EntityId> fatigueLoadIds; // the FTGLOAD cards of the event, at least one
    std::string entryName;                      // the event name, empty when the card has no NAME line
};

} // namespace oor::model
