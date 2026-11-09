#include "oor/model/cards/thermal/radiationCavityList/RadiationCavityListTable.hpp"

#include <string>

namespace oor::model
{

std::optional<RadiationCavityListCard> RadiationCavityListFormat::read(FieldReader& reader)
{
    RadiationCavityListCard card;
    card.cavityId = reader.requiredInteger(1, "cavityId");
    card.matrixKind = reader.optionalInteger(2, "matrixKind");
    card.elements = readThermalRuns(reader, 3, "elements", false);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiationCavityListFormat::check(const RadiationCavityListCard& card)
{
    if (card.cavityId <= 0)
        return CardProblem{"card.bad-id", "the cavity ID must be a positive integer", "cavityId"};
    // format reference: MTXTYP is at most 4 and not 0 (a negative one is -n, the number of elements that may lose
    // energy to space).
    if (card.matrixKind && (*card.matrixKind == 0 || *card.matrixKind > 4))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be 1 to 4, or negative (or blank)", "matrixKind"};
    for (const ThermalRun& run : card.elements)
    {
        if (run.increment != 1)
            return CardProblem{"card.bad-value",
                               std::string("a range of this card has no ") + std::string(CardProblem::FIELD),
                               "increment"};
    }
    return checkThermalRuns(card.elements, "elements", true);
}

void RadiationCavityListFormat::write(const RadiationCavityListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.cavityId);
    if (card.matrixKind)
        writer.integer(2, *card.matrixKind);
    writeThermalRuns(writer, 3, card.elements);
}

std::uint32_t RadiationCavityListTable::append(const RadiationCavityListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cavityId.push(card.cavityId);
    m_matrixKind.push(card.matrixKind.value_or(NO_MATRIX_TYPE));
    m_runs.push(toThermalTriples(card.elements));
    return row;
}

RadiationCavityListCard RadiationCavityListTable::card(std::uint32_t row) const
{
    RadiationCavityListCard card;
    card.cavityId = m_cavityId.get(row);
    if (const std::int32_t matrixKind = m_matrixKind.get(row); matrixKind != NO_MATRIX_TYPE)
        card.matrixKind = matrixKind;
    card.elements = toThermalRuns(m_runs.get(row));
    return card;
}

void RadiationCavityListTable::store(std::uint32_t row, const RadiationCavityListCard& card)
{
    m_cavityId.set(row, card.cavityId);
    m_matrixKind.set(row, card.matrixKind.value_or(NO_MATRIX_TYPE));
    m_runs.set(row, toThermalTriples(card.elements));
}

void RadiationCavityListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_cavityId.keepRows(rows);
    m_matrixKind.keepRows(rows);
    m_runs.keepRows(rows);
}

void RadiationCavityListTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitThermalRuns(m_runs, row, visitor, "elements", ReferenceTarget::SurfaceElement,
                     ReferenceTarget::SurfaceElementInRange);
}

} // namespace oor::model
