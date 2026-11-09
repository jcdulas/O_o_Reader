#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <cstdlib>

namespace oor::model
{

namespace
{

// The fields of a line of a card: eight, after the name.
constexpr int FIELDS_PER_LINE = 8;

} // namespace

std::vector<ThermalRun> readThermalRuns(FieldReader& reader, int first, const char* name, bool allowBy)
{
    std::vector<ThermalRun> runs;
    const int last = reader.lastUsedField();
    // The next field that is not blank after `field`, or 0.
    const auto next = [&](int field)
    {
        for (int candidate = field + 1; candidate <= last; ++candidate)
        {
            if (!reader.isBlank(candidate))
                return candidate;
        }
        return 0;
    };
    for (int field = first; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == "Through"))
        {
            const int end = next(field);
            if (runs.empty() || runs.back().first != runs.back().last)
            {
                reader.error("card.bad-value", "THRU must follow a single ID", "Through");
                break;
            }
            if (end == 0)
            {
                reader.error("card.missing-field", "THRU needs a last ID", "Through");
                break;
            }
            runs.back().last = reader.requiredInteger(end, name);
            field = end;
            continue;
        }
        if (allowBy && (reader.text(field) == "By"))
        {
            const int step = next(field);
            if (runs.empty() || runs.back().first == runs.back().last)
            {
                reader.error("card.bad-value", "BY must follow a THRU range", "By");
                break;
            }
            if (step == 0)
            {
                reader.error("card.missing-field", "BY needs an increment", "By");
                break;
            }
            runs.back().increment = reader.requiredInteger(step, "By");
            field = step;
            continue;
        }
        const base::EntityId id = reader.requiredInteger(field, name);
        runs.push_back(ThermalRun{id, id, 1});
    }
    return runs;
}

int writeThermalRuns(FieldWriter& writer, int first, const std::vector<ThermalRun>& runs)
{
    int field = first;
    for (const ThermalRun& run : runs)
    {
        if (run.first == run.last)
        {
            writer.integer(field++, run.first);
            continue;
        }
        // "first THRU last [BY n]" stays on one line, because the solver reads the THRU form field by field.
        const int needed = run.increment != 1 ? 5 : 3;
        const int position = (field - 1) % FIELDS_PER_LINE;
        if (position + needed > FIELDS_PER_LINE)
            field += FIELDS_PER_LINE - position;
        writer.integer(field++, run.first);
        writer.text(field++, "Through");
        writer.integer(field++, run.last);
        if (run.increment != 1)
        {
            writer.text(field++, "By");
            writer.integer(field++, run.increment);
        }
    }
    return field;
}

std::optional<CardProblem> checkThermalRuns(const std::vector<ThermalRun>& runs, const char* name, bool allowNegative)
{
    if (runs.empty())
        return CardProblem{"card.missing-field", "the card needs at least one ID", name};
    for (const ThermalRun& run : runs)
    {
        if (run.first == run.last)
        {
            if (run.first == 0 || (run.first < 0 && !allowNegative))
                return CardProblem{"card.bad-id", "an ID must be a positive integer", name};
            if (run.increment != 1)
                return CardProblem{"card.bad-value", "BY needs a THRU range", "By"};
            continue;
        }
        if (run.first <= 0 || run.last <= 0)
            return CardProblem{"card.bad-id", "the ends of a THRU range must be positive integers", name};
        if (run.last < run.first)
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", name};
        if (run.increment <= 0)
            return CardProblem{"card.bad-value", "the BY increment must be positive", "By"};
    }
    return std::nullopt;
}

std::vector<base::EntityId> expandThermalRuns(const std::vector<ThermalRun>& runs)
{
    std::uint64_t count = 0;
    for (const ThermalRun& run : runs)
        count += run.first == run.last ? 1 : base::idRangeCount(run.first, run.last, run.increment);
    base::requireExpandable(count, "the ID list of a thermal card");
    std::vector<base::EntityId> ids;
    ids.reserve(static_cast<std::size_t>(count));
    for (const ThermalRun& run : runs)
    {
        if (run.first == run.last)
        {
            ids.push_back(run.first);
            continue;
        }
        // 64-bit, and a step of at least 1: a last ID of 2147483647 or a BY of 0 must not loop forever
        const std::int64_t step = std::max<std::int64_t>(run.increment, 1);
        for (std::int64_t id = run.first; id <= run.last; id += step)
            ids.push_back(static_cast<base::EntityId>(id));
    }
    return ids;
}

std::vector<std::array<base::EntityId, 3>> toThermalTriples(const std::vector<ThermalRun>& runs)
{
    std::vector<std::array<base::EntityId, 3>> triples;
    triples.reserve(runs.size());
    for (const ThermalRun& run : runs)
        triples.push_back({run.first, run.last, run.increment});
    return triples;
}

std::vector<ThermalRun> toThermalRuns(std::span<const std::array<base::EntityId, 3>> triples)
{
    std::vector<ThermalRun> runs;
    runs.reserve(triples.size());
    for (const auto& triple : triples)
        runs.push_back(ThermalRun{triple[0], triple[1], triple[2]});
    return runs;
}

void visitThermalRuns(VariableLengthColumn<std::array<base::EntityId, 3>>& column, std::uint32_t row,
                      ReferenceVisitor& visitor, const char* name, ReferenceTarget single, ReferenceTarget range)
{
    const auto current = column.get(row);
    std::vector<std::array<base::EntityId, 3>> triples(current.begin(), current.end());
    bool changed = false;
    const auto visit = [&](ReferenceTarget target, base::EntityId id)
    {
        // The sign of an element ID says which face of the surface element; the reference is its absolute value.
        const base::EntityId magnitude = id < 0 ? -id : id;
        const base::EntityId updated = visitor.visit(name, target, magnitude);
        return id < 0 ? -updated : updated;
    };
    for (auto& triple : triples)
    {
        if (triple[0] == triple[1])
        {
            const base::EntityId updated = visit(single, triple[0]);
            changed = changed || updated != triple[0];
            triple[0] = updated;
            triple[1] = updated;
            continue;
        }
        for (std::size_t end = 0; end < 2; ++end)
        {
            const base::EntityId updated = visit(range, triple[end]);
            changed = changed || updated != triple[end];
            triple[end] = updated;
        }
    }
    if (changed)
        column.set(row, triples);
}

} // namespace oor::model
