#pragma once

#include "oor/model/format/CardLayouts.hpp"

#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// Field ownership when a card layout is walked in slot order or compact record order.

// The kind of an internal slot as its neutral text shows it: blank, integer, real or text.
[[nodiscard]] SlotKind slotKindOf(std::string_view text) noexcept;

// One text field that the old layout spreads over several slots (a field with a span), placed somewhere in a card.
struct SpanPlace
{
    int firstSlot = 0; // slot 1 first
    int span = 1;
};

// A count field of a card whose number is more than the slots of the card can hold (a repetition takes one slot at
// least): CardLayout::toValues would go through every repetition, so the converter reads the slots with a smaller
// count and gives the record the number as written.
struct LargeCount
{
    std::string field; // the name of the count field
    int slot = 0;      // the slot it was read from
};

// Where the filled slots of one card are in its layout, found the way CardLayout::toValues finds them: the form the
// slots select, the fixed fields, the repetitions of the groups (perLine, groups after the first, nested lists), then
// the keyword sections (line by line, or packed one after the other).
struct SlotPlaces
{
    // The layout the slots were placed in: the layout itself, or the form of it that the slots select; null when
    // they select no form (CardLayout::toValues then refuses the card).
    const CardLayout* layout = nullptr;
    // The filled slots (numbers, slot 1 first) that no field covers: a padding slot of the old layout that a deck
    // filled anyway, or a line of sections that no key word opens. CardLayout::toValues refuses a card with such a
    // slot; the converter reports and blanks them.
    std::vector<int> withoutField;
    // The text fields spread over several slots: the fixed fields with a span (a span to the end of the card ends at
    // the last slot), the group fields with a span in every repetition the slots reach, and the section fields and
    // list fields with a span in the sections found.
    std::vector<SpanPlace> spans;
    // The kind of the field that holds each filled slot the layout places (a fixed field, a field of a group, of a
    // section or of a list); a slot of two alternative fixed fields has the kind of the one its text fits best.
    std::map<int, LayoutKind> fieldKinds;
    // The field that owns each filled slot. A section key or a group marker has an empty field name; an absent
    // entry is a slot the layout does not place. Pieces of a spanning text all carry the same field name.
    std::map<int, std::string> fieldNames;
    // The count fields whose number is more than the slots of the card can hold.
    std::vector<LargeCount> largeCounts;
};

// The places of the slots of a card (slots[i] is slot i + 1, blank where the card has nothing).
[[nodiscard]] SlotPlaces placeSlots(const CardLayout& layout, std::span<const std::string> slots);

// The layout of a record (its values after the card name): the layout itself, or its form that the record takes (the
// first whose condition on a field holds, as CardLayout::toSlots picks it); null when it takes none.
[[nodiscard]] const CardLayout* recordLayout(const CardLayout& layout, std::span<const std::string> values);

// The name of the layout field of every value of a record, in the order of CardLayout::toSlots: the fixed fields, the
// fields of each repetition of the groups and of their nested lists, the fields of the sections and of their lists.
// A value that is no field (a count of repetitions, the key word of a section) and a value past the end of the layout
// have an empty name.
[[nodiscard]] std::vector<std::string> recordFieldNames(const CardLayout& layout, std::span<const std::string> values);

// A value of a record that gives a number of repetitions (CardLayout::toSlots repeats that many times).
struct RecordCount
{
    std::size_t value = 0; // its index among the values of the record
    std::string field;     // the name of its count field; empty for a count the record gives before the repetitions
};

// The values of a record that give a number of repetitions, found as CardLayout::toSlots finds them: the count fields
// of the groups, the counts the record gives (of a group before sections, of a nested list, of the list of a section).
[[nodiscard]] std::vector<RecordCount> recordCounts(const CardLayout& layout, std::span<const std::string> values);

} // namespace oor::model
