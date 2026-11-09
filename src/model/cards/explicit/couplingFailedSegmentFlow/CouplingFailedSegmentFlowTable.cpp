#include "oor/model/cards/explicit/couplingFailedSegmentFlow/CouplingFailedSegmentFlowTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<CouplingFailedSegmentFlowCard> CouplingFailedSegmentFlowFormat::read(FieldReader& reader)
{
    CouplingFailedSegmentFlowCard card;
    card.id = reader.requiredInteger(1, "id");
    card.density = reader.optionalReal(2, "density");
    card.specificEnergy = reader.optionalReal(3, "specificEnergy");
    card.xVelocity = reader.optionalReal(4, "xVelocity");
    card.yVelocity = reader.optionalReal(5, "yVelocity");
    card.zVelocity = reader.optionalReal(6, "zVelocity");
    card.pressure = reader.optionalReal(7, "pressure");
    card.matId = reader.integer(8, "matId", 0);
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CouplingFailedSegmentFlowFormat::check(const CouplingFailedSegmentFlowCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.density, "density", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificEnergy, "specificEnergy", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.xVelocity, "xVelocity", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.yVelocity, "yVelocity", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.zVelocity, "zVelocity", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.pressure, "pressure", RealRule::Finite))
        return problem;
    return checkOptionalId(card.matId, "matId");
}

void CouplingFailedSegmentFlowFormat::write(const CouplingFailedSegmentFlowCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.density)
        writer.real(2, *card.density);
    if (card.specificEnergy)
        writer.real(3, *card.specificEnergy);
    if (card.xVelocity)
        writer.real(4, *card.xVelocity);
    if (card.yVelocity)
        writer.real(5, *card.yVelocity);
    if (card.zVelocity)
        writer.real(6, *card.zVelocity);
    if (card.pressure)
        writer.real(7, *card.pressure);
    writer.integerUnlessDefault(8, card.matId, 0);
}

std::uint32_t CouplingFailedSegmentFlowTable::append(const CouplingFailedSegmentFlowCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CouplingFailedSegmentFlowTable::store(std::uint32_t row, const CouplingFailedSegmentFlowCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CouplingFailedSegmentFlowTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CouplingFailedSegmentFlowTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CouplingFailedSegmentFlowTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "matId", ReferenceTarget::Material, m_cards.edit(row).matId);
}

} // namespace oor::model
