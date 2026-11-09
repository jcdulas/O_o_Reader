#include "oor/model/cards/aero/aeroNode/AeroNodeTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 7;

} // namespace

std::optional<AeroNodeCard> AeroNodeFormat::read(FieldReader& reader)
{
    AeroNodeCard card;
    card.id = reader.requiredInteger(1, "id");
    card.positionFrameId = reader.integer(2, "positionFrameId", 0);
    card.xCoord1 = reader.real(3, "xCoord1", 0.0);
    card.xCoord2 = reader.real(4, "xCoord2", 0.0);
    card.xCoord3 = reader.real(5, "xCoord3", 0.0);
    card.displacementFrameId = reader.integer(6, "displacementFrameId", 0);
    card.scaleId = reader.integer(7, "scaleId", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.report(CardProblem{"card.bad-value",
                                  "an " + std::string(reader.cardName()) +
                                      " has seven fields: {field:id}, {field:positionFrameId}, {field:xCoord1}, "
                                      "{field:xCoord2}, {field:xCoord3}, {field:displacementFrameId} and "
                                      "{field:scaleId}",
                                  ""});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroNodeFormat::check(const AeroNodeCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.positionFrameId, "positionFrameId"))
        return problem;
    if (const auto problem = checkReal(card.xCoord1, "xCoord1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.xCoord2, "xCoord2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.xCoord3, "xCoord3", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.displacementFrameId, "displacementFrameId"))
        return problem;
    return checkOptionalId(card.scaleId, "scaleId");
}

void AeroNodeFormat::write(const AeroNodeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.positionFrameId, 0);
    writer.real(3, card.xCoord1);
    writer.real(4, card.xCoord2);
    writer.real(5, card.xCoord3);
    writer.integerUnlessDefault(6, card.displacementFrameId, 0);
    writer.integerUnlessDefault(7, card.scaleId, 0);
}

std::uint32_t AeroNodeTable::append(const AeroNodeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroNodeTable::store(std::uint32_t row, const AeroNodeCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroNodeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroNodeTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroNodeTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroNodeCard& card = m_cards.edit(row);
    visitId(visitor, "positionFrameId", ReferenceTarget::Coordinate, card.positionFrameId);
    visitId(visitor, "displacementFrameId", ReferenceTarget::Coordinate, card.displacementFrameId);
    visitId(visitor, "scaleId", ReferenceTarget::AeroScale, card.scaleId);
}

} // namespace oor::model
