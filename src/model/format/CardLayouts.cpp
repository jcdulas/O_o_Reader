#include "oor/model/format/CardLayouts.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/Json.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <utility>

namespace oor::model
{

namespace
{

// The format version of the catalogue this code reads.
constexpr int FORMAT_VERSION = 3;

struct KindName
{
    LayoutKind kind;
    std::string_view name;
};

constexpr std::array<KindName, 5> KIND_NAMES = {{
    {LayoutKind::Integer, "integer"},
    {LayoutKind::Real, "real"},
    {LayoutKind::Number, "number"},
    {LayoutKind::Text, "text"},
    {LayoutKind::Any, "any"},
}};

// The names of the kinds of value a form condition tests.
struct SlotKindName
{
    SlotKind kind;
    std::string_view name;
};

constexpr std::array<SlotKindName, 4> SLOT_KIND_NAMES = {{
    {SlotKind::Blank, "blank"},
    {SlotKind::Integer, "integer"},
    {SlotKind::Real, "real"},
    {SlotKind::Text, "text"},
}};

// How well a field of `kind` takes a value a card writer wrote as `slot`: the alternative with the best score gets the
// value (0: it does not fit).
int fitness(LayoutKind kind, SlotKind slot) noexcept
{
    constexpr int EXACT = 3;
    constexpr int NUMBER = 2;
    constexpr int ANY = 1;
    switch (slot)
    {
    case SlotKind::Integer:
        return kind == LayoutKind::Integer ? EXACT : kind == LayoutKind::Number ? NUMBER : kind == LayoutKind::Any;
    case SlotKind::Real:
        return kind == LayoutKind::Real ? EXACT : kind == LayoutKind::Number ? NUMBER : kind == LayoutKind::Any;
    case SlotKind::Text:
        return kind == LayoutKind::Text ? EXACT : kind == LayoutKind::Any ? ANY : 0;
    case SlotKind::Blank:
        break;
    }
    return 0;
}

// Two kinds a value tells apart (integer and real, or a number and a text): alternatives on one slot, or the first
// field of a group and the first field of its nested list.
bool kindsDiffer(LayoutKind a, LayoutKind b) noexcept
{
    return a != b && a != LayoutKind::Any && b != LayoutKind::Any &&
           !(a == LayoutKind::Number && b != LayoutKind::Text) && !(b == LayoutKind::Number && a != LayoutKind::Text);
}

// The first slot of the line that holds `slot`.
int lineStartOf(int slot) noexcept
{
    return (slot - 1) / LINE_SLOTS * LINE_SLOTS + 1;
}

// The first slot of a line at `slot` or after it.
int lineStartAtOrAfter(int slot) noexcept
{
    return lineStartOf(slot) == slot ? slot : lineStartOf(slot) + LINE_SLOTS;
}

// The place of `slot` in a group whose first repetition starts at `first`: the repetition and the index of the group
// field there; none for a slot before the group or in its padding.
std::optional<std::pair<std::size_t, std::size_t>> placeInGroup(const LayoutGroup& group, int first, int slot)
{
    const int relative = slot - first;
    if (relative < 0)
        return std::nullopt;
    int repetition = 0;
    int offset = 0;
    if (group.perLine == 0)
    {
        repetition = relative / group.period;
        offset = relative % group.period;
    }
    else
    {
        const int inLine = relative % LINE_SLOTS;
        const int position = inLine / group.period;
        if (position >= group.perLine)
            return std::nullopt;
        repetition = relative / LINE_SLOTS * group.perLine + position;
        offset = inLine % group.period;
    }
    for (std::size_t field = 0; field < group.fields.size(); ++field)
    {
        const LayoutGroupField& candidate = group.fields[field];
        if (offset >= candidate.offset && offset < candidate.offset + candidate.span)
            return std::pair{static_cast<std::size_t>(repetition), field};
    }
    return std::nullopt;
}

// The last offset a repetition of `group` takes (the end of its last field).
int lastOffsetOf(const LayoutGroup& group) noexcept
{
    int last = 0;
    for (const LayoutGroupField& field : group.fields)
        last = std::max(last, field.offset + field.span - 1);
    return last;
}

// The slots the repetitions of a line take, from the first slot of the first one to the last field of the last one,
// for a group that gives perLine.
int lineLength(const LayoutGroup& group) noexcept
{
    return (group.perLine - 1) * group.period + lastOffsetOf(group) + 1;
}

// A field whose value is a number (never a key word).
bool isNumeric(LayoutKind kind) noexcept
{
    return kind == LayoutKind::Integer || kind == LayoutKind::Real || kind == LayoutKind::Number;
}

// The kind of a value as a record writes it.
SlotKind kindOfText(std::string_view text) noexcept
{
    if (text.empty())
        return SlotKind::Blank;
    if (base::NumberParser::parseInteger(text))
        return SlotKind::Integer;
    if (base::NumberParser::parseReal(text))
        return SlotKind::Real;
    return SlotKind::Text;
}

// A count of repetitions in a record: blank is 0. Nullopt when the text is not a count.
std::optional<std::size_t> countOf(const std::string& text)
{
    if (text.empty())
        return 0;
    const std::optional<std::int64_t> count = base::NumberParser::parseInteger(text);
    if (!count || *count < 0)
        return std::nullopt;
    return static_cast<std::size_t>(*count);
}

// A count of repetitions larger than the values present (the values of a record, the slots of a card) is refused
// before any repetition is placed: every repetition takes one value at least, so such a count cannot be honoured, and a
// count of two billion would take all the memory of the machine.
bool countFits(std::size_t count, std::size_t present, const std::string& what, const std::string& card,
               std::string& problem)
{
    if (count <= present)
        return true;
    problem = "the number of " + what + " of a " + card + " (" + std::to_string(count) + ") is more than the " +
              std::to_string(present) + " values given";
    return false;
}

std::string countText(std::size_t count)
{
    return count == 0 ? std::string{} : std::to_string(count);
}

// The groups of a layout in order: the group, then the next groups.
std::vector<const LayoutGroup*> groupsOf(const CardLayout& layout)
{
    std::vector<const LayoutGroup*> groups;
    if (layout.group)
        groups.push_back(&*layout.group);
    for (const LayoutGroup& group : layout.nextGroups)
        groups.push_back(&group);
    return groups;
}

// True when a record gives the number of repetitions of group `index` before them: the layout does not know it (no
// count field, no fixed count) and the group is not the last part of the record.
bool recordGivesCount(const CardLayout& layout, std::size_t index)
{
    const std::vector<const LayoutGroup*> groups = groupsOf(layout);
    const LayoutGroup& group = *groups[index];
    const bool last = index + 1 == groups.size();
    return group.fixedCount == 0 && group.countField.empty() && (!last || layout.sections.has_value());
}

// The layouts with more than one group, or a group with a nested list, go through the slots one group after the other.
bool readsGroupsInTurn(const CardLayout& layout) noexcept
{
    return !layout.nextGroups.empty() || (layout.group && layout.group->list);
}

// Where a form condition looks: the index of the value in a record and the slot in the old layout.
struct ConditionPlace
{
    std::size_t value = 0;
    int slot = 0;
};

// The place of the field a form condition names: a fixed field, a field of the first repetition of the group, or the
// sections (their first key word).
std::optional<ConditionPlace> conditionPlace(const CardLayout& layout, std::string_view name)
{
    for (std::size_t index = 0; index < layout.fields.size(); ++index)
    {
        if (layout.fields[index].name == name)
            return ConditionPlace{index, layout.fields[index].slot};
    }
    if (layout.group)
    {
        const LayoutGroup& group = *layout.group;
        const std::size_t first = layout.fields.size() + (recordGivesCount(layout, 0) ? 1 : 0);
        for (std::size_t index = 0; index < group.fields.size(); ++index)
        {
            if (group.fields[index].name == name)
                return ConditionPlace{first + index, group.slotOf(group.firstSlot, 0, group.fields[index].offset)};
        }
    }
    if (layout.sections && !layout.group && layout.sections->name == name)
        return ConditionPlace{layout.fields.size(), layout.sections->firstSlot + layout.sections->keyOffset};
    return std::nullopt;
}

// True when a value is one of the values of a condition: numbers compared as numbers, neutral text matched exactly.
bool valueMatches(std::string_view expected, std::string_view text)
{
    if (expected.empty() || text.empty())
        return expected.empty() && text.empty();
    const std::optional<double> left = base::NumberParser::parseReal(expected);
    const std::optional<double> right = base::NumberParser::parseReal(text);
    if (left && right)
        return *left == *right;
    return (expected == text);
}

bool conditionHolds(const FormCondition& condition, std::string_view text, SlotKind kind)
{
    for (const std::string& value : condition.values)
    {
        if (valueMatches(value, text))
            return true;
    }
    return std::find(condition.kinds.begin(), condition.kinds.end(), kind) != condition.kinds.end();
}

// Reading the JSON text, with messages that name the card.
class CatalogueReader
{
public:
    std::vector<CardLayout> read(std::string_view text)
    {
        const base::JsonValue root = base::JsonValue::parse(text);
        const base::JsonValue* version = root.member("formatVersion");
        if (version == nullptr || version->asNumber("formatVersion") != FORMAT_VERSION)
            fail("the catalogue", "formatVersion is not " + std::to_string(FORMAT_VERSION));
        const base::JsonValue* cards = root.member("cards");
        if (cards == nullptr)
            fail("the catalogue", "it has no \"cards\" object");
        std::vector<CardLayout> layouts;
        for (const auto& [name, value] : cards->asObject("cards"))
            layouts.push_back(readCard(name, value));
        return layouts;
    }

private:
    // Where a group stands: the first group (with its firstSlot), a group after it, or the list of a section (with
    // its offset).
    enum class GroupPlace
    {
        First,
        Next,
        SectionList
    };

    [[noreturn]] static void fail(const std::string& where, const std::string& problem)
    {
        throw base::UsageError("card layout catalogue, " + where + ": " + problem);
    }

    static const base::JsonValue& required(const base::JsonValue& object, std::string_view member,
                                           const std::string& where)
    {
        const base::JsonValue* value = object.member(member);
        if (value == nullptr)
            fail(where, "\"" + std::string(member) + "\" is missing");
        return *value;
    }

    static int integerOf(const base::JsonValue& value, const std::string& what, const std::string& where)
    {
        if (!value.isNumber())
            fail(where, what + " is not a number");
        const double number = value.asNumber(what);
        if (number != static_cast<double>(static_cast<int>(number)))
            fail(where, what + " is not an integer");
        return static_cast<int>(number);
    }

    // The span of a field; a fixed field may give 0 (`toEnd`): it runs to the last slot of the card.
    static int spanOf(const base::JsonValue& object, const std::string& where, bool toEnd = false)
    {
        const base::JsonValue* span = object.member("span");
        if (span == nullptr)
            return 1;
        const int slots = integerOf(*span, "span", where);
        if (slots == 0 && toEnd)
            return SPAN_TO_END;
        if (slots < 1)
            fail(where, "the span is below 1");
        return slots;
    }

    static LayoutKind kindOf(const base::JsonValue& object, const std::string& where)
    {
        const std::string& name = required(object, "kind", where).asText(where + " kind");
        const std::optional<LayoutKind> kind = CardLayouts::kindOf(name);
        if (!kind)
            fail(where, "unknown kind \"" + name + "\" (integer, real, number, text or any)");
        return *kind;
    }

    static CardLayout readCard(const std::string& card, const base::JsonValue& value)
    {
        const std::string where = "card " + card;
        if (!value.isObject())
            fail(where, "the layout is not an object");
        const base::JsonValue* forms = value.member("forms");
        if (forms == nullptr)
            return readLayout(card, value, where);
        const base::JsonValue* fields = value.member("fields");
        if ((fields != nullptr && !fields->asArray(where + " fields").empty()) || value.member("group") != nullptr ||
            value.member("nextGroups") != nullptr || value.member("sections") != nullptr)
            fail(where, "a card with forms has no field, group or sections of its own");
        CardLayout layout;
        layout.card = card;
        std::set<std::string> names;
        for (const base::JsonValue& item : forms->asArray(where + " forms"))
        {
            CardForm form;
            form.name = required(item, "name", where).asText(where + " form name");
            const std::string at = where + ", form " + form.name;
            if (!names.insert(form.name).second)
                fail(at, "two forms have this name");
            if (!layout.forms.empty() && !layout.forms.back().when)
                fail(at, "only the last form has no condition");
            form.layout = readLayout(card, item, at);
            if (const base::JsonValue* when = item.member("when"))
                form.when = readCondition(*when, at, form.layout);
            layout.forms.push_back(std::move(form));
        }
        if (layout.forms.size() < 2)
            fail(where, "a card with forms has two of them or more");
        if (const base::JsonValue* note = value.member("note"))
            layout.note = note->asText(where + " note");
        return layout;
    }

    static FormCondition readCondition(const base::JsonValue& value, const std::string& where, const CardLayout& layout)
    {
        const std::string at = where + " condition";
        FormCondition condition;
        if (const base::JsonValue* field = value.member("field"))
        {
            condition.field = field->asText(at + " field");
            if (!conditionPlace(layout, condition.field))
                fail(at, "the form has no field " + condition.field +
                             " (a fixed field, a field of its group, or the name of its sections)");
        }
        if (const base::JsonValue* values = value.member("values"))
        {
            for (const base::JsonValue& item : values->asArray(at + " values"))
                condition.values.push_back(item.asText(at + " value"));
        }
        if (const base::JsonValue* kinds = value.member("kinds"))
        {
            for (const base::JsonValue& item : kinds->asArray(at + " kinds"))
            {
                const std::string& name = item.asText(at + " kind");
                const auto found = std::find_if(SLOT_KIND_NAMES.begin(), SLOT_KIND_NAMES.end(),
                                                [&](const SlotKindName& entry) { return entry.name == name; });
                if (found == SLOT_KIND_NAMES.end())
                    fail(at, "unknown kind of value \"" + name + "\" (blank, integer, real or text)");
                condition.kinds.push_back(found->kind);
            }
        }
        if (const base::JsonValue* slot = value.member("blankSlot"))
            condition.blankSlot = integerOf(*slot, "blankSlot", at);
        if (const base::JsonValue* slot = value.member("filledSlot"))
            condition.filledSlot = integerOf(*slot, "filledSlot", at);
        const bool onValue = !condition.values.empty() || !condition.kinds.empty();
        const bool onField = !condition.field.empty() && onValue;
        const int onSlot = (condition.blankSlot > 0 ? 1 : 0) + (condition.filledSlot > 0 ? 1 : 0);
        if (onField == (onSlot > 0) || onSlot > 1 || (onSlot > 0 && (!condition.field.empty() || onValue)))
            fail(at, "a condition gives a field with values or kinds, or one blankSlot or filledSlot");
        return condition;
    }

    static CardLayout readLayout(const std::string& card, const base::JsonValue& value, const std::string& where)
    {
        CardLayout layout;
        layout.card = card;
        std::set<std::string> names;
        const auto addName = [&](const std::string& name, const std::string& at)
        {
            if (name.empty())
                fail(at, "a field has no name");
            if (!names.insert(name).second)
                fail(at, "the field name \"" + name + "\" is used twice");
        };
        for (const base::JsonValue& item : required(value, "fields", where).asArray(where + " fields"))
        {
            LayoutField field;
            field.name = required(item, "name", where).asText(where + " field name");
            const std::string at = where + ", field " + field.name;
            addName(field.name, at);
            field.slot = integerOf(required(item, "slot", at), "slot", at);
            if (field.slot < 1)
                fail(at, "the slot is below 1");
            field.kind = kindOf(item, at);
            if (const base::JsonValue* fallback = item.member("default"))
                field.defaultValue = fallback->asText(at + " default");
            field.span = spanOf(item, at, true);
            layout.fields.push_back(std::move(field));
        }
        checkAlternatives(layout, where);
        // The names given before each group, for the count fields.
        std::set<std::string> earlier = names;
        if (const base::JsonValue* group = value.member("group"))
        {
            const std::string at = where + ", group";
            layout.group = readGroup(*group, at, addName, GroupPlace::First);
            if (layout.group->firstSlot < 1)
                fail(at, "firstSlot is 1 or more");
            checkLines(*layout.group, (layout.group->firstSlot - 1) % LINE_SLOTS, at + " " + layout.group->name);
            checkCount(*layout.group, earlier, at + " " + layout.group->name);
            for (const LayoutGroupField& field : layout.group->fields)
                earlier.insert(field.name);
        }
        if (const base::JsonValue* next = value.member("nextGroups"))
        {
            if (!layout.group)
                fail(where, "nextGroups follow a group");
            for (const base::JsonValue& item : next->asArray(where + " nextGroups"))
            {
                const std::string at = where + ", group";
                LayoutGroup group = readGroup(item, at, addName, GroupPlace::Next);
                checkLines(group, 0, at + " " + group.name);
                checkCount(group, earlier, at + " " + group.name);
                for (const LayoutGroupField& field : group.fields)
                    earlier.insert(field.name);
                layout.nextGroups.push_back(std::move(group));
            }
            checkGroupEnds(layout, where);
        }
        if (const base::JsonValue* sections = value.member("sections"))
        {
            if (!layout.nextGroups.empty())
                fail(where, "a card with several groups has no sections");
            layout.sections = readSections(*sections, where, names, layout.group.has_value());
        }
        if (const base::JsonValue* note = value.member("note"))
            layout.note = note->asText(where + " note");
        if (layout.group)
        {
            for (const LayoutField& field : layout.fields)
            {
                if (field.slot + field.span - 1 >= layout.group->firstSlot)
                    fail(where, "the field " + field.name + " is in the slots of the group " + layout.group->name);
            }
        }
        if (layout.sections && layout.sections->firstSlot > 0)
            checkFixedInSections(layout, where);
        for (std::size_t index = 0; index < layout.fields.size(); ++index)
        {
            const bool toEnd = layout.fields[index].span == SPAN_TO_END;
            if (toEnd && (index + 1 != layout.fields.size() || layout.group || layout.sections))
                fail(where,
                     "the field " + layout.fields[index].name +
                         " runs to the end of the card (span 0): it is the last field, with no group or sections");
        }
        return layout;
    }

    // A group that gives `perLine` keeps its repetitions inside a line, from `lineOffset`, the place of its first
    // repetition in its line.
    static void checkLines(const LayoutGroup& group, int lineOffset, const std::string& at)
    {
        if (group.perLine < 0)
            fail(at, "perLine is 0 or more");
        if (group.perLine > 0 && lineOffset + lineLength(group) > LINE_SLOTS)
            fail(at, "perLine repetitions do not fit in a line");
        if (group.list && group.perLine > 0)
            fail(at, "a group with a nested list gives no perLine");
    }

    // A count field is a field given before the group.
    static void checkCount(const LayoutGroup& group, const std::set<std::string>& earlier, const std::string& at)
    {
        if (!group.countField.empty() && earlier.count(group.countField) == 0)
            fail(at, "the count field " + group.countField + " is not a field given before the group");
    }

    // A group followed by another one tells where it ends: by its count, or by the marker of the next one.
    static void checkGroupEnds(const CardLayout& layout, const std::string& where)
    {
        const std::vector<const LayoutGroup*> groups = groupsOf(layout);
        for (std::size_t index = 0; index + 1 < groups.size(); ++index)
        {
            const LayoutGroup& group = *groups[index];
            const LayoutGroup& next = *groups[index + 1];
            const bool counted = group.fixedCount > 0 || !group.countField.empty();
            if (!counted && next.marker.empty())
                fail(where, "the group " + group.name + " is followed by " + next.name +
                                ": it gives a count, or the next group a marker");
            if (!next.marker.empty() && (group.perLine > 0 || group.list))
                fail(where, "the group " + group.name + " before a marker has no perLine and no nested list");
        }
    }

    // A fixed field in the slots of the sections is on their first line, at a place no section uses.
    static void checkFixedInSections(const CardLayout& layout, const std::string& where)
    {
        const LayoutSections& sections = *layout.sections;
        for (const LayoutField& field : layout.fields)
        {
            if (field.slot < sections.firstSlot)
                continue;
            if (sections.packed)
                fail(where, "the field " + field.name + " is in the slots of the sections " + sections.name);
            const int offset = field.slot - sections.firstSlot;
            bool taken = offset >= LINE_SLOTS || field.span > 1 || offset == sections.keyOffset;
            for (const LayoutSectionKind& kind : sections.kinds)
            {
                taken = taken || offset == kind.keyOffset;
                for (const LayoutGroupField& other : kind.fields)
                {
                    const int inLine = other.offset % LINE_SLOTS;
                    taken = taken || (offset >= inLine && offset < inLine + other.span);
                }
                if (kind.list)
                    taken = taken || kind.list->perLine == 0 ||
                            (offset >= kind.list->firstSlot % LINE_SLOTS &&
                             offset < kind.list->firstSlot % LINE_SLOTS + lineLength(*kind.list));
            }
            if (taken)
                fail(where, "the field " + field.name + " is in the slots of the sections " + sections.name);
        }
    }

    // Two fields on one slot must be next to each other and of kinds a value tells apart (integer and real, or a
    // number and a text).
    static void checkAlternatives(const CardLayout& layout, const std::string& where)
    {
        for (std::size_t first = 0; first < layout.fields.size(); ++first)
        {
            for (std::size_t second = first + 1; second < layout.fields.size(); ++second)
            {
                const LayoutField& a = layout.fields[first];
                const LayoutField& b = layout.fields[second];
                const bool overlaps = a.slot < b.slot + b.span && b.slot < a.slot + a.span;
                if (overlaps && a.slot != b.slot)
                    fail(where, "the field " + b.name + " is in the span of " + a.name);
                if (a.slot != b.slot)
                    continue;
                if (second != first + 1)
                    fail(where, "the alternatives " + a.name + " and " + b.name + " are not next to each other");
                if (!kindsDiffer(a.kind, b.kind))
                    fail(where, "the fields " + a.name + " and " + b.name +
                                    " share a slot but their kinds do not tell them apart");
            }
        }
    }

    // The fields of a group, of a nested list or of the list of a section, inside `period` slots.
    template <typename AddName>
    static std::vector<LayoutGroupField> readGroupFields(const base::JsonValue& value, const std::string& at,
                                                         const AddName& addName, int period)
    {
        std::vector<LayoutGroupField> fields;
        std::set<int> offsets;
        for (const base::JsonValue& item : required(value, "fields", at).asArray(at + " fields"))
        {
            LayoutGroupField field;
            field.name = required(item, "name", at).asText(at + " field name");
            const std::string fieldAt = at + ", field " + field.name;
            addName(field.name, fieldAt);
            field.offset = integerOf(required(item, "offset", fieldAt), "offset", fieldAt);
            field.span = spanOf(item, fieldAt);
            if (field.offset < 0 || field.offset + field.span > period)
                fail(fieldAt, "the offset is not inside the period");
            for (int slot = field.offset; slot < field.offset + field.span; ++slot)
            {
                if (!offsets.insert(slot).second)
                    fail(fieldAt, "two fields have the offset " + std::to_string(slot));
            }
            field.kind = kindOf(item, fieldAt);
            fields.push_back(std::move(field));
        }
        if (fields.empty())
            fail(at, "the group has no field");
        return fields;
    }

    // A nested list: its fields fill its items with no gap; its first field and the first field of the group are of
    // kinds a value tells apart.
    template <typename AddName>
    static LayoutNestedList readNestedList(const base::JsonValue& value, const LayoutGroup& group,
                                           const std::string& at, const AddName& addName)
    {
        LayoutNestedList list;
        list.name = required(value, "name", at).asText(at + " list name");
        const std::string listAt = at + ", list " + list.name;
        const auto fieldCount = static_cast<int>(required(value, "fields", listAt).asArray(listAt).size());
        list.fields = readGroupFields(value, listAt, addName, fieldCount);
        for (const LayoutGroupField& field : list.fields)
        {
            if (field.span != 1)
                fail(listAt, "the fields of a nested list have no span");
        }
        const auto offsetZero = [](const LayoutGroupField& field) { return field.offset == 0; };
        const auto groupFirst = std::find_if(group.fields.begin(), group.fields.end(), offsetZero);
        const auto listFirst = std::find_if(list.fields.begin(), list.fields.end(), offsetZero);
        if (groupFirst != group.fields.begin() || listFirst != list.fields.begin() ||
            !kindsDiffer(groupFirst->kind, listFirst->kind))
            fail(listAt,
                 "the first field of the group and the first field of its list, each at offset 0, are of "
                 "kinds a value tells apart");
        return list;
    }

    // A group, or the list of a section (whose "offset" is the place of its first repetition from the first slot of
    // the section).
    template <typename AddName>
    static LayoutGroup readGroup(const base::JsonValue& value, const std::string& where, const AddName& addName,
                                 GroupPlace place)
    {
        LayoutGroup group;
        group.name = required(value, "name", where).asText(where + " name");
        const std::string at = where + " " + group.name;
        if (place == GroupPlace::SectionList)
            group.firstSlot = integerOf(required(value, "offset", at), "offset", at);
        else if (place == GroupPlace::First)
            group.firstSlot = integerOf(required(value, "firstSlot", at), "firstSlot", at);
        else if (value.member("firstSlot") != nullptr)
            fail(at, "a group after the first one has no firstSlot: it starts after the group before it");
        group.period = integerOf(required(value, "period", at), "period", at);
        if (group.period < 1)
            fail(at, "the period is 1 or more");
        if (const base::JsonValue* perLine = value.member("perLine"))
            group.perLine = integerOf(*perLine, "perLine", at);
        if (const base::JsonValue* count = value.member("count"))
        {
            if (place == GroupPlace::SectionList)
                fail(at, "the list of a section has no count: the record gives it");
            if (count->isNumber())
            {
                group.fixedCount = integerOf(*count, "count", at);
                if (group.fixedCount < 1)
                    fail(at, "a fixed count is 1 or more");
            }
            else
            {
                group.countField = count->asText(at + " count");
            }
        }
        if (const base::JsonValue* marker = value.member("marker"))
        {
            if (place != GroupPlace::Next)
                fail(at, "only a group after the first one has a marker");
            group.marker = marker->asText(at + " marker");
            if (kindOfText(group.marker) != SlotKind::Text)
                fail(at, "the marker is a word");
        }
        group.fields = readGroupFields(value, at, addName, group.period);
        if (const base::JsonValue* list = value.member("list"))
        {
            if (place == GroupPlace::SectionList)
                fail(at, "the list of a section has no nested list");
            group.list = readNestedList(*list, group, at, addName);
        }
        return group;
    }

    // The sections of a card. Their field names are checked against those of the fixed fields and the group, and
    // inside each kind (two kinds may name a field alike: a record gives one kind at a time).
    static LayoutSections readSections(const base::JsonValue& value, const std::string& where,
                                       const std::set<std::string>& cardNames, bool afterGroup)
    {
        LayoutSections sections;
        sections.name = required(value, "name", where).asText(where + " sections name");
        const std::string at = where + ", sections " + sections.name;
        if (const base::JsonValue* packed = value.member("packed"))
            sections.packed = packed->asBool(at + " packed");
        if (const base::JsonValue* first = value.member("firstSlot"))
            sections.firstSlot = integerOf(*first, "firstSlot", at);
        if (afterGroup && sections.firstSlot != 0)
            fail(at, "sections after a group start on the line after it: no firstSlot");
        if (!afterGroup && sections.packed && sections.firstSlot < 1)
            fail(at, "firstSlot is 1 or more");
        if (!afterGroup && !sections.packed && (sections.firstSlot < 1 || (sections.firstSlot - 1) % LINE_SLOTS != 0))
            fail(at, "firstSlot is the first slot of a line (1, 9, 17, ...)");
        if (sections.packed)
        {
            if (value.member("keyOffset") != nullptr)
                fail(at, "packed sections have their key word first: no keyOffset");
        }
        else
        {
            sections.keyOffset = integerOf(required(value, "keyOffset", at), "keyOffset", at);
        }
        if (sections.keyOffset < 0 || sections.keyOffset >= LINE_SLOTS)
            fail(at, "keyOffset is from 0 to 7");
        for (const base::JsonValue& item : required(value, "kinds", at).asArray(at + " kinds"))
        {
            LayoutSectionKind kind;
            kind.key = required(item, "key", at).asText(at + " key");
            if (const base::JsonValue* name = item.member("name"))
                kind.name = name->asText(at + " kind name");
            const std::string kindAt = at + ", key \"" + kind.key + "\"";
            if (sections.find(kind.key) != nullptr)
                fail(kindAt, "two kinds have this key");
            kind.keyOffset = sections.keyOffset;
            if (const base::JsonValue* keyOffset = item.member("keyOffset"))
            {
                kind.keyOffset = integerOf(*keyOffset, "keyOffset", kindAt);
                if (kind.keyOffset < 0 || kind.keyOffset >= LINE_SLOTS || sections.packed || kind.key.empty())
                    fail(kindAt,
                         "the keyOffset of a kind is from 0 to 7, for a kind with a key word of sections "
                         "that are not packed");
            }
            if (const base::JsonValue* allLines = item.member("allLines"))
                kind.allLines = allLines->asBool(kindAt + " allLines");
            std::set<std::string> names = cardNames;
            const auto addName = [&](const std::string& name, const std::string& fieldAt)
            {
                if (name.empty())
                    fail(fieldAt, "a field has no name");
                if (!names.insert(name).second)
                    fail(fieldAt, "the field name \"" + name + "\" is used twice");
            };
            if (const base::JsonValue* fields = item.member("fields"))
            {
                for (const base::JsonValue& entry : fields->asArray(kindAt + " fields"))
                {
                    LayoutGroupField field;
                    field.name = required(entry, "name", kindAt).asText(kindAt + " field name");
                    const std::string fieldAt = kindAt + ", field " + field.name;
                    addName(field.name, fieldAt);
                    field.offset = integerOf(required(entry, "offset", fieldAt), "offset", fieldAt);
                    field.span = spanOf(entry, fieldAt);
                    field.kind = kindOf(entry, fieldAt);
                    kind.fields.push_back(std::move(field));
                }
            }
            if (const base::JsonValue* list = item.member("list"))
                kind.list = readGroup(*list, kindAt + ", list", addName, GroupPlace::SectionList);
            if (sections.packed)
                checkPackedKind(kind, kindAt);
            else
                checkKind(kind, kindAt);
            sections.kinds.push_back(std::move(kind));
        }
        if (sections.kinds.empty())
            fail(at, "the sections have no kind");
        return sections;
    }

    // The fields of a kind of section stay on their line, off the place of the key word and off each other; a list
    // comes after the fields. A number may be at the key place of a following line (no key word is a number, so the
    // line carries on the section), and at the key place of the first line of a kind with no key word.
    static void checkKind(const LayoutSectionKind& kind, const std::string& at)
    {
        std::set<int> taken;
        for (const LayoutGroupField& field : kind.fields)
        {
            const std::string fieldAt = at + ", field " + field.name;
            if (field.offset < 0 || field.offset % LINE_SLOTS + field.span > LINE_SLOTS)
                fail(fieldAt, "the field is not inside a line");
            for (int offset = field.offset; offset < field.offset + field.span; ++offset)
            {
                const bool numberAllowed = offset >= LINE_SLOTS || kind.key.empty();
                if (offset % LINE_SLOTS == kind.keyOffset && !(numberAllowed && isNumeric(field.kind)))
                    fail(fieldAt, "the field is in the place of the key word");
                if (!taken.insert(offset).second)
                    fail(fieldAt, "two fields have the offset " + std::to_string(offset));
            }
        }
        if (!kind.list)
            return;
        const LayoutGroup& list = *kind.list;
        const std::string listAt = at + ", list " + list.name;
        if (list.firstSlot < 0)
            fail(listAt, "the offset is 0 or more");
        if (list.perLine == 0)
        {
            for (const LayoutGroupField& field : list.fields)
            {
                if (!isNumeric(field.kind))
                    fail(listAt, "a list with no perLine runs over the key places of its lines: it holds numbers");
            }
            if (list.firstSlot <= kind.keyOffset)
                fail(listAt, "the list is in the place of the key word");
        }
        else
        {
            const int inLine = list.firstSlot % LINE_SLOTS;
            const int end = inLine + lineLength(list);
            if (end > LINE_SLOTS)
                fail(listAt, "the repetitions of a line do not fit in it");
            if (kind.keyOffset >= inLine && kind.keyOffset < end)
                fail(listAt, "the list is in the place of the key word");
        }
        for (const int offset : taken)
        {
            if (offset >= list.firstSlot)
                fail(listAt, "the fields of a kind with a list come before the list");
        }
    }

    // A kind of packed sections: its key word, its fields after it, then its list with no gap.
    static void checkPackedKind(const LayoutSectionKind& kind, const std::string& at)
    {
        if (kind.key.empty())
            fail(at, "a kind of packed sections has a key word");
        std::set<int> taken;
        for (const LayoutGroupField& field : kind.fields)
        {
            for (int offset = field.offset; offset < field.offset + field.span; ++offset)
            {
                if (offset < 1 || !taken.insert(offset).second)
                    fail(at + ", field " + field.name, "the field is in the place of the key word or of a field");
            }
        }
        if (!kind.list)
            return;
        if (kind.list->perLine != 0)
            fail(at, "the list of a packed section gives no perLine");
        if (kind.list->firstSlot < 1)
            fail(at, "the list comes after the key word");
        for (const int offset : taken)
        {
            if (offset >= kind.list->firstSlot)
                fail(at, "the fields of a kind with a list come before the list");
        }
    }
};

} // namespace

int LayoutGroup::slotOf(int first, int repetition, int offset) const noexcept
{
    if (perLine == 0)
        return first + repetition * period + offset;
    return first + repetition / perLine * LINE_SLOTS + repetition % perLine * period + offset;
}

int LayoutSectionKind::lineCount() const noexcept
{
    int lines = 1;
    for (const LayoutGroupField& field : fields)
        lines = std::max(lines, field.offset / LINE_SLOTS + 1);
    return lines;
}

const LayoutSectionKind* LayoutSections::find(std::string_view word) const noexcept
{
    for (const LayoutSectionKind& kind : kinds)
    {
        if ((kind.key == word))
            return &kind;
    }
    return nullptr;
}

namespace
{

// Puts record values into slots, growing the slots as needed.
class SlotFiller
{
public:
    explicit SlotFiller(std::vector<std::string>& slots) : m_slots(slots) {}

    void put(int slot, const std::string& value)
    {
        if (value.empty())
            return;
        const auto index = static_cast<std::size_t>(slot - 1);
        if (m_slots.size() <= index)
            m_slots.resize(index + 1);
        m_slots[index] = value;
    }

private:
    std::vector<std::string>& m_slots;
};

// The values of the record still to be placed.
class ValueCursor
{
public:
    explicit ValueCursor(std::span<const std::string> values, std::size_t start) : m_values(values), m_index(start) {}

    [[nodiscard]] bool done() const noexcept { return m_index >= m_values.size(); }
    // The next value (blank once the record is over).
    const std::string& next()
    {
        static const std::string BLANK;
        return done() ? BLANK : m_values[m_index++];
    }
    [[nodiscard]] bool restIsBlank() const
    {
        return std::all_of(m_values.begin() + static_cast<std::ptrdiff_t>(std::min(m_index, m_values.size())),
                           m_values.end(), [](const std::string& value) { return value.empty(); });
    }

private:
    std::span<const std::string> m_values;
    std::size_t m_index;
};

// The slot texts a card writer produced, read line by line for the sections.
struct SlotView
{
    std::span<const std::string> slots;

    [[nodiscard]] int last() const noexcept { return static_cast<int>(slots.size()); }
    [[nodiscard]] const std::string& at(int slot) const
    {
        static const std::string BLANK;
        return slot >= 1 && slot <= last() ? slots[static_cast<std::size_t>(slot - 1)] : BLANK;
    }
};

// A section found in the slots of a card writer: its kind, its first slot and its values.
struct FoundSection
{
    const LayoutSectionKind* kind = nullptr;
    int firstSlot = 0;
    std::string key;
    std::vector<std::string> fields;
    std::vector<std::string> items; // the fields of the list's repetitions, one after the other
    int lines = 1;                  // the lines that hold its values
};

// The kind whose key word is at its key place on the line that starts at `line`; null when there is none.
const LayoutSectionKind* kindAtLine(const LayoutSections& sections, const SlotView& view, int line)
{
    for (const LayoutSectionKind& kind : sections.kinds)
    {
        if (!kind.key.empty() && (view.at(line + kind.keyOffset) == kind.key))
            return &kind;
    }
    return nullptr;
}

// The kind of the value in a slot a card writer filled (the kind it wrote, else the kind of the text).
SlotKind slotKindAt(const SlotView& view, std::span<const SlotKind> kinds, int slot)
{
    const std::string& text = view.at(slot);
    if (text.empty())
        return SlotKind::Blank;
    const auto index = static_cast<std::size_t>(slot - 1);
    return index < kinds.size() && kinds[index] != SlotKind::Blank ? kinds[index] : kindOfText(text);
}

// The form a record takes: the first whose condition on a field holds (a condition on a slot of the old layout never
// holds for a record).
const CardForm* recordForm(const CardLayout& layout, std::span<const std::string> values)
{
    static const std::string BLANK;
    for (const CardForm& form : layout.forms)
    {
        if (!form.when)
            return &form;
        if (form.when->field.empty())
            continue;
        const std::optional<ConditionPlace> place = conditionPlace(form.layout, form.when->field);
        const std::string& text = place && place->value < values.size() ? values[place->value] : BLANK;
        if (conditionHolds(*form.when, text, kindOfText(text)))
            return &form;
    }
    return nullptr;
}

// The form of the slots a card writer filled.
const CardForm* slotsForm(const CardLayout& layout, const SlotView& view, std::span<const SlotKind> kinds)
{
    for (const CardForm& form : layout.forms)
    {
        if (!form.when)
            return &form;
        const FormCondition& condition = *form.when;
        if (condition.blankSlot > 0)
        {
            if (view.at(condition.blankSlot).empty())
                return &form;
            continue;
        }
        if (condition.filledSlot > 0)
        {
            if (!view.at(condition.filledSlot).empty())
                return &form;
            continue;
        }
        const std::optional<ConditionPlace> place = conditionPlace(form.layout, condition.field);
        if (place && conditionHolds(condition, view.at(place->slot), slotKindAt(view, kinds, place->slot)))
            return &form;
    }
    return nullptr;
}

// Places the repetitions of the groups of a layout one group after the other (next groups, nested lists). `next` is
// the slot after the last one used, `placedAny` whether a group has a repetition.
bool placeGroupsInTurn(const CardLayout& layout, std::span<const std::string> values, ValueCursor& cursor,
                       SlotFiller& filler, int& next, bool& placedAny, std::string& problem)
{
    std::map<std::string, std::string> known; // the values of the fields given so far, by name
    for (std::size_t index = 0; index < layout.fields.size() && index < values.size(); ++index)
        known[layout.fields[index].name] = values[index];
    const std::vector<const LayoutGroup*> groups = groupsOf(layout);
    std::vector<std::pair<int, std::string>> markers;
    next = groups.front()->firstSlot;
    placedAny = false;
    for (std::size_t index = 0; index < groups.size(); ++index)
    {
        const LayoutGroup& group = *groups[index];
        int start = group.firstSlot;
        if (index > 0 && !group.marker.empty())
        {
            markers.emplace_back(next, group.marker);
            start = next + 1;
        }
        else if (index > 0)
        {
            start = lineStartAtOrAfter(next);
        }
        std::optional<std::size_t> count;
        const bool toEnd = group.fixedCount == 0 && group.countField.empty() && !recordGivesCount(layout, index);
        if (group.fixedCount > 0)
            count = static_cast<std::size_t>(group.fixedCount);
        else if (!group.countField.empty())
            count = countOf(known[group.countField]);
        else if (!toEnd)
            count = countOf(cursor.next());
        if (!toEnd && !count)
        {
            problem = "the number of " + group.name + " of a " + layout.card + " is not a count";
            return false;
        }
        if (group.fixedCount == 0 && count && !countFits(*count, values.size(), group.name, layout.card, problem))
            return false;
        int slot = start;
        std::size_t repetition = 0;
        while (toEnd ? !cursor.done() : repetition < *count)
        {
            const int base = group.list ? slot : group.slotOf(start, static_cast<int>(repetition), 0);
            for (const LayoutGroupField& field : group.fields)
            {
                const std::string& value = cursor.next();
                filler.put(base + field.offset, value);
                if (repetition == 0)
                    known[field.name] = value;
            }
            slot = group.list ? base + group.period
                              : group.slotOf(start, static_cast<int>(repetition), lastOffsetOf(group)) + 1;
            if (group.list)
            {
                const std::string& text = cursor.next();
                const std::optional<std::size_t> items = countOf(text);
                if (!items)
                {
                    problem = "the number of " + group.list->name + " of a " + layout.card + " is not a count: \"" +
                              text + "\"";
                    return false;
                }
                if (!countFits(*items, values.size(), group.list->name, layout.card, problem))
                    return false;
                for (std::size_t item = 0; item < *items; ++item)
                {
                    for (const LayoutGroupField& field : group.list->fields)
                        filler.put(slot + field.offset, cursor.next());
                    slot += static_cast<int>(group.list->fields.size());
                }
            }
            ++repetition;
        }
        placedAny = placedAny || repetition > 0;
        next = repetition > 0 ? slot : start;
    }
    // A marker is written once the card has a repetition in its groups.
    if (placedAny)
    {
        for (const auto& [slot, marker] : markers)
            filler.put(slot, marker);
    }
    return true;
}

} // namespace

bool CardLayout::toSlots(std::span<const std::string> values, std::vector<std::string>& slots,
                         std::string& problem) const
{
    if (!forms.empty())
    {
        const CardForm* form = recordForm(*this, values);
        if (form == nullptr)
        {
            slots.clear();
            problem = "the record is of no form of a " + card;
            return false;
        }
        return form->layout.toSlots(values, slots, problem);
    }
    slots.clear();
    SlotFiller filler(slots);
    const std::size_t fixed = std::min(values.size(), fields.size());
    for (std::size_t index = 0; index < fixed; ++index)
    {
        if (values[index].empty())
            continue;
        const LayoutField& field = fields[index];
        const auto slot = static_cast<std::size_t>(field.slot);
        if (slots.size() >= slot && !slots[slot - 1].empty())
        {
            problem = "the fields " + fields[index - 1].name + " and " + field.name + " are alternatives: give one";
            return false;
        }
        filler.put(field.slot, values[index]);
    }
    ValueCursor cursor(values, fields.size());
    if (cursor.done())
        return true;
    if (!group && !sections)
    {
        problem = "a " + card + " has " + std::to_string(fields.size()) + " fields, the record gives " +
                  std::to_string(values.size());
        return false;
    }
    int sectionLine = sections ? sections->firstSlot : 0;
    if (group && readsGroupsInTurn(*this))
    {
        int next = 0;
        bool placedAny = false;
        if (!placeGroupsInTurn(*this, values, cursor, filler, next, placedAny, problem))
            return false;
        sectionLine = placedAny ? lineStartAtOrAfter(next) : lineStartOf(group->firstSlot);
        if (sections && sections->packed)
            sectionLine = next;
    }
    else if (group)
    {
        const std::size_t perRepetition = group->fields.size();
        std::size_t count = (values.size() - fields.size() + perRepetition - 1) / perRepetition;
        if (sections)
        {
            const std::string& text = cursor.next();
            const std::optional<std::size_t> given = countOf(text);
            if (!given)
            {
                problem = "the number of " + group->name + " of a " + card + " is not a count: \"" + text + "\"";
                return false;
            }
            if (!countFits(*given, values.size(), group->name, card, problem))
                return false;
            count = *given;
        }
        for (std::size_t repetition = 0; repetition < count; ++repetition)
        {
            for (const LayoutGroupField& field : group->fields)
                filler.put(group->slotOf(group->firstSlot, static_cast<int>(repetition), field.offset), cursor.next());
        }
        const int lastSlot = count == 0
                                 ? group->firstSlot
                                 : group->slotOf(group->firstSlot, static_cast<int>(count) - 1, lastOffsetOf(*group));
        sectionLine = count == 0 ? lineStartOf(lastSlot) : lineStartOf(lastSlot) + LINE_SLOTS;
        if (sections && sections->packed)
            sectionLine = count == 0 ? group->firstSlot : lastSlot + 1;
    }
    if (!sections)
    {
        if (!cursor.restIsBlank())
        {
            problem = "a " + card + " has no place for the values at the end of the record";
            return false;
        }
        return true;
    }
    while (!cursor.done())
    {
        const std::string& key = cursor.next();
        const LayoutSectionKind* kind = sections->find(key);
        if (kind == nullptr)
        {
            if (key.empty() && cursor.restIsBlank())
                break;
            problem = "\"" + key + "\" is not a key word of the " + sections->name + " of a " + card;
            return false;
        }
        if (sections->packed)
        {
            // The key word, its fields after it, then its list: the next section starts in the slot after them.
            filler.put(sectionLine, key);
            int end = sectionLine + 1;
            for (const LayoutGroupField& field : kind->fields)
            {
                filler.put(sectionLine + field.offset, cursor.next());
                end = std::max(end, sectionLine + field.offset + field.span);
            }
            if (kind->list && !cursor.done())
            {
                const LayoutGroup& list = *kind->list;
                const std::string& text = cursor.next();
                const std::optional<std::size_t> count = countOf(text);
                if (!count)
                {
                    problem = "the number of " + list.name + " of a " + card + " is not a count: \"" + text + "\"";
                    return false;
                }
                if (!countFits(*count, values.size(), list.name, card, problem))
                    return false;
                const int first = sectionLine + list.firstSlot;
                for (std::size_t repetition = 0; repetition < *count; ++repetition)
                {
                    for (const LayoutGroupField& field : list.fields)
                        filler.put(list.slotOf(first, static_cast<int>(repetition), field.offset), cursor.next());
                }
                end = std::max(end, first + static_cast<int>(*count) * list.period);
            }
            sectionLine = end;
            continue;
        }
        filler.put(sectionLine + kind->keyOffset, key);
        int lastLine = kind->allLines ? kind->lineCount() - 1 : 0;
        for (const LayoutGroupField& field : kind->fields)
        {
            const std::string& value = cursor.next();
            filler.put(sectionLine + field.offset, value);
            if (!value.empty())
                lastLine = std::max(lastLine, field.offset / LINE_SLOTS);
        }
        if (kind->list && !cursor.done())
        {
            const LayoutGroup& list = *kind->list;
            const std::string& text = cursor.next();
            const std::optional<std::size_t> count = countOf(text);
            if (!count)
            {
                problem = "the number of " + list.name + " of a " + card + " is not a count: \"" + text + "\"";
                return false;
            }
            if (!countFits(*count, values.size(), list.name, card, problem))
                return false;
            for (std::size_t repetition = 0; repetition < *count; ++repetition)
            {
                for (const LayoutGroupField& field : list.fields)
                {
                    const int slot =
                        list.slotOf(sectionLine + list.firstSlot, static_cast<int>(repetition), field.offset);
                    filler.put(slot, cursor.next());
                    lastLine = std::max(lastLine, (slot - sectionLine) / LINE_SLOTS);
                }
            }
        }
        sectionLine += (lastLine + 1) * LINE_SLOTS;
    }
    return true;
}

namespace
{

// The values found for one group in the slots: its repetitions one after the other, each with the number of items of
// its nested list then their fields.
struct FoundGroup
{
    std::size_t count = 0;
    std::vector<std::string> values;
};

// Reads the groups of a layout one after the other from the slots (next groups, nested lists), up to `end` (the first
// slot of the sections, or past the last slot). The slots it reads are marked in `used`.
bool readGroupsInTurn(const CardLayout& layout, const SlotView& view, std::span<const SlotKind> kinds, int end,
                      std::vector<bool>& used, std::vector<FoundGroup>& found, std::string& problem)
{
    std::map<std::string, std::string> known;
    for (const LayoutField& field : layout.fields)
        known[field.name] = view.at(field.slot);
    const auto take = [&](int slot) -> const std::string&
    {
        if (slot >= 1 && slot <= view.last())
            used[static_cast<std::size_t>(slot - 1)] = true;
        return view.at(slot);
    };
    // The last slot before `limit` that holds a value.
    const auto lastFilledBefore = [&](int limit)
    {
        int last = std::min(limit - 1, view.last());
        while (last >= 1 && view.at(last).empty())
            --last;
        return last;
    };
    const std::vector<const LayoutGroup*> groups = groupsOf(layout);
    int next = groups.front()->firstSlot;
    for (std::size_t index = 0; index < groups.size(); ++index)
    {
        const LayoutGroup& group = *groups[index];
        int start = group.firstSlot;
        if (index > 0 && !group.marker.empty())
        {
            if ((view.at(next) == group.marker))
            {
                take(next);
                start = next + 1;
            }
            else
            {
                start = end; // no marker: the group has no repetition
            }
        }
        else if (index > 0)
        {
            start = lineStartAtOrAfter(next);
        }
        // Where the group ends at the latest: at the marker of the next group, or at the end.
        int limit = end;
        if (index + 1 < groups.size() && !groups[index + 1]->marker.empty())
        {
            for (int slot = start; slot < end; slot += group.period)
            {
                if ((view.at(slot) == groups[index + 1]->marker))
                {
                    limit = slot;
                    break;
                }
            }
        }
        std::optional<std::size_t> count;
        if (group.fixedCount > 0)
            count = static_cast<std::size_t>(group.fixedCount);
        else if (!group.countField.empty())
            count = countOf(known[group.countField]);
        if ((group.fixedCount > 0 || !group.countField.empty()) && !count)
        {
            problem = "the number of " + group.name + " of a " + layout.card + " is not a count";
            return false;
        }
        if (group.fixedCount == 0 && count &&
            !countFits(*count, static_cast<std::size_t>(std::max(view.last(), 0)), group.name, layout.card, problem))
            return false;
        FoundGroup result;
        const int lastFilled = lastFilledBefore(limit);
        int slot = start;
        std::size_t repetition = 0;
        while (count ? repetition < *count : slot <= lastFilled)
        {
            const int base = group.list ? slot : group.slotOf(start, static_cast<int>(repetition), 0);
            for (const LayoutGroupField& field : group.fields)
            {
                std::string text;
                for (int piece = 0; piece < field.span; ++piece)
                    text += take(base + field.offset + piece);
                if (repetition == 0)
                    known[field.name] = text;
                result.values.push_back(std::move(text));
            }
            slot = group.list ? base + group.period
                              : group.slotOf(start, static_cast<int>(repetition), lastOffsetOf(group)) + 1;
            if (group.list)
            {
                // The items go on until a value fits the first field of the group better than that of the list.
                const LayoutNestedList& list = *group.list;
                const LayoutKind groupFirst = group.fields.front().kind;
                const LayoutKind listFirst = list.fields.front().kind;
                std::vector<std::string> items;
                while (slot <= lastFilled)
                {
                    const SlotKind kind = slotKindAt(view, kinds, slot);
                    if (kind != SlotKind::Blank && fitness(groupFirst, kind) > fitness(listFirst, kind))
                        break;
                    for (const LayoutGroupField& field : list.fields)
                        items.push_back(take(slot + field.offset));
                    slot += static_cast<int>(list.fields.size());
                }
                result.values.push_back(countText(items.size() / list.fields.size()));
                result.values.insert(result.values.end(), items.begin(), items.end());
            }
            ++repetition;
        }
        result.count = repetition;
        next = repetition > 0 ? slot : start;
        found.push_back(std::move(result));
    }
    return true;
}

// Reads the sections of a card from the slots a card writer filled, line by line from `start`: a line whose key place
// holds a key word opens a section, the lines after it carry on the section while its fields or its list go on, a line
// with a blank key place (or a number there, for a kind with a field at the key place) opens a section of the kind
// with no key word. The slots that `isFixed` names are skipped.
template <typename IsFixed>
bool readSections(const LayoutSections& sections, const SlotView& view, int start, const IsFixed& isFixed,
                  std::vector<FoundSection>& found, const std::string& card, std::string& problem)
{
    const LayoutSectionKind* blankKind = nullptr;
    for (const LayoutSectionKind& kind : sections.kinds)
    {
        if (kind.key.empty())
            blankKind = &kind;
    }
    bool blankKindTakesKeyPlace = false;
    if (blankKind != nullptr)
    {
        for (const LayoutGroupField& field : blankKind->fields)
            blankKindTakesKeyPlace = blankKindTakesKeyPlace || field.offset == sections.keyOffset;
    }
    const auto noPlace = [&](int slot)
    {
        problem = "slot " + std::to_string(slot) + " of a " + card + " has no field in the layout";
        return false;
    };
    for (int line = start; line <= view.last(); line += LINE_SLOTS)
    {
        const std::string& keyText = view.at(line + sections.keyOffset);
        const LayoutSectionKind* kind = kindAtLine(sections, view, line);
        int firstUsed = 0;
        for (int slot = line; slot < line + LINE_SLOTS && firstUsed == 0; ++slot)
        {
            if (!view.at(slot).empty() && !isFixed(slot))
                firstUsed = slot;
        }
        const bool carriesOn =
            !found.empty() &&
            (found.back().kind->list || (line - found.back().firstSlot) / LINE_SLOTS < found.back().kind->lineCount());
        const bool opensBlankKind =
            blankKind != nullptr &&
            (keyText.empty() || (blankKindTakesKeyPlace && kindOfText(keyText) != SlotKind::Text));
        if (kind != nullptr)
        {
            found.push_back(FoundSection{kind, line, view.at(line + kind->keyOffset), {}, {}, 1});
        }
        else if (firstUsed == 0)
        {
            continue;
        }
        else if (!carriesOn && opensBlankKind)
        {
            // A kind that takes all its lines starts on the line after the section before it, even when its first
            // lines are blank.
            int first = line;
            if (blankKind->allLines)
            {
                int expected = start;
                if (!found.empty() && !found.back().kind->list)
                {
                    const FoundSection& before = found.back();
                    const int lines = std::max(before.lines, before.kind->allLines ? before.kind->lineCount() : 0);
                    expected = before.firstSlot + lines * LINE_SLOTS;
                }
                if (expected < line && line - expected < blankKind->lineCount() * LINE_SLOTS)
                    first = expected;
            }
            found.push_back(FoundSection{blankKind, first, {}, {}, {}, 1});
        }
        else if (!carriesOn)
        {
            return noPlace(firstUsed);
        }
        FoundSection& section = found.back();
        section.fields.resize(section.kind->fields.size());
        for (int slot = line; slot < line + LINE_SLOTS; ++slot)
        {
            const std::string& text = view.at(slot);
            if (text.empty() || isFixed(slot) || (kind != nullptr && slot == line + kind->keyOffset))
                continue;
            const int offset = slot - section.firstSlot;
            bool placed = false;
            for (std::size_t field = 0; field < section.kind->fields.size() && !placed; ++field)
            {
                const LayoutGroupField& candidate = section.kind->fields[field];
                if (offset >= candidate.offset && offset < candidate.offset + candidate.span)
                {
                    section.fields[field] += text; // the pieces of a span, joined
                    placed = true;
                }
            }
            if (!placed && section.kind->list)
            {
                const LayoutGroup& list = *section.kind->list;
                if (const auto place = placeInGroup(list, section.firstSlot + list.firstSlot, slot))
                {
                    const std::size_t index = place->first * list.fields.size() + place->second;
                    if (section.items.size() <= index)
                        section.items.resize(index + 1);
                    section.items[index] += text;
                    placed = true;
                }
            }
            if (!placed)
                return noPlace(slot);
            section.lines = std::max(section.lines, offset / LINE_SLOTS + 1);
        }
    }
    return true;
}

// Reads packed sections from `start`: a key word, the fields after it, then the repetitions of its list up to the
// next key word.
bool readPackedSections(const LayoutSections& sections, const SlotView& view, int start,
                        std::vector<FoundSection>& found, const std::string& card, std::string& problem)
{
    const auto isKey = [&](int slot) { return !view.at(slot).empty() && sections.find(view.at(slot)) != nullptr; };
    int slot = start;
    while (slot <= view.last())
    {
        if (view.at(slot).empty())
        {
            ++slot;
            continue;
        }
        if (!isKey(slot))
        {
            problem = "slot " + std::to_string(slot) + " of a " + card + " has no field in the layout";
            return false;
        }
        const LayoutSectionKind* kind = sections.find(view.at(slot));
        FoundSection section{kind, slot, view.at(slot), {}, {}, 1};
        int end = slot + 1;
        for (const LayoutGroupField& field : kind->fields)
        {
            std::string text;
            for (int piece = 0; piece < field.span; ++piece)
                text += view.at(slot + field.offset + piece);
            section.fields.push_back(std::move(text));
            end = std::max(end, slot + field.offset + field.span);
        }
        if (kind->list)
        {
            const LayoutGroup& list = *kind->list;
            int item = slot + list.firstSlot;
            while (item <= view.last() && !isKey(item))
            {
                for (const LayoutGroupField& field : list.fields)
                    section.items.push_back(view.at(item + field.offset));
                item += list.period;
            }
            end = std::max(end, item);
        }
        found.push_back(std::move(section));
        slot = end;
    }
    return true;
}

// The first line from the group's first line whose key place holds the key word of a section: where the sections of a
// card with a group start (past the last slot when there is none). Packed sections start at the first repetition of
// the group that holds a key word.
int sectionsAfterGroup(const LayoutGroup& group, const LayoutSections& sections, const SlotView& view)
{
    if (sections.packed)
    {
        for (int slot = group.firstSlot; slot <= view.last(); slot += group.period)
        {
            if (!view.at(slot).empty() && sections.find(view.at(slot)) != nullptr)
                return slot;
        }
        return view.last() + 1;
    }
    for (int line = lineStartOf(group.firstSlot); line <= view.last(); line += LINE_SLOTS)
    {
        if (kindAtLine(sections, view, line) != nullptr)
            return line;
    }
    return view.last() + 1;
}

} // namespace

bool CardLayout::toValues(std::span<const std::string> slots, std::span<const SlotKind> kinds,
                          std::vector<std::string>& values, std::string& problem) const
{
    const SlotView view{slots};
    if (!forms.empty())
    {
        const CardForm* form = slotsForm(*this, view, kinds);
        if (form == nullptr)
        {
            problem = "the fields of a " + card + " are of none of its forms";
            return false;
        }
        return form->layout.toValues(slots, kinds, values, problem);
    }
    const bool inTurn = group && readsGroupsInTurn(*this);
    int sectionStart = view.last() + 1;
    if (sections)
        sectionStart = group ? sectionsAfterGroup(*group, *sections, view) : sections->firstSlot;
    const auto fixedFieldAt = [&](int slot, SlotKind kind) -> std::optional<std::size_t>
    {
        std::optional<std::size_t> best;
        int bestFitness = -1;
        for (std::size_t field = 0; field < fields.size(); ++field)
        {
            if (slot < fields[field].slot || slot >= fields[field].slot + fields[field].span)
                continue;
            const int score = fitness(fields[field].kind, kind);
            if (score > bestFitness)
            {
                best = field;
                bestFitness = score;
            }
        }
        return best;
    };
    std::vector<std::string> fixedValues(fields.size());
    std::vector<std::string> groupValues;
    std::vector<bool> used(slots.size(), false);
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        if (slots[index].empty())
            continue;
        const int slot = static_cast<int>(index) + 1;
        const SlotKind kind = index < kinds.size() ? kinds[index] : SlotKind::Text;
        if (const std::optional<std::size_t> field = fixedFieldAt(slot, kind))
        {
            fixedValues[*field] += slots[index]; // the pieces of a span, joined
            used[index] = true;
            continue;
        }
        if (slot >= sectionStart || inTurn)
            continue; // read with the groups or the sections below
        std::optional<std::pair<std::size_t, std::size_t>> place;
        if (group)
            place = placeInGroup(*group, group->firstSlot, slot);
        if (!place)
        {
            problem = "slot " + std::to_string(slot) + " of a " + card + " has no field in the layout";
            return false;
        }
        const std::size_t position = place->first * group->fields.size() + place->second;
        if (groupValues.size() <= position)
            groupValues.resize(position + 1);
        groupValues[position] += slots[index]; // the pieces of a span, joined
    }
    std::vector<FoundGroup> foundGroups;
    if (inTurn)
    {
        if (!readGroupsInTurn(*this, view, kinds, sectionStart, used, foundGroups, problem))
            return false;
        for (std::size_t index = 0; index < slots.size() && static_cast<int>(index) + 1 < sectionStart; ++index)
        {
            if (!slots[index].empty() && !used[index])
            {
                problem = "slot " + std::to_string(index + 1) + " of a " + card + " has no field in the layout";
                return false;
            }
        }
    }
    std::vector<FoundSection> found;
    if (sections && sectionStart <= view.last())
    {
        const auto isFixed = [&](int slot) { return fixedFieldAt(slot, SlotKind::Text).has_value(); };
        const bool read = sections->packed ? readPackedSections(*sections, view, sectionStart, found, card, problem)
                                           : readSections(*sections, view, sectionStart, isFixed, found, card, problem);
        if (!read)
            return false;
    }
    values = std::move(fixedValues);
    if (inTurn)
    {
        for (std::size_t index = 0; index < foundGroups.size(); ++index)
        {
            if (recordGivesCount(*this, index))
                values.push_back(countText(foundGroups[index].count));
            values.insert(values.end(), foundGroups[index].values.begin(), foundGroups[index].values.end());
        }
    }
    else if (group)
    {
        const std::size_t perRepetition = group->fields.size();
        const std::size_t count = (groupValues.size() + perRepetition - 1) / perRepetition;
        if (sections)
        {
            values.push_back(countText(count));
            groupValues.resize(count * perRepetition);
        }
        values.insert(values.end(), groupValues.begin(), groupValues.end());
    }
    for (FoundSection& section : found)
    {
        values.push_back(section.key);
        values.insert(values.end(), section.fields.begin(), section.fields.end());
        if (section.kind->list)
        {
            const std::size_t perRepetition = section.kind->list->fields.size();
            const std::size_t count = (section.items.size() + perRepetition - 1) / perRepetition;
            section.items.resize(count * perRepetition);
            values.push_back(countText(count));
            values.insert(values.end(), section.items.begin(), section.items.end());
        }
    }
    while (!values.empty() && values.back().empty())
        values.pop_back();
    return true;
}

const std::shared_ptr<const CardLayouts>& CardLayouts::standard()
{
    static const std::shared_ptr<const CardLayouts> STANDARD = fromJson(cardLayoutsText());
    return STANDARD;
}

std::shared_ptr<const CardLayouts> CardLayouts::fromJson(std::string_view text)
{
    auto layouts = std::make_shared<CardLayouts>();
    layouts->m_layouts = CatalogueReader().read(text);
    for (std::size_t index = 0; index < layouts->m_layouts.size(); ++index)
    {
        if (!layouts->m_index.emplace(layouts->m_layouts[index].card, index).second)
            throw base::UsageError("card layout catalogue: the card " + layouts->m_layouts[index].card +
                                   " has two layouts");
    }
    return layouts;
}

const CardLayout* CardLayouts::find(std::string_view card) const
{
    const auto found = m_index.find(std::string(card));
    return found == m_index.end() ? nullptr : &m_layouts[found->second];
}

std::string_view CardLayouts::kindName(LayoutKind kind) noexcept
{
    for (const KindName& entry : KIND_NAMES)
    {
        if (entry.kind == kind)
            return entry.name;
    }
    return "any";
}

std::optional<LayoutKind> CardLayouts::kindOf(std::string_view name) noexcept
{
    for (const KindName& entry : KIND_NAMES)
    {
        if (entry.name == name)
            return entry.kind;
    }
    return std::nullopt;
}

} // namespace oor::model
