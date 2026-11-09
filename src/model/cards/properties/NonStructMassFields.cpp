#include "oor/model/cards/properties/NonStructMassFields.hpp"

#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;

// The types of the TYPE field: the neutral names of card types, which the deck writes as their keywords, and the plain
// word ELEMENT.
constexpr std::array<std::string_view, 16> NON_STRUCT_MASS_TYPES{
    "ShellProp",    "CompProp",         "CompPropGlobal", "BarProp",
    "BarShapeProp", "BeamProp",         "BeamShapeProp",  "BeamLumpedAreaProp",
    "RodProp",      "Rod2InlineSection",  "CurvedBeamProp", "ShearPanelProp",
    "TubeProp",     "ConicalShellProp", "CrackTipProp2D", NON_STRUCT_MASS_ELEMENT};

} // namespace

bool isNonStructMassType(std::string_view type) noexcept
{
    for (const std::string_view known : NON_STRUCT_MASS_TYPES)
    {
        if (known == type)
            return true;
    }
    return false;
}

bool nonStructMassTypeIsElement(std::string_view type) noexcept
{
    return type == NON_STRUCT_MASS_ELEMENT || type == "Rod2InlineSection";
}

std::string readNonStructMassType(FieldReader& reader, int field)
{
    const std::string word(reader.text(field));
    if (word.empty())
    {
        reader.error("card.missing-field", "the " + std::string("type") + " of non-structural mass is required",
                     "type");
        return word;
    }
    for (std::size_t index = 0; index < NON_STRUCT_MASS_TYPES.size(); ++index)
    {
        if (NON_STRUCT_MASS_TYPES[index] == word)
            return std::string(NON_STRUCT_MASS_TYPES[index]);
    }
    reader.error("card.bad-value", std::string("type") + " must be a property type or ELEMENT, not " + word, "type");
    return word;
}

std::optional<CardProblem> checkNonStructMassType(const std::string& type)
{
    if (type.empty())
        return CardProblem{"card.missing-field",
                           "the " + std::string(CardProblem::FIELD) + " of non-structural mass is required", "type"};
    if (!isNonStructMassType(type))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a property type or ELEMENT, not " + type,
                           "type"};
    return std::nullopt;
}

ReferenceTarget nonStructMassTarget(std::string_view type) noexcept
{
    if (nonStructMassTypeIsElement(type))
        return ReferenceTarget::ElementInRange;
    if (type == "CrackTipProp2D")
        return ReferenceTarget::Other; // not a card of the library
    return ReferenceTarget::PropertyInRange;
}

std::vector<NonStructMassPair> readNonStructMassPairs(FieldReader& reader, int first)
{
    std::vector<NonStructMassPair> pairs;
    const int last = reader.lastUsedField();
    for (int field = first; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        NonStructMassPair pair;
        pair.id = reader.requiredInteger(field, "targetId");
        pair.value = reader.requiredReal(field + 1, "targetValue");
        pairs.push_back(pair);
    }
    return pairs;
}

void writeNonStructMassPairs(FieldWriter& writer, int first, const std::vector<NonStructMassPair>& pairs)
{
    int field = first;
    for (const NonStructMassPair& pair : pairs)
    {
        writer.integer(field, pair.id);
        writer.real(field + 1, pair.value);
        field += 2;
    }
}

std::optional<CardProblem> checkNonStructMassPairs(const std::vector<NonStructMassPair>& pairs)
{
    if (pairs.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("targetValue"),
                           "targetId"};
    for (const NonStructMassPair& pair : pairs)
    {
        if (pair.id <= 0)
            return CardProblem{"card.bad-id", "an " + std::string(CardProblem::FIELD) + " must be a positive integer",
                               "targetId"};
    }
    return std::nullopt;
}

NonStructMassIdList readNonStructMassIds(FieldReader& reader, int first)
{
    NonStructMassIdList list;
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
        const std::string word(reader.text(field));
        if (word == "All")
        {
            if (field != first || next(field) != 0)
                reader.error("card.bad-value", "ALL must be the only " + std::string("targetId") + " of the list",
                             "targetId");
            list.all = true;
            break;
        }
        if (word == "Through")
        {
            const int end = next(field);
            if (list.runs.empty() || list.runs.back().first != list.runs.back().last)
            {
                reader.error("card.bad-value", "THRU must follow a single " + std::string("targetId"), "through");
                break;
            }
            if (end == 0)
            {
                reader.error("card.missing-field", "THRU needs a last " + std::string("targetId"), "through");
                break;
            }
            list.runs.back().last = reader.requiredInteger(end, "targetId");
            field = end;
            continue;
        }
        if (word == "By")
        {
            const int step = next(field);
            if (list.runs.empty() || list.runs.back().first == list.runs.back().last)
            {
                reader.error("card.bad-value", "BY must follow a THRU range", "increment");
                break;
            }
            if (step == 0)
            {
                reader.error("card.missing-field", "BY needs an increment", "increment");
                break;
            }
            list.runs.back().increment = reader.requiredInteger(step, "increment");
            field = step;
            continue;
        }
        const base::EntityId id = reader.requiredInteger(field, "targetId");
        list.runs.push_back(NonStructMassRun{id, id, 1});
    }
    return list;
}

void writeNonStructMassIds(FieldWriter& writer, int first, const NonStructMassIdList& ids)
{
    if (ids.all)
    {
        writer.text(first, "All");
        return;
    }
    int field = first;
    for (const NonStructMassRun& run : ids.runs)
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
}

std::optional<CardProblem> checkNonStructMassIds(const NonStructMassIdList& ids)
{
    if (ids.all)
    {
        if (!ids.runs.empty())
            return CardProblem{"card.bad-value", "ALL cannot be followed by " + std::string(CardProblem::FIELD) + "s",
                               "targetId"};
        return std::nullopt;
    }
    if (ids.runs.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one " + std::string(CardProblem::FIELD) + ", or ALL", "targetId"};
    for (const NonStructMassRun& run : ids.runs)
    {
        if (run.first <= 0 || run.last <= 0)
            return CardProblem{"card.bad-id", "an " + std::string(CardProblem::FIELD) + " must be a positive integer",
                               "targetId"};
        if (run.last < run.first)
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", "targetId"};
        if (run.increment <= 0)
            return CardProblem{"card.bad-value", "the BY increment must be positive", "increment"};
        if (run.first == run.last && run.increment != 1)
            return CardProblem{"card.bad-value", "BY needs a THRU range", "increment"};
    }
    return std::nullopt;
}

std::vector<base::EntityId> expandNonStructMassIds(const NonStructMassIdList& ids)
{
    std::uint64_t count = 0;
    for (const NonStructMassRun& run : ids.runs)
        count += base::idRangeCount(run.first, run.last, run.increment);
    base::requireExpandable(count, "the ID list of a non-structural mass card");
    std::vector<base::EntityId> result;
    result.reserve(static_cast<std::size_t>(count));
    for (const NonStructMassRun& run : ids.runs)
    {
        // 64-bit, and a step of at least 1: a last ID of 2147483647 or a BY of 0 must not loop forever
        const std::int64_t step = std::max<std::int64_t>(run.increment, 1);
        for (std::int64_t id = run.first; id <= run.last; id += step)
            result.push_back(static_cast<base::EntityId>(id));
    }
    return result;
}

std::vector<std::array<base::EntityId, 3>> toTriples(const std::vector<NonStructMassRun>& runs)
{
    std::vector<std::array<base::EntityId, 3>> triples;
    triples.reserve(runs.size());
    for (const NonStructMassRun& run : runs)
        triples.push_back({run.first, run.last, run.increment});
    return triples;
}

std::vector<NonStructMassRun> toNonStructMassRuns(std::span<const std::array<base::EntityId, 3>> triples)
{
    std::vector<NonStructMassRun> runs;
    runs.reserve(triples.size());
    for (const auto& triple : triples)
        runs.push_back(NonStructMassRun{triple[0], triple[1], triple[2]});
    return runs;
}

void visitNonStructMassRuns(VariableLengthColumn<std::array<base::EntityId, 3>>& column, std::uint32_t row,
                            ReferenceVisitor& visitor, std::string_view type)
{
    const ReferenceTarget target = nonStructMassTarget(type);
    const auto current = column.get(row);
    std::vector<std::array<base::EntityId, 3>> triples(current.begin(), current.end());
    bool changed = false;
    for (auto& triple : triples)
    {
        // A single ID is both ends: it is visited once.
        const bool single = triple[0] == triple[1];
        for (std::size_t end = 0; end < 2; ++end)
        {
            const base::EntityId updated = visitor.visit("targetId", target, triple[end]);
            changed = changed || updated != triple[end];
            triple[end] = updated;
            if (single)
            {
                triple[1] = updated;
                break;
            }
        }
    }
    if (changed)
        column.set(row, triples);
}

void visitNonStructMassIds(VariableLengthColumn<base::EntityId>& column, std::uint32_t row, ReferenceVisitor& visitor,
                           std::string_view type)
{
    const ReferenceTarget target = nonStructMassTarget(type);
    const auto current = column.get(row);
    std::vector<base::EntityId> ids(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& id : ids)
    {
        const base::EntityId updated = visitor.visit("targetId", target, id);
        changed = changed || updated != id;
        id = updated;
    }
    if (changed)
        column.set(row, ids);
}

} // namespace oor::model
