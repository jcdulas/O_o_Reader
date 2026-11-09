#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

namespace
{

// The increment of a range when BY is left out.
base::EntityId defaultIncrement(base::EntityId first, base::EntityId last) noexcept
{
    return last >= first ? 1 : -1;
}

} // namespace

std::vector<ContactRun> readContactRuns(FieldReader& reader, int first, int last, const char* name,
                                        const ContactRunRules& rules)
{
    std::vector<ContactRun> runs;
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
            if (runs.empty() || runs.back().isRange())
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
            runs.back().increment = defaultIncrement(runs.back().first, runs.back().last);
            field = end;
            continue;
        }
        if (rules.allowBy && (reader.text(field) == "By"))
        {
            const int step = next(field);
            if (runs.empty() || !runs.back().isRange())
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
        runs.push_back(ContactRun{id, id, 1});
    }
    return runs;
}

int writeContactRuns(FieldWriter& writer, int first, const std::vector<ContactRun>& runs)
{
    int field = first;
    for (const ContactRun& run : runs)
    {
        if (!run.isRange())
        {
            writer.integer(field++, run.first);
            continue;
        }
        const bool writesBy = run.increment != defaultIncrement(run.first, run.last);
        const int needed = writesBy ? 5 : 3;
        const int position = (field - 1) % CONTACT_LINE_FIELDS;
        if (position + needed > CONTACT_LINE_FIELDS)
            field = firstFieldOfNextLine(field);
        writer.integer(field++, run.first);
        writer.text(field++, "Through");
        writer.integer(field++, run.last);
        if (writesBy)
        {
            writer.text(field++, "By");
            writer.integer(field++, run.increment);
        }
    }
    return field;
}

std::optional<CardProblem> checkContactRuns(const std::vector<ContactRun>& runs, const char* name,
                                            const ContactRunRules& rules)
{
    for (const ContactRun& run : runs)
    {
        if (run.first <= 0 || run.last <= 0)
            return CardProblem{"card.bad-id", "an ID must be a positive integer", name};
        if (!run.isRange())
        {
            if (run.increment != 1)
                return CardProblem{"card.bad-value", "BY needs a THRU range", "By"};
            continue;
        }
        if (run.increment == 0)
            return CardProblem{"card.bad-value", "the BY increment must not be zero", "By"};
        if (run.last < run.first && !rules.allowDescending)
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", name};
        if ((run.increment > 0) != (run.last > run.first))
            return CardProblem{"card.bad-value", "the BY increment must go from the first ID to the last one", "By"};
        if (!rules.allowBy && run.increment != defaultIncrement(run.first, run.last))
            return CardProblem{"card.bad-value", "this card has no BY increment", "By"};
    }
    return std::nullopt;
}

std::vector<base::EntityId> expandContactRuns(const std::vector<ContactRun>& runs)
{
    std::uint64_t count = 0;
    for (const ContactRun& run : runs)
        count += run.isRange() ? base::idRangeCount(run.first, run.last, run.increment) : 1;
    base::requireExpandable(count, "the ID list of a contact card");
    std::vector<base::EntityId> ids;
    ids.reserve(static_cast<std::size_t>(count));
    for (const ContactRun& run : runs)
    {
        if (!run.isRange())
        {
            ids.push_back(run.first);
            continue;
        }
        // 64-bit counters: an end of 2147483647 (or -2147483648 going down) must not wrap
        if (run.increment > 0)
        {
            for (std::int64_t id = run.first; id <= run.last; id += run.increment)
                ids.push_back(static_cast<base::EntityId>(id));
        }
        else if (run.increment < 0)
        {
            for (std::int64_t id = run.first; id >= run.last; id += run.increment)
                ids.push_back(static_cast<base::EntityId>(id));
        }
    }
    return ids;
}

void visitContactRuns(std::vector<ContactRun>& runs, ReferenceVisitor& visitor, const char* name,
                      ReferenceTarget single, ReferenceTarget range)
{
    for (ContactRun& run : runs)
    {
        if (!run.isRange())
        {
            const base::EntityId updated = visitor.visit(name, single, run.first);
            if (updated != run.first)
            {
                run.first = updated;
                run.last = updated;
            }
            continue;
        }
        const base::EntityId first = visitor.visit(name, range, run.first);
        const base::EntityId last = visitor.visit(name, range, run.last);
        if (first != run.first)
            run.first = first;
        if (last != run.last)
            run.last = last;
    }
}

std::vector<std::array<base::EntityId, 3>> toContactTriples(const std::vector<ContactRun>& runs)
{
    std::vector<std::array<base::EntityId, 3>> triples;
    triples.reserve(runs.size());
    for (const ContactRun& run : runs)
        triples.push_back({run.first, run.last, run.increment});
    return triples;
}

std::vector<ContactRun> toContactRuns(std::span<const std::array<base::EntityId, 3>> triples)
{
    std::vector<ContactRun> runs;
    runs.reserve(triples.size());
    for (const auto& triple : triples)
        runs.push_back(ContactRun{triple[0], triple[1], triple[2]});
    return runs;
}

void visitContactTriples(VariableLengthColumn<std::array<base::EntityId, 3>>& column, std::uint32_t row,
                         ReferenceVisitor& visitor, const char* name, ReferenceTarget single, ReferenceTarget range)
{
    std::vector<ContactRun> runs = toContactRuns(column.get(row));
    const std::vector<ContactRun> before = runs;
    visitContactRuns(runs, visitor, name, single, range);
    if (runs != before)
        column.set(row, toContactTriples(runs));
}

} // namespace oor::model
