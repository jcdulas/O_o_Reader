#include "oor/model/cards/sets/DofLists.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <iterator>
#include <string>

namespace oor::model
{

namespace
{

// The fields of a line of a card: eight, after the name.
constexpr int FIELDS_PER_LINE = 8;
// The first field of the first continuation line.
constexpr int FIRST_CONTINUATION_FIELD = 9;

} // namespace

std::vector<IdRun> readIdRuns(FieldReader& reader, int first, const char* name)
{
    std::vector<IdRun> runs;
    const int last = reader.lastUsedField();
    for (int field = first; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == "Through"))
        {
            if (runs.empty() || runs.back().first != runs.back().last)
            {
                reader.error("card.bad-value", "THRU must follow a single ID", "Through");
                break;
            }
            int next = field + 1;
            while (next <= last && reader.isBlank(next))
                ++next;
            if (next > last)
            {
                reader.error("card.missing-field", "THRU needs a last ID", "Through");
                break;
            }
            const base::EntityId end = reader.requiredInteger(next, name);
            if (!reader.failed() && end < runs.back().first)
                reader.error("card.bad-value", "the end of a THRU range must not be below its start", name);
            runs.back().last = end;
            field = next;
            continue;
        }
        const base::EntityId id = reader.requiredInteger(field, name);
        runs.push_back(IdRun{id, id});
    }
    return runs;
}

int writeIdRuns(FieldWriter& writer, int first, const std::vector<IdRun>& runs, bool compress)
{
    // With compression, IDs that follow each other are one run: 1, 2, 3, 4 is "1 THRU 4". Two IDs stay two IDs (a THRU
    // would not be shorter).
    std::vector<IdRun> merged;
    for (const IdRun& run : runs)
    {
        if (compress && !merged.empty() && run.first == merged.back().last + 1 && run.first > 0)
            merged.back().last = run.last;
        else
            merged.push_back(run);
    }
    int field = first;
    for (const IdRun& run : merged)
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
            // "first THRU last" stays on one line, and its THRU is never the first field of a line.
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

std::optional<CardProblem> checkIdRuns(const std::vector<IdRun>& runs, const char* name, IdSign sign, bool mayBeEmpty)
{
    if (runs.empty() && !mayBeEmpty)
        return CardProblem{"card.missing-field", "the list needs at least one ID", name};
    for (const IdRun& run : runs)
    {
        const auto valid = [sign](base::EntityId id)
        {
            switch (sign)
            {
            case IdSign::Positive:
                return id > 0;
            case IdSign::NotZero:
                return id != 0;
            case IdSign::NotNegative:
                return id >= 0;
            }
            return false;
        };
        if (!valid(run.first) || !valid(run.last))
        {
            const char* message = sign == IdSign::Positive  ? "an ID must be a positive integer"
                                  : sign == IdSign::NotZero ? "an ID must not be zero"
                                                            : "an ID must not be negative";
            return CardProblem{"card.bad-id", message, name};
        }
        if (run.last < run.first)
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", name};
    }
    return std::nullopt;
}

std::vector<std::array<base::EntityId, 2>> toPairs(const std::vector<IdRun>& runs)
{
    std::vector<std::array<base::EntityId, 2>> pairs;
    pairs.reserve(runs.size());
    for (const IdRun& run : runs)
        pairs.push_back({run.first, run.last});
    return pairs;
}

std::vector<IdRun> toIdRuns(std::span<const std::array<base::EntityId, 2>> pairs)
{
    std::vector<IdRun> runs;
    runs.reserve(pairs.size());
    for (const auto& pair : pairs)
        runs.push_back(IdRun{pair[0], pair[1]});
    return runs;
}

void visitIdRuns(VariableLengthColumn<std::array<base::EntityId, 2>>& column, std::uint32_t row,
                 ReferenceVisitor& visitor, const char* name, ReferenceTarget single, ReferenceTarget range)
{
    const auto current = column.get(row);
    std::vector<std::array<base::EntityId, 2>> pairs(current.begin(), current.end());
    bool changed = false;
    for (auto& pair : pairs)
    {
        if (pair[0] == pair[1])
        {
            const base::EntityId updated = visitor.visit(name, single, pair[0]);
            changed = changed || updated != pair[0];
            pair = {updated, updated};
            continue;
        }
        for (base::EntityId& id : pair)
        {
            const base::EntityId updated = visitor.visit(name, range, id);
            changed = changed || updated != id;
            id = updated;
        }
    }
    if (changed)
        column.set(row, pairs);
}

void visitIdList(VariableLengthColumn<base::EntityId>& column, std::uint32_t row, ReferenceVisitor& visitor,
                 const char* name, ReferenceTarget target)
{
    const auto current = column.get(row);
    std::vector<base::EntityId> ids(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& id : ids)
    {
        const base::EntityId updated = visitor.visit(name, target, id);
        changed = changed || updated != id;
        id = updated;
    }
    if (changed)
        column.set(row, ids);
}

int dofPairField(const DofPairLayout& layout, std::size_t index) noexcept
{
    const int position = static_cast<int>(index);
    if (position < layout.firstLinePairs)
        return layout.firstField + 2 * position;
    return FIRST_CONTINUATION_FIELD + 2 * (position - layout.firstLinePairs);
}

std::vector<DofPair> readDofPairs(FieldReader& reader, const DofPairLayout& layout)
{
    std::vector<DofPair> pairs;
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; dofPairField(layout, index) <= last && !reader.failed(); ++index)
    {
        const int field = dofPairField(layout, index);
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding
        const std::string number = std::to_string(index + 1);
        const std::string idName = "node" + number;
        const std::string componentName = "components" + number;
        DofPair pair;
        pair.node = reader.requiredInteger(field, idName.c_str());
        pair.components = base::sortedDigits(reader.integer(field + 1, componentName.c_str(), 0));
        pairs.push_back(pair);
    }
    return pairs;
}

void writeDofPairs(FieldWriter& writer, const DofPairLayout& layout, const std::vector<DofPair>& pairs)
{
    std::size_t index = 0;
    for (const DofPair& pair : pairs)
    {
        const int field = dofPairField(layout, index++);
        writer.integer(field, pair.node);
        writer.integerUnlessDefault(field + 1, pair.components, 0);
    }
}

std::optional<CardProblem> checkDofPairs(const std::vector<DofPair>& pairs)
{
    // The field names of the first pairs (a CardProblem keeps the name as a pointer, so they are constants); the pairs
    // after them are reported on the names "node" and "components".
    static constexpr const char* ID_NAMES[] = {"node1", "node2", "node3", "node4", "node5", "node6", "node7", "node8"};
    static constexpr const char* COMPONENT_NAMES[] = {"components1", "components2", "components3", "components4",
                                                      "components5", "components6", "components7", "components8"};
    constexpr std::size_t NAMED = std::size(ID_NAMES);
    if (pairs.empty())
        return CardProblem{"card.missing-field", "the card needs at least one point", "node1"};
    for (std::size_t index = 0; index < pairs.size(); ++index)
    {
        const DofPair& pair = pairs[index];
        if (pair.node <= 0)
            return CardProblem{"card.bad-id", "a grid or scalar point ID must be a positive integer",
                               index < NAMED ? ID_NAMES[index] : "node"};
        if (!base::isComponentSet(pair.components))
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
                index < NAMED ? COMPONENT_NAMES[index] : "components"};
    }
    return std::nullopt;
}

void splitDofPairs(const std::vector<DofPair>& pairs, std::vector<base::EntityId>& nodes,
                   std::vector<std::int32_t>& components)
{
    nodes.clear();
    components.clear();
    nodes.reserve(pairs.size());
    components.reserve(pairs.size());
    for (const DofPair& pair : pairs)
    {
        nodes.push_back(pair.node);
        components.push_back(pair.components);
    }
}

std::vector<DofPair> joinDofPairs(std::span<const base::EntityId> nodes, std::span<const std::int32_t> components)
{
    std::vector<DofPair> pairs;
    pairs.reserve(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index)
        pairs.push_back(DofPair{nodes[index], components[index]});
    return pairs;
}

} // namespace oor::model
