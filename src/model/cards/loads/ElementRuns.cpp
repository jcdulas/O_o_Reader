#include "oor/model/cards/loads/ElementRuns.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

// The fields of a line of a card: eight, after the name.
constexpr int FIELDS_PER_LINE = 8;

} // namespace

std::vector<ElementRun> readElementRuns(FieldReader& reader, int first, const char* name)
{
    std::vector<ElementRun> runs;
    const int last = reader.lastUsedField();
    for (int field = first; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == "Through"))
        {
            if (runs.empty() || runs.back().first != runs.back().last)
            {
                reader.error("card.bad-value", "THRU must follow a single element", "Through");
                break;
            }
            int next = field + 1;
            while (next <= last && reader.isBlank(next))
                ++next;
            if (next > last)
            {
                reader.error("card.missing-field", "THRU needs a last element", "Through");
                break;
            }
            const base::EntityId end = reader.requiredInteger(next, name);
            if (!reader.failed() && end <= runs.back().first)
                reader.error("card.bad-value", "the end of a THRU range must be above its start", name);
            runs.back().last = end;
            field = next;
            continue;
        }
        const base::EntityId element = reader.requiredInteger(field, name);
        runs.push_back(ElementRun{element, element});
    }
    return runs;
}

int writeElementRuns(FieldWriter& writer, int first, const std::vector<ElementRun>& runs, bool compress)
{
    // With compression, elements that follow each other are one run: 1, 2, 3, 4 is "1 THRU 4". Two elements stay two
    // elements (a THRU would not be shorter).
    std::vector<ElementRun> merged;
    for (const ElementRun& run : runs)
    {
        if (compress && !merged.empty() && run.first == merged.back().last + 1)
            merged.back().last = run.last;
        else
            merged.push_back(run);
    }
    int field = first;
    for (const ElementRun& run : merged)
    {
        const bool asRange = run.last != run.first && (!compress || run.last - run.first >= 2);
        if (run.last != run.first && !asRange)
        {
            writer.integer(field++, run.first);
            writer.integer(field++, run.last);
            continue;
        }
        if (asRange)
        {
            // "first THRU last" stays on one line.
            const int position = (field - 1) % FIELDS_PER_LINE;
            if (position + 3 > FIELDS_PER_LINE)
                field += FIELDS_PER_LINE - position;
            writer.integer(field++, run.first);
            writer.text(field++, "Through");
            writer.integer(field++, run.last);
        }
        else
        {
            writer.integer(field++, run.first);
        }
    }
    return field;
}

bool isOneRun(const std::vector<ElementRun>& runs) noexcept
{
    for (std::size_t index = 1; index < runs.size(); ++index)
    {
        if (runs[index].first != runs[index - 1].last + 1)
            return false;
    }
    return !runs.empty();
}

std::optional<CardProblem> checkElementRuns(const std::vector<ElementRun>& runs, const char* name)
{
    for (const ElementRun& run : runs)
    {
        if (run.first <= 0 || run.last <= 0)
            return CardProblem{"card.bad-id", "an element ID must be a positive integer", name};
        if (run.last < run.first)
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", name};
    }
    return std::nullopt;
}

std::vector<std::array<base::EntityId, 2>> toPairs(const std::vector<ElementRun>& runs)
{
    std::vector<std::array<base::EntityId, 2>> pairs;
    pairs.reserve(runs.size());
    for (const ElementRun& run : runs)
        pairs.push_back({run.first, run.last});
    return pairs;
}

std::vector<ElementRun> toRuns(std::span<const std::array<base::EntityId, 2>> pairs)
{
    std::vector<ElementRun> runs;
    runs.reserve(pairs.size());
    for (const auto& pair : pairs)
        runs.push_back(ElementRun{pair[0], pair[1]});
    return runs;
}

void visitElementRuns(VariableLengthColumn<std::array<base::EntityId, 2>>& column, std::uint32_t row,
                      ReferenceVisitor& visitor, const char* name)
{
    const auto current = column.get(row);
    std::vector<std::array<base::EntityId, 2>> pairs(current.begin(), current.end());
    bool changed = false;
    for (auto& pair : pairs)
    {
        if (pair[0] == pair[1])
        {
            const base::EntityId updated = visitor.visit(name, ReferenceTarget::Element, pair[0]);
            changed = changed || updated != pair[0];
            pair = {updated, updated};
            continue;
        }
        for (base::EntityId& id : pair)
        {
            const base::EntityId updated = visitor.visit(name, ReferenceTarget::ElementInRange, id);
            changed = changed || updated != id;
            id = updated;
        }
    }
    if (changed)
        column.set(row, pairs);
}

} // namespace oor::model
