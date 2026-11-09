#include "oor/model/format/LayoutSlots.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>

namespace oor::model
{

// These walks follow CardLayout::toValues and CardLayout::toSlots: keep their placement rules together when the
// catalogue gains another layout feature.

namespace
{

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

// The last slot a fixed field covers in a card of `lastSlot` slots (a span to the end stops at the last slot).
int lastSlotOf(const LayoutField& field, int lastSlot) noexcept
{
    if (field.span == SPAN_TO_END)
        return std::max(field.slot, lastSlot);
    return field.slot + field.span - 1;
}

// The last offset a repetition of `group` takes (the end of its last field).
int lastOffsetOf(const LayoutGroup& group) noexcept
{
    int last = 0;
    for (const LayoutGroupField& field : group.fields)
        last = std::max(last, field.offset + field.span - 1);
    return last;
}

// How well a field of `kind` takes a value of kind `slot` (0: it does not fit): an item of a nested list goes on until
// a value fits the first field of the group better than the first field of the list.
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

// A count of repetitions: blank is 0; nullopt when the text is not a count.
std::optional<std::size_t> countOf(std::string_view text)
{
    if (text.empty())
        return 0;
    const std::optional<std::int64_t> count = base::NumberParser::parseInteger(text);
    if (!count || *count < 0)
        return std::nullopt;
    return static_cast<std::size_t>(*count);
}

// The groups of a layout in order: the group, then the groups after it.
std::vector<const LayoutGroup*> groupsOf(const CardLayout& layout)
{
    std::vector<const LayoutGroup*> groups;
    if (layout.group)
        groups.push_back(&*layout.group);
    for (const LayoutGroup& group : layout.nextGroups)
        groups.push_back(&group);
    return groups;
}

// True when a record gives the number of repetitions of group `index` before them: the layout does not know it and
// the group is not the last part of the record.
bool recordGivesCount(const CardLayout& layout, std::size_t index)
{
    const std::vector<const LayoutGroup*> groups = groupsOf(layout);
    const LayoutGroup& group = *groups[index];
    const bool last = index + 1 == groups.size();
    return group.fixedCount == 0 && group.countField.empty() && (!last || layout.sections.has_value());
}

// The layouts with more than one group, or a group with a nested list, are read one group after the other.
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

bool conditionHolds(const FormCondition& condition, std::string_view text)
{
    for (const std::string& value : condition.values)
    {
        if (valueMatches(value, text))
            return true;
    }
    return std::find(condition.kinds.begin(), condition.kinds.end(), slotKindOf(text)) != condition.kinds.end();
}

// The text of a slot (blank past the slots of the card).
const std::string& slotText(std::span<const std::string> slots, int slot)
{
    static const std::string BLANK;
    return slot >= 1 && slot <= static_cast<int>(slots.size()) ? slots[static_cast<std::size_t>(slot - 1)] : BLANK;
}

// The form the slots of a card select (CardLayout::toValues): a condition on a slot of the old layout, or on the
// value of a field of the form.
const CardLayout* slotsLayout(const CardLayout& layout, std::span<const std::string> slots)
{
    if (layout.forms.empty())
        return &layout;
    for (const CardForm& form : layout.forms)
    {
        if (!form.when)
            return &form.layout;
        const FormCondition& condition = *form.when;
        if (condition.blankSlot > 0)
        {
            if (slotText(slots, condition.blankSlot).empty())
                return &form.layout;
            continue;
        }
        if (condition.filledSlot > 0)
        {
            if (!slotText(slots, condition.filledSlot).empty())
                return &form.layout;
            continue;
        }
        const std::optional<ConditionPlace> place = conditionPlace(form.layout, condition.field);
        if (place && conditionHolds(condition, slotText(slots, place->slot)))
            return &form.layout;
    }
    return nullptr;
}

// The place of a slot in a group (or in the list of a section).
struct GroupPlace
{
    int repetition = 0;
    const LayoutGroupField* field = nullptr;
};

// Where `slot` is in `group` when its first repetition starts at `first`: none for a slot before the group or in its
// padding.
std::optional<GroupPlace> groupPlaceOf(const LayoutGroup& group, int first, int slot)
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
    for (const LayoutGroupField& field : group.fields)
    {
        if (offset >= field.offset && offset < field.offset + field.span)
            return GroupPlace{repetition, &field};
    }
    return std::nullopt;
}

// A section found in the slots: its kind, its first slot and the number of lines that hold its values.
struct OpenSection
{
    const LayoutSectionKind* kind = nullptr;
    int firstSlot = 0;
    int lines = 1;
};

// Finds the places of the slots of one card in a layout with no forms.
class SlotPlacer
{
public:
    SlotPlacer(const CardLayout& layout, std::span<const std::string> slots)
        : m_layout(layout), m_slots(slots), m_last(static_cast<int>(slots.size())), m_used(slots.size(), false)
    {
        m_places.layout = &layout;
    }

    SlotPlaces place()
    {
        const bool inTurn = m_layout.group && readsGroupsInTurn(m_layout);
        const int sectionStart = sectionStartOf();
        addFixedSpans();
        noteFixedKinds();
        if (m_layout.group && !inTurn)
            addGroupSpans(sectionStart);
        if (inTurn)
            placeGroupsInTurn(sectionStart);
        for (int slot = 1; slot <= m_last && slot < sectionStart; ++slot)
        {
            if (at(slot).empty() || isFixed(slot))
                continue;
            bool inGroup = inTurn && m_used[static_cast<std::size_t>(slot - 1)];
            if (!inTurn && m_layout.group)
            {
                if (const std::optional<GroupPlace> place =
                        groupPlaceOf(*m_layout.group, m_layout.group->firstSlot, slot))
                {
                    noteKind(slot, place->field->kind, place->field->name);
                    inGroup = true;
                }
            }
            if (!inGroup)
                m_places.withoutField.push_back(slot);
        }
        if (m_layout.sections && sectionStart <= m_last)
        {
            if (m_layout.sections->packed)
                placePackedSections(sectionStart);
            else
                placeSections(sectionStart);
        }
        std::sort(m_places.withoutField.begin(), m_places.withoutField.end());
        return std::move(m_places);
    }

private:
    [[nodiscard]] const std::string& at(int slot) const { return slotText(m_slots, slot); }

    // True when a fixed field covers `slot`: it takes the slot before the groups and the sections do.
    [[nodiscard]] bool isFixed(int slot) const
    {
        return std::any_of(m_layout.fields.begin(), m_layout.fields.end(), [&](const LayoutField& field)
                           { return slot >= field.slot && slot <= lastSlotOf(field, m_last); });
    }

    // Notes the kind of the field that holds a filled slot (the first field found keeps it).
    void noteKind(int slot, LayoutKind kind, std::string_view name)
    {
        if (!at(slot).empty())
        {
            m_places.fieldKinds.emplace(slot, kind);
            m_places.fieldNames.emplace(slot, name);
        }
    }

    // The kinds of the fixed fields: of two alternatives on one slot, the one the text of the slot fits best (as
    // CardLayout::toValues chooses it).
    void noteFixedKinds()
    {
        for (int slot = 1; slot <= m_last; ++slot)
        {
            const SlotKind kind = slotKindOf(at(slot));
            if (kind == SlotKind::Blank)
                continue;
            int best = -1;
            for (const LayoutField& field : m_layout.fields)
            {
                if (slot < field.slot || slot > lastSlotOf(field, m_last))
                    continue;
                const int score = fitness(field.kind, kind);
                if (score > best)
                {
                    best = score;
                    m_places.fieldKinds[slot] = field.kind;
                    m_places.fieldNames[slot] = field.name;
                }
            }
        }
    }

    // The kind whose key word is at its key place on the line that starts at `line`; null when there is none.
    [[nodiscard]] const LayoutSectionKind* kindAtLine(int line) const
    {
        for (const LayoutSectionKind& kind : m_layout.sections->kinds)
        {
            if (!kind.key.empty() && (at(line + kind.keyOffset) == kind.key))
                return &kind;
        }
        return nullptr;
    }

    [[nodiscard]] bool isKeyWord(int slot) const
    {
        return !at(slot).empty() && m_layout.sections->find(at(slot)) != nullptr;
    }

    // Where the sections start: their first slot, or after a group the first line from the group's first line that
    // holds a key word at its key place (packed sections: the first repetition of the group that holds a key word);
    // past the last slot when there is none.
    [[nodiscard]] int sectionStartOf() const
    {
        if (!m_layout.sections)
            return m_last + 1;
        const LayoutSections& sections = *m_layout.sections;
        if (!m_layout.group)
            return sections.firstSlot;
        const LayoutGroup& group = *m_layout.group;
        if (sections.packed)
        {
            for (int slot = group.firstSlot; slot <= m_last; slot += group.period)
            {
                if (isKeyWord(slot))
                    return slot;
            }
            return m_last + 1;
        }
        for (int line = lineStartOf(group.firstSlot); line <= m_last; line += LINE_SLOTS)
        {
            if (kindAtLine(line) != nullptr)
                return line;
        }
        return m_last + 1;
    }

    void addSpan(int firstSlot, int span)
    {
        if (span > 1 && m_spanStarts.insert(firstSlot).second)
            m_places.spans.push_back(SpanPlace{firstSlot, span});
    }

    void addFixedSpans()
    {
        for (const LayoutField& field : m_layout.fields)
            addSpan(field.slot, lastSlotOf(field, m_last) - field.slot + 1);
    }

    void addGroupSpans(int sectionStart)
    {
        const LayoutGroup& group = *m_layout.group;
        for (int repetition = 0;; ++repetition)
        {
            const int start = group.slotOf(group.firstSlot, repetition, 0);
            if (start > m_last || start >= sectionStart)
                return;
            for (const LayoutGroupField& field : group.fields)
                addSpan(group.slotOf(group.firstSlot, repetition, field.offset), field.span);
        }
    }

    // Marks a slot as read by a group.
    const std::string& take(int slot)
    {
        if (slot >= 1 && slot <= m_last)
            m_used[static_cast<std::size_t>(slot - 1)] = true;
        return at(slot);
    }

    // The last slot before `limit` that holds a value (0 when there is none).
    [[nodiscard]] int lastFilledBefore(int limit) const
    {
        int last = std::min(limit - 1, m_last);
        while (last >= 1 && at(last).empty())
            --last;
        return last;
    }

    // The groups one after the other, as the reader's readGroupsInTurn reads them up to `end`: a group after the first
    // starts on the line after the group before it, or right after its marker word; a group runs for its count (a
    // count field, a fixed count) or to its last filled slot; the items of a nested list go on until a value fits the
    // first field of the group better than the first field of the list. The slots read are marked in m_used.
    void placeGroupsInTurn(int end)
    {
        std::map<std::string, std::string> known; // the values of the fields read so far, by name
        std::map<std::string, int> knownSlot;     // and the slots they were read from
        for (const LayoutField& field : m_layout.fields)
        {
            known[field.name] = at(field.slot);
            knownSlot[field.name] = field.slot;
        }
        const std::vector<const LayoutGroup*> groups = groupsOf(m_layout);
        int next = groups.front()->firstSlot;
        for (std::size_t index = 0; index < groups.size(); ++index)
        {
            const LayoutGroup& group = *groups[index];
            int start = group.firstSlot;
            if (index > 0 && !group.marker.empty())
            {
                if ((at(next) == group.marker))
                {
                    take(next);
                    m_places.fieldNames.emplace(next, "");
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
            int limit = end;
            if (index + 1 < groups.size() && !groups[index + 1]->marker.empty())
            {
                for (int slot = start; slot < end; slot += group.period)
                {
                    if ((at(slot) == groups[index + 1]->marker))
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
                count = countOf(known[group.countField]).value_or(0); // not a count: the reader refuses the card
            // Past the last slot every repetition is blank: a larger count is read as the number of slots.
            if (count && *count > static_cast<std::size_t>(m_last))
            {
                m_places.largeCounts.push_back(LargeCount{group.countField, knownSlot[group.countField]});
                count = static_cast<std::size_t>(m_last);
            }
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
                    {
                        text += take(base + field.offset + piece);
                        noteKind(base + field.offset + piece, field.kind, field.name);
                    }
                    if (repetition == 0)
                    {
                        known[field.name] = text;
                        knownSlot[field.name] = base + field.offset;
                    }
                    addSpan(base + field.offset, field.span);
                }
                slot = group.list ? base + group.period
                                  : group.slotOf(start, static_cast<int>(repetition), lastOffsetOf(group)) + 1;
                if (group.list)
                    slot = placeNestedList(group, slot, lastFilled);
                ++repetition;
            }
            next = repetition > 0 ? slot : start;
        }
    }

    // The items of the nested list of `group` from `slot`; returns the slot after them.
    int placeNestedList(const LayoutGroup& group, int slot, int lastFilled)
    {
        const LayoutNestedList& list = *group.list;
        const LayoutKind groupFirst = group.fields.front().kind;
        const LayoutKind listFirst = list.fields.front().kind;
        while (slot <= lastFilled)
        {
            const SlotKind kind = slotKindOf(at(slot));
            if (kind != SlotKind::Blank && fitness(groupFirst, kind) > fitness(listFirst, kind))
                break;
            for (const LayoutGroupField& field : list.fields)
            {
                take(slot + field.offset);
                noteKind(slot + field.offset, field.kind, field.name);
            }
            slot += static_cast<int>(list.fields.size());
        }
        return slot;
    }

    // The sections line by line, as the reader's readSections reads them: a line whose key place holds a key word
    // opens a section (each kind may have its own key place), the lines after it carry on the section while its fields
    // or its list go on, a line with a blank key place (or a number there, when the kind with no key word has a field
    // there) opens a section of the kind with no key word. A kind that takes all its lines starts on the line after
    // the section before it, even when its first lines are blank.
    void placeSections(int start)
    {
        const LayoutSections& sections = *m_layout.sections;
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
        std::vector<OpenSection> found;
        for (int line = start; line <= m_last; line += LINE_SLOTS)
        {
            const std::string& keyText = at(line + sections.keyOffset);
            const LayoutSectionKind* kind = kindAtLine(line);
            bool used = false;
            for (int slot = line; slot < line + LINE_SLOTS && !used; ++slot)
                used = !at(slot).empty() && !isFixed(slot);
            const bool carriesOn =
                !found.empty() && (found.back().kind->list ||
                                   (line - found.back().firstSlot) / LINE_SLOTS < found.back().kind->lineCount());
            const bool opensBlankKind =
                blankKind != nullptr &&
                (keyText.empty() || (blankKindTakesKeyPlace && slotKindOf(keyText) != SlotKind::Text));
            if (kind != nullptr)
            {
                openSection(found, kind, line);
            }
            else if (!used)
            {
                continue;
            }
            else if (!carriesOn && opensBlankKind)
            {
                int first = line;
                if (blankKind->allLines)
                {
                    int expected = start;
                    if (!found.empty() && !found.back().kind->list)
                    {
                        const OpenSection& before = found.back();
                        const int lines = std::max(before.lines, before.kind->allLines ? before.kind->lineCount() : 0);
                        expected = before.firstSlot + lines * LINE_SLOTS;
                    }
                    if (expected < line && line - expected < blankKind->lineCount() * LINE_SLOTS)
                        first = expected;
                }
                openSection(found, blankKind, first);
            }
            else if (!carriesOn)
            {
                markLine(line);
                continue;
            }
            OpenSection& section = found.back();
            for (int slot = line; slot < line + LINE_SLOTS; ++slot)
            {
                if (at(slot).empty() || isFixed(slot) || (kind != nullptr && slot == line + kind->keyOffset))
                    continue;
                if (!placeInSection(section, slot))
                    m_places.withoutField.push_back(slot);
            }
        }
    }

    void openSection(std::vector<OpenSection>& found, const LayoutSectionKind* kind, int firstSlot)
    {
        found.push_back(OpenSection{kind, firstSlot, 1});
        if (!kind->key.empty())
            m_places.fieldNames.emplace(firstSlot + kind->keyOffset, "");
        for (const LayoutGroupField& field : kind->fields)
            addSpan(firstSlot + field.offset, field.span);
    }

    // True when a field or the list of `section` covers `slot`; the section then holds its values up to that line.
    bool placeInSection(OpenSection& section, int slot)
    {
        const int offset = slot - section.firstSlot;
        bool placed = false;
        for (const LayoutGroupField& field : section.kind->fields)
        {
            if (!placed && offset >= field.offset && offset < field.offset + field.span)
            {
                noteKind(slot, field.kind, field.name);
                placed = true;
            }
        }
        if (!placed && section.kind->list)
        {
            const LayoutGroup& list = *section.kind->list;
            const int first = section.firstSlot + list.firstSlot;
            if (const std::optional<GroupPlace> place = groupPlaceOf(list, first, slot))
            {
                addSpan(list.slotOf(first, place->repetition, place->field->offset), place->field->span);
                noteKind(slot, place->field->kind, place->field->name);
                placed = true;
            }
        }
        if (placed)
            section.lines = std::max(section.lines, offset / LINE_SLOTS + 1);
        return placed;
    }

    // Packed sections, as the reader's readPackedSections reads them: a key word, the fields after it, then the
    // repetitions of its list up to the next key word; the next section starts in the slot after them.
    void placePackedSections(int start)
    {
        int slot = start;
        while (slot <= m_last)
        {
            if (at(slot).empty())
            {
                ++slot;
                continue;
            }
            if (!isKeyWord(slot))
            {
                m_places.withoutField.push_back(slot);
                ++slot;
                continue;
            }
            const LayoutSectionKind& kind = *m_layout.sections->find(at(slot));
            m_places.fieldNames.emplace(slot, "");
            int end = slot + 1;
            for (const LayoutGroupField& field : kind.fields)
            {
                addSpan(slot + field.offset, field.span);
                for (int piece = 0; piece < field.span; ++piece)
                    noteKind(slot + field.offset + piece, field.kind, field.name);
                end = std::max(end, slot + field.offset + field.span);
            }
            if (kind.list)
            {
                const LayoutGroup& list = *kind.list;
                int item = slot + list.firstSlot;
                while (item <= m_last && !isKeyWord(item))
                {
                    for (const LayoutGroupField& field : list.fields)
                    {
                        addSpan(item + field.offset, field.span);
                        noteKind(item + field.offset, field.kind, field.name);
                    }
                    item += list.period;
                }
                end = std::max(end, item);
            }
            slot = end;
        }
    }

    // Every filled slot of a line that no section takes.
    void markLine(int line)
    {
        for (int slot = line; slot < line + LINE_SLOTS; ++slot)
        {
            if (!at(slot).empty() && !isFixed(slot))
                m_places.withoutField.push_back(slot);
        }
    }

    const CardLayout& m_layout;
    std::span<const std::string> m_slots;
    int m_last;
    std::vector<bool> m_used; // the slots read by the groups read one after the other
    SlotPlaces m_places;
    std::set<int> m_spanStarts;
};

// Names the values of a record one after the other, as CardLayout::toSlots takes them.
class RecordNamer
{
public:
    RecordNamer(const CardLayout& layout, std::span<const std::string> values)
        : m_layout(layout), m_values(values), m_names(values.size())
    {
    }

    // The values that give a number of repetitions, once names() has run.
    [[nodiscard]] const std::vector<RecordCount>& counts() const noexcept { return m_counts; }

    std::vector<std::string> names()
    {
        for (const LayoutField& field : m_layout.fields)
            name(field.name);
        if (m_layout.group && readsGroupsInTurn(m_layout))
            nameGroupsInTurn();
        else if (m_layout.group)
            nameGroup();
        if (m_layout.sections)
            nameSections();
        return std::move(m_names);
    }

private:
    [[nodiscard]] bool done() const noexcept { return m_index >= m_values.size(); }

    // Gives the next value a name and returns its text (blank once the record is over).
    const std::string& name(std::string_view field)
    {
        static const std::string BLANK;
        if (done())
            return BLANK;
        m_names[m_index] = std::string(field);
        return m_values[m_index++];
    }

    // The next value, a count: its number (0 when it is not a count).
    std::size_t count()
    {
        if (!done())
            m_counts.push_back(RecordCount{m_index, {}});
        return countOf(name({})).value_or(0);
    }

    void nameFields(const std::vector<LayoutGroupField>& fields)
    {
        for (const LayoutGroupField& field : fields)
            name(field.name);
    }

    // The group of a layout read in one piece: its count first when sections follow, else the rest of the record.
    void nameGroup()
    {
        const LayoutGroup& group = *m_layout.group;
        if (!m_layout.sections)
        {
            while (!done())
                nameFields(group.fields);
            return;
        }
        const std::size_t repetitions = count();
        for (std::size_t repetition = 0; repetition < repetitions && !done(); ++repetition)
            nameFields(group.fields);
    }

    void nameGroupsInTurn()
    {
        std::map<std::string, std::string> known;
        std::map<std::string, std::size_t> knownIndex; // the index of the value of each field in `known`
        for (std::size_t index = 0; index < m_layout.fields.size() && index < m_values.size(); ++index)
        {
            known[m_layout.fields[index].name] = m_values[index];
            knownIndex[m_layout.fields[index].name] = index;
        }
        const std::vector<const LayoutGroup*> groups = groupsOf(m_layout);
        for (std::size_t index = 0; index < groups.size(); ++index)
        {
            const LayoutGroup& group = *groups[index];
            const bool toEnd = group.fixedCount == 0 && group.countField.empty() && !recordGivesCount(m_layout, index);
            std::size_t repetitions = 0;
            if (group.fixedCount > 0)
                repetitions = static_cast<std::size_t>(group.fixedCount);
            else if (!group.countField.empty())
            {
                repetitions = countOf(known[group.countField]).value_or(0);
                if (const auto found = knownIndex.find(group.countField); found != knownIndex.end())
                    m_counts.push_back(RecordCount{found->second, group.countField});
            }
            else if (!toEnd)
                repetitions = count();
            for (std::size_t repetition = 0; (toEnd || repetition < repetitions) && !done(); ++repetition)
            {
                for (const LayoutGroupField& field : group.fields)
                {
                    const std::size_t index = m_index;
                    const std::string& value = name(field.name);
                    if (repetition == 0)
                    {
                        known[field.name] = value;
                        knownIndex[field.name] = index;
                    }
                }
                if (!group.list)
                    continue;
                const std::size_t items = count();
                for (std::size_t item = 0; item < items && !done(); ++item)
                    nameFields(group.list->fields);
            }
        }
    }

    void nameSections()
    {
        while (!done())
        {
            const LayoutSectionKind* kind = m_layout.sections->find(name({}));
            if (kind == nullptr)
                return;
            nameFields(kind->fields);
            if (!kind->list || done())
                continue;
            const std::size_t repetitions = count();
            for (std::size_t repetition = 0; repetition < repetitions && !done(); ++repetition)
                nameFields(kind->list->fields);
        }
    }

    const CardLayout& m_layout;
    std::span<const std::string> m_values;
    std::vector<std::string> m_names;
    std::vector<RecordCount> m_counts;
    std::size_t m_index = 0;
};

} // namespace

SlotKind slotKindOf(std::string_view text) noexcept
{
    if (text.empty())
        return SlotKind::Blank;
    if (base::NumberParser::parseInteger(text))
        return SlotKind::Integer;
    if (base::NumberParser::parseReal(text))
        return SlotKind::Real;
    return SlotKind::Text;
}

SlotPlaces placeSlots(const CardLayout& layout, std::span<const std::string> slots)
{
    const CardLayout* form = slotsLayout(layout, slots);
    if (form == nullptr)
        return {};
    return SlotPlacer(*form, slots).place();
}

const CardLayout* recordLayout(const CardLayout& layout, std::span<const std::string> values)
{
    if (layout.forms.empty())
        return &layout;
    static const std::string BLANK;
    for (const CardForm& form : layout.forms)
    {
        if (!form.when)
            return &form.layout;
        if (form.when->field.empty())
            continue; // a form of the old layout only
        const std::optional<ConditionPlace> place = conditionPlace(form.layout, form.when->field);
        const std::string& text = place && place->value < values.size() ? values[place->value] : BLANK;
        if (conditionHolds(*form.when, text))
            return &form.layout;
    }
    return nullptr;
}

std::vector<std::string> recordFieldNames(const CardLayout& layout, std::span<const std::string> values)
{
    const CardLayout* form = recordLayout(layout, values);
    if (form == nullptr)
        return std::vector<std::string>(values.size());
    return RecordNamer(*form, values).names();
}

std::vector<RecordCount> recordCounts(const CardLayout& layout, std::span<const std::string> values)
{
    const CardLayout* form = recordLayout(layout, values);
    if (form == nullptr)
        return {};
    RecordNamer namer(*form, values);
    (void)namer.names();
    return namer.counts();
}

} // namespace oor::model
