#include "oor/model/cards/thermal/temperatureBoundary/TemperatureBoundaryTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

namespace
{

// The fields of a line of a card: eight, after the name.
constexpr int FIELDS_PER_LINE = 8;
constexpr int FIRST_GROUP_FIELD = 3;

} // namespace

TemperatureBoundaryTable::TemperatureBoundaryTable() : m_kind("Static") {}

std::optional<TemperatureBoundaryCard> TemperatureBoundaryFormat::read(FieldReader& reader)
{
    TemperatureBoundaryCard card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = readKeyword(reader, 2, "kind", {"Static", "Transient"}, "Static");
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
    // Groups: a temperature, then a grid point, or a grid point THRU another, optionally BY an increment.
    int field = FIRST_GROUP_FIELD;
    while (field != 0 && field <= last && !reader.failed())
    {
        TemperatureBoundaryGroup group;
        group.temperature = reader.requiredReal(field, "temperature");
        const int gridField = next(field);
        if (gridField == 0)
        {
            reader.error("card.missing-field", "a temperature needs its grid point", "nodeId");
            break;
        }
        const base::EntityId first = reader.requiredInteger(gridField, "nodeId");
        group.grids = ThermalRun{first, first, 1};
        field = next(gridField);
        if (field != 0 && (reader.text(field) == "Through"))
        {
            const int endField = next(field);
            if (endField == 0)
            {
                reader.error("card.missing-field", "THRU needs a last grid point", "nodeId");
                break;
            }
            group.grids.last = reader.requiredInteger(endField, "nodeId");
            field = next(endField);
            if (field != 0 && (reader.text(field) == "By"))
            {
                const int stepField = next(field);
                if (stepField == 0)
                {
                    reader.error("card.missing-field", "BY needs an increment", "increment");
                    break;
                }
                group.grids.increment = reader.requiredInteger(stepField, "increment");
                field = next(stepField);
            }
        }
        card.groups.push_back(group);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TemperatureBoundaryFormat::check(const TemperatureBoundaryCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the temperature set ID must be a positive integer", "id"};
    if (const auto problem = checkKeyword(card.kind, "kind", {"Static", "Transient"}))
        return problem;
    if (card.groups.empty())
        return CardProblem{"card.missing-field", "the card needs at least one temperature and grid point",
                           "temperature"};
    std::vector<ThermalRun> runs;
    for (const TemperatureBoundaryGroup& group : card.groups)
        runs.push_back(group.grids);
    return checkThermalRuns(runs, "nodeId", false);
}

void TemperatureBoundaryFormat::write(const TemperatureBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, card.kind, "Static");
    int field = FIRST_GROUP_FIELD;
    for (const TemperatureBoundaryGroup& group : card.groups)
    {
        const bool single = group.grids.first == group.grids.last;
        const int needed = single ? 2 : (group.grids.increment != 1 ? 6 : 4);
        // "temperature GID THRU GID [BY n]" stays on one line, because the solver reads the THRU form field by field.
        const int position = (field - 1) % FIELDS_PER_LINE;
        if (position + needed > FIELDS_PER_LINE)
            field += FIELDS_PER_LINE - position;
        writer.real(field++, group.temperature);
        writer.integer(field++, group.grids.first);
        if (single)
            continue;
        writer.text(field++, "Through");
        writer.integer(field++, group.grids.last);
        if (group.grids.increment != 1)
        {
            writer.text(field++, "By");
            writer.integer(field++, group.grids.increment);
        }
    }
}

std::uint32_t TemperatureBoundaryTable::append(const TemperatureBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_kind.push(card.kind);
    std::vector<double> temperatures;
    std::vector<ThermalRun> runs;
    for (const TemperatureBoundaryGroup& group : card.groups)
    {
        temperatures.push_back(group.temperature);
        runs.push_back(group.grids);
    }
    m_temperatures.push(temperatures);
    m_runs.push(toThermalTriples(runs));
    return row;
}

TemperatureBoundaryCard TemperatureBoundaryTable::card(std::uint32_t row) const
{
    TemperatureBoundaryCard card;
    card.id = m_id.get(row);
    card.kind = m_kind.get(row);
    const auto temperatures = m_temperatures.get(row);
    const std::vector<ThermalRun> runs = toThermalRuns(m_runs.get(row));
    for (std::size_t index = 0; index < temperatures.size() && index < runs.size(); ++index)
        card.groups.push_back(TemperatureBoundaryGroup{temperatures[index], runs[index]});
    return card;
}

void TemperatureBoundaryTable::store(std::uint32_t row, const TemperatureBoundaryCard& card)
{
    m_id.set(row, card.id);
    m_kind.set(row, card.kind);
    std::vector<double> temperatures;
    std::vector<ThermalRun> runs;
    for (const TemperatureBoundaryGroup& group : card.groups)
    {
        temperatures.push_back(group.temperature);
        runs.push_back(group.grids);
    }
    m_temperatures.set(row, temperatures);
    m_runs.set(row, toThermalTriples(runs));
}

void TemperatureBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_kind.keepRows(rows);
    m_temperatures.keepRows(rows);
    m_runs.keepRows(rows);
}

void TemperatureBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitThermalRuns(m_runs, row, visitor, "nodeId", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
