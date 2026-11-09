#include "oor/model/cards/explicit/gasBagSwitch/GasBagSwitchTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<GasBagSwitchCard> GasBagSwitchFormat::read(FieldReader& reader)
{
    GasBagSwitchCard card;
    card.id = reader.requiredInteger(1, "id");
    card.couplingId = reader.requiredInteger(2, "couplingId");
    card.gasBagId = reader.integer(3, "gasBagId", 0);
    card.startTime = reader.real(4, "startTime", 0.0);
    card.validityPercent = reader.real(5, "validityPercent", 0.0);
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasBagSwitchFormat::check(const GasBagSwitchCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.couplingId, "couplingId"))
        return problem;
    if (const auto problem = checkOptionalId(card.gasBagId, "gasBagId"))
        return problem;
    if (const auto problem = checkReal(card.startTime, "startTime", RealRule::NonNegative))
        return problem;
    return checkReal(card.validityPercent, "validityPercent", RealRule::NonNegative);
}

void GasBagSwitchFormat::write(const GasBagSwitchCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.couplingId);
    writer.integerUnlessDefault(3, card.gasBagId, 0);
    writer.realUnlessDefault(4, card.startTime, 0.0);
    writer.realUnlessDefault(5, card.validityPercent, 0.0);
}

std::uint32_t GasBagSwitchTable::append(const GasBagSwitchCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void GasBagSwitchTable::store(std::uint32_t row, const GasBagSwitchCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void GasBagSwitchTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void GasBagSwitchTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void GasBagSwitchTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    GasBagSwitchCard& card = m_cards.edit(row);
    visitId(visitor, "couplingId", ReferenceTarget::Coupling, card.couplingId);
    visitId(visitor, "gasBagId", ReferenceTarget::GasBag, card.gasBagId);
}

} // namespace oor::model
