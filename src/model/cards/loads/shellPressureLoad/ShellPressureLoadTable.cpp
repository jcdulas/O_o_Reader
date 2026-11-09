#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ELEMENT_FIELD = 3;

} // namespace

std::optional<ShellPressureLoadCard> ShellPressureLoadFormat::read(FieldReader& reader)
{
    ShellPressureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pressure = reader.requiredReal(2, "pressure");
    card.elements = readElementRuns(reader, FIRST_ELEMENT_FIELD, "elemId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellPressureLoadFormat::check(const ShellPressureLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    // at least one positive EID on each entry.
    if (card.elements.empty())
        return CardProblem{"card.missing-field", "the card needs at least one element", "elemId1"};
    if (auto problem = checkElementRuns(card.elements, "elemId"))
        return problem;
    // The alternate format is "EID1 THRU EID2" and nothing else.
    for (const ElementRun& run : card.elements)
    {
        if (run.first != run.last && card.elements.size() != 1)
            return CardProblem{"card.bad-value",
                               "a THRU range is the whole list of an element list (" +
                                   CardProblem::fieldNamed("elemId1") + " THRU " + CardProblem::fieldNamed("elemId2") +
                                   ")",
                               "elemId"};
    }
    return std::nullopt;
}

void ShellPressureLoadFormat::write(const ShellPressureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.pressure);
    // The THRU form of PLOAD2 is the whole list: the writer compresses only a list that is one run.
    writeElementRuns(writer, FIRST_ELEMENT_FIELD, card.elements, writer.compressesRanges() && isOneRun(card.elements));
}

std::uint32_t ShellPressureLoadTable::append(const ShellPressureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_pressure.push(card.pressure);
    m_runs.push(toPairs(card.elements));
    return row;
}

ShellPressureLoadCard ShellPressureLoadTable::card(std::uint32_t row) const
{
    ShellPressureLoadCard card;
    card.id = m_id.get(row);
    card.pressure = m_pressure.get(row);
    card.elements = toRuns(m_runs.get(row));
    return card;
}

void ShellPressureLoadTable::store(std::uint32_t row, const ShellPressureLoadCard& card)
{
    m_id.set(row, card.id);
    m_pressure.set(row, card.pressure);
    m_runs.set(row, toPairs(card.elements));
}

void ShellPressureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_pressure.keepRows(rows);
    m_runs.keepRows(rows);
}

void ShellPressureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitElementRuns(m_runs, row, visitor, "elemId");
}

} // namespace oor::model
