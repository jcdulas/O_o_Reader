#include "oor/model/cards/solution_control/cosimulationSelection/CosimulationSelectionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_SERVICE_FIELD = 9; // CSRVID1, the first field of the continuation line
constexpr int LAST_SERVICE_FIELD = 11; // CSRVID3: the last field of the card
constexpr std::size_t MOST_SERVICES = 3;

} // namespace

std::optional<CosimulationSelectionCard> CosimulationSelectionFormat::read(FieldReader& reader)
{
    CosimulationSelectionCard card;
    card.id = reader.requiredInteger(1, "id");
    requireBlankFields(reader, {2, 3, 4, 5, 6, 7, 8}, LAST_SERVICE_FIELD);
    for (int field = FIRST_SERVICE_FIELD; field <= LAST_SERVICE_FIELD && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.services.push_back(reader.requiredInteger(field, "serviceIds"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CosimulationSelectionFormat::check(const CosimulationSelectionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.services.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one service (" + std::string(CardProblem::FIELD) + ")",
                           "serviceId1"};
    if (card.services.size() > MOST_SERVICES)
        return CardProblem{"card.bad-value", "the card has at most three services", "serviceId3"};
    for (const base::EntityId service : card.services)
    {
        if (const auto problem = checkRequiredId(service, "serviceIds"))
            return problem;
    }
    return std::nullopt;
}

void CosimulationSelectionFormat::write(const CosimulationSelectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_SERVICE_FIELD;
    for (const base::EntityId service : card.services)
        writer.integer(field++, service);
}

std::uint32_t CosimulationSelectionTable::append(const CosimulationSelectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CosimulationSelectionTable::store(std::uint32_t row, const CosimulationSelectionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CosimulationSelectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CosimulationSelectionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CosimulationSelectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The SID is selected by a Case Control command and names nothing: only the services are references.
    for (base::EntityId& service : m_cards.edit(row).services)
        visitId(visitor, "serviceIds", ReferenceTarget::CosimulationService, service);
}

} // namespace oor::model
