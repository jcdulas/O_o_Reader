#pragma once

#include <string>
#include <string_view>

namespace oor::model::slot_trace
{

// The instrumentation that tools/oos_layouts/generate_layouts.py uses to find the slots each card type reads and writes
// (the card layout catalogue, formats/oos-layouts.json). It is active only in a build configured with the CMake option
// OOR_TRACE_SLOTS (the macro of the same name); the calls are compiled out otherwise. Each process writes what it saw,
// once per distinct line, to a file of the folder named by the environment variable OOR_SLOT_TRACE_DIR when it ends.

// A card reader looked at slot `slot` of a card of `cardType`: with `accessor` "integer", "real" or "number" and the
// field name the reader gives, or "text" (no name) for a look at the text.
void noteRead(std::string_view cardType, int slot, std::string_view accessor, std::string_view name);
// A card reader read the text spread over slots `first` to `last` (FieldReader::spanText).
void noteSpan(std::string_view cardType, int first, int last);
// A card writer wrote a card of `cardType`; `slots` lists the slots it filled as "slot:kind" items separated by commas,
// kind i (integer), r (real) or t (text).
void noteWrite(std::string_view cardType, const std::string& slots);

} // namespace oor::model::slot_trace
