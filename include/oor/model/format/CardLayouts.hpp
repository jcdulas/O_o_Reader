#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace oor::model
{

// What a field of a card layout holds. It tells two alternative fields apart (a real angle or an integer frame ID in
// one place) and is shown in the card catalogue; the card reader checks the value itself.
enum class LayoutKind : std::uint8_t
{
    Integer,
    Real,
    Number, // an integer or a real, one meaning
    Text,
    Any // a number or a word (a list with Through in it, a value whose meaning depends on another field)
};

// The kind of the value a card writer put in a slot (FieldWriter): how the record writer picks between alternatives.
enum class SlotKind : std::uint8_t
{
    Blank,
    Integer,
    Real,
    Text
};

// The span of a text that runs to the last slot of the card (a span of 0 in the JSON text): the last fixed field of a
// card with no group, which the reader reads up to the last field used.
inline constexpr int SPAN_TO_END = 1 << 20;

// A field of the fixed part of a card: its name, the slot the card reader reads it from (FieldReader numbering: slot 1
// is the first field after the name, the slots of a continuation line follow), its kind and, when the code has one,
// its default as text (for the card catalogue).
//
// A text field may reserve several internal slots with `span`. Its complete text occupies the first slot and the
// remaining slots are padding. The compact model record carries the whole text as one value.
struct LayoutField
{
    std::string name;
    int slot = 0;
    LayoutKind kind = LayoutKind::Any;
    std::string defaultValue;
    int span = 1;
};

// A field of a repeated group: its place in one repetition (0 for the first slot of the period), and its span.
struct LayoutGroupField
{
    std::string name;
    int offset = 0;
    LayoutKind kind = LayoutKind::Any;
    int span = 1;
};

// The number of slots of a line of the old layout: the slots of line n (from 0) are 8n + 1 to 8n + 8.
inline constexpr int LINE_SLOTS = 8;

// A list of variable length at the end of every repetition of a group (the nodes after the weight and the components
// of a term): its fields follow each other, one item after the other, right after the fields of the repetition. A
// record gives the number of its items, then their fields. In the slots, an item goes on until a value fits the first
// field of the group better than the first field of the list (a real weight after integer nodes): the kinds of the two
// fields tell them apart.
struct LayoutNestedList
{
    std::string name;
    std::vector<LayoutGroupField> fields; // offsets 0 to fields.size() - 1
};

// A repeated group: repetition k (from 0) has its field at offset j in slot firstSlot + k * period + j. The slots of a
// period that no field names are padding of the old layout.
//
// A group whose repetitions do not fill the lines (three pairs to a line from the second slot of each line) gives
// `perLine`, the number of repetitions on a line: repetition k is then in slot
// firstSlot + (k / perLine) * LINE_SLOTS + (k % perLine) * period + j, the slots after them on a line being padding.
//
// A card may have more groups after the first one (CardLayout::nextGroups), one after the other. Each of them starts on
// the first slot of the line after the last slot of the group before it (on that slot when it starts a line), or, when
// it gives a `marker`, right after that word, which the old layout writes in the slot after the group before it (the
// record never writes it). How many repetitions a group has: `countField`, the name of a field given before it whose
// value is the number; `fixedCount`, a number the layout fixes (1: fields placed after a group); otherwise the record
// gives the number before the repetitions, except for the last group of a card with no sections, which runs to the end
// of the record.
struct LayoutGroup
{
    std::string name;
    int firstSlot = 1;
    int period = 1;
    int perLine = 0; // 0: the repetitions follow each other with no gap
    std::vector<LayoutGroupField> fields;
    std::string countField;
    int fixedCount = 0; // 0: not fixed
    std::string marker;
    std::optional<LayoutNestedList> list;

    // The slot of field `offset` of repetition `repetition`, for a group whose first repetition starts at `first`.
    [[nodiscard]] int slotOf(int first, int repetition, int offset) const noexcept;
};

// One kind of section of a card made of keyword lines: the word that opens the section in the old layout (at the key
// offset of its first line; empty for a section whose lines have no word), its fields, at their offset from the first
// slot of the section's first line (an offset of 8 or more is on a following line, whose key place is blank), and
// an optional list after them, whose `firstSlot` is the offset in the first line of its first repetition.
//
// A kind may give its own `keyOffset` (a key word in the second slot of its line while the others are in the first).
// A kind with `allLines` always takes every line its fields cover, blank lines included (otherwise a section ends with
// its last line that holds a value). A kind with no key word (key "") may have a number at the key place of its first
// line: a line whose key place is blank or holds a number, and that carries on no section, opens it. Its list may run
// on over the lines with no gap (perLine 0, numbers only), and may start on a line after the first (an offset of 8 or
// more).
struct LayoutSectionKind
{
    std::string key;
    std::string name;
    int keyOffset = 0;
    bool allLines = false;
    std::vector<LayoutGroupField> fields;
    std::optional<LayoutGroup> list;

    // The number of lines the fields cover (1 when there is none).
    [[nodiscard]] int lineCount() const noexcept;
};

// The keyword sections of a card: from `firstSlot` (the first slot of a line), or from the line after the last
// repetition of the group when firstSlot is 0, one section after the other, each starting on a new line with its key
// word at `keyOffset` of the line. A record gives each section as its key word, its fields, then for a list the number
// of its repetitions and their fields.
//
// `packed` sections do not start on new lines: each one starts in the slot after the section (or the group) before it,
// with its key word, its fields at their offset from the key word, then its list with no gap; in the slots, a list ends
// at the next key word.
struct LayoutSections
{
    std::string name;
    int firstSlot = 0;
    int keyOffset = 0;
    bool packed = false;
    std::vector<LayoutSectionKind> kinds;

    // The kind whose key is `word` (case is ignored); null when there is none.
    [[nodiscard]] const LayoutSectionKind* find(std::string_view word) const noexcept;
};

// What selects a form of a card (CardLayout::forms). Either a field of the form (a fixed field, the first repetition of
// a field of its group, or the name of its sections for the key word of the first section) whose value is one of
// `values` (numbers compared as numbers, words whatever their case, "" for a blank) or of one of `kinds` (a blank, an
// integer, a real or a word); or a slot of the old layout that is blank (`blankSlot`) or filled (`filledSlot`): a form
// that only the old layout has, which a record never selects.
struct FormCondition
{
    std::string field;
    std::vector<std::string> values;
    std::vector<SlotKind> kinds;
    int blankSlot = 0;
    int filledSlot = 0;
};

struct CardForm;

// The layout of one card type: how a record of a model file (the card name, then its fields in compact order) maps to
// the slots the card reader reads, and back.
//
// A record holds the fixed fields in order, then the repetitions of the group one after the other, then the sections.
// Two fixed fields on the same slot are alternatives (a real material angle or an integer material frame): a record
// gives one of them. When a card has a group and sections, the record gives the number of repetitions of the group
// before them.
//
// A card whose fields change with a value (a header record and a column record, a model word that changes the meaning
// of the fields) has `forms`, each a whole layout with the condition that selects it; the card itself then has no
// field. The first form whose condition holds is the one; the last form may have no condition (the form of every other
// record).
struct CardLayout
{
    std::string card; // the neutral name of the card type
    std::vector<LayoutField> fields;
    std::optional<LayoutGroup> group;
    std::vector<LayoutGroup> nextGroups; // the groups after the first one, in order
    std::optional<LayoutSections> sections;
    std::vector<CardForm> forms;
    std::string note; // a remark for the card catalogue

    // The slot texts of a record's values (the values after the card name): slots[i] is slot i + 1, blank where no
    // value goes. False, with `problem` set, when the record does not fit (too many values, two alternatives given).
    [[nodiscard]] bool toSlots(std::span<const std::string> values, std::vector<std::string>& slots,
                               std::string& problem) const;

    // The values of a record (after the card name) for the slot texts a card writer produced and their kinds; the
    // blank values at the end are dropped. False, with `problem` set, when a filled slot has no place in the layout.
    [[nodiscard]] bool toValues(std::span<const std::string> slots, std::span<const SlotKind> kinds,
                                std::vector<std::string>& values, std::string& problem) const;

    // The number of values the fixed fields take (alternatives take one value each).
    [[nodiscard]] std::size_t fixedCount() const noexcept { return fields.size(); }
};

// One form of a card: its name, its condition (none for the last form), its layout.
struct CardForm
{
    std::string name;
    std::optional<FormCondition> when;
    CardLayout layout;
};

// The card layout catalogue: one layout per card type. The standard one is formats/oos-layouts.json, compiled into the
// library (generated by tools/oos_layouts, then reviewed by hand; docs/model-format-cards.md is made from it).
class CardLayouts
{
public:
    // The standard catalogue, read once.
    [[nodiscard]] static const std::shared_ptr<const CardLayouts>& standard();

    // Reads a catalogue from its JSON text and checks it. Throws base::UsageError naming the card and the problem: a
    // missing or mistyped member, an unknown kind, a slot below 1, two fields on one slot that are not alternatives, a
    // name used twice, a group that overlaps the fixed fields or whose offsets leave its period, sections whose fields
    // take the place of the key word or of a fixed field, a group count that names no field before it, a form
    // condition on a field the form does not have.
    [[nodiscard]] static std::shared_ptr<const CardLayouts> fromJson(std::string_view text);

    // The layout of a card type by its neutral name; null when there is none.
    [[nodiscard]] const CardLayout* find(std::string_view card) const;
    [[nodiscard]] const std::vector<CardLayout>& layouts() const noexcept { return m_layouts; }

    // The name of a kind in the JSON text ("integer") and back.
    [[nodiscard]] static std::string_view kindName(LayoutKind kind) noexcept;
    [[nodiscard]] static std::optional<LayoutKind> kindOf(std::string_view name) noexcept;

private:
    std::vector<CardLayout> m_layouts;
    std::unordered_map<std::string, std::size_t> m_index;
};

// The text of formats/oos-layouts.json (compiled in by cmake/EmbedText.cmake).
[[nodiscard]] std::string_view cardLayoutsText() noexcept;

} // namespace oor::model
