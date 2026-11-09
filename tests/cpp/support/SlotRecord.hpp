#pragma once

#include "oor/base/text/NumberParser.hpp"
#include "oor/deck/records/RecordText.hpp"
#include "oor/model/format/CardLayouts.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace oor::testing
{

// The kind of a slot text, as a card writer would have marked it: what tells two alternatives of one slot apart.
inline model::SlotKind slotKindOf(std::string_view text)
{
    if (text.empty())
        return model::SlotKind::Blank;
    if (base::NumberParser::parseInteger(text))
        return model::SlotKind::Integer;
    if (base::NumberParser::parseReal(text))
        return model::SlotKind::Real;
    return model::SlotKind::Text;
}

// The record of a card (one line of [model], with a new line) given by its neutral name and the texts of the slots
// its card reader reads: slot 1 is the first field after the name, and a card of several lines of eight fields goes
// on with slot 9. The card layout of the type (formats/oos-layouts.json) puts each slot at its place in the record.
// For the tests that change one field at a time by its slot number; a filled slot that the layout has no place for
// fails the test.
inline std::string slotRecord(std::string_view card, const std::vector<std::string>& slots)
{
    const model::CardLayout* layout = model::CardLayouts::standard()->find(card);
    INFO(card);
    REQUIRE(layout != nullptr);
    std::vector<model::SlotKind> kinds;
    kinds.reserve(slots.size());
    for (const std::string& slot : slots)
        kinds.push_back(slotKindOf(slot));
    std::vector<std::string> values{std::string(card)};
    std::vector<std::string> fields;
    std::string problem;
    const bool fits = layout->toValues(slots, kinds, fields, problem);
    INFO(problem);
    REQUIRE(fits);
    values.insert(values.end(), fields.begin(), fields.end());
    return deck::RecordText::format(values) + "\n";
}

} // namespace oor::testing
