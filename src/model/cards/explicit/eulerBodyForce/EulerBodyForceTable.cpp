#include "oor/model/cards/explicit/eulerBodyForce/EulerBodyForceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<EulerBodyForceCard> EulerBodyForceFormat::read(FieldReader& reader)
{
    EulerBodyForceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.frameId = reader.integer(2, "frameId", 0);
    card.scaleFactor = reader.real(3, "scaleFactor", 0.0);
    card.tableId = reader.integer(4, "tableId", 0);
    card.directionX = reader.real(5, "directionX", 0.0);
    card.directionY = reader.real(6, "directionY", 0.0);
    card.directionZ = reader.real(7, "directionZ", 0.0);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerBodyForceFormat::check(const EulerBodyForceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalId(card.tableId, "tableId"))
        return problem;
    if (const auto problem = checkReal(card.directionX, "directionX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.directionY, "directionY", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.directionZ, "directionZ", RealRule::Finite))
        return problem;
    // either the scale factor or the table must be given.
    if (card.scaleFactor == 0.0 && card.tableId == 0)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " or " + CardProblem::fieldNamed("tableId") + " must be given",
            "scaleFactor"};
    // a direction needs a nonzero component.
    if (card.directionX == 0.0 && card.directionY == 0.0 && card.directionZ == 0.0)
        return CardProblem{"card.missing-field",
                           "at least one of " + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("directionY") + " and " + CardProblem::fieldNamed("directionZ") +
                               " must be nonzero",
                           "directionX"};
    return std::nullopt;
}

void EulerBodyForceFormat::write(const EulerBodyForceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.frameId, 0);
    writer.realUnlessDefault(3, card.scaleFactor, 0.0);
    writer.integerUnlessDefault(4, card.tableId, 0);
    writer.realUnlessDefault(5, card.directionX, 0.0);
    writer.realUnlessDefault(6, card.directionY, 0.0);
    writer.realUnlessDefault(7, card.directionZ, 0.0);
}

std::uint32_t EulerBodyForceTable::append(const EulerBodyForceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerBodyForceTable::store(std::uint32_t row, const EulerBodyForceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerBodyForceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerBodyForceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerBodyForceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerBodyForceCard& card = m_cards.edit(row);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
    visitId(visitor, "tableId", ReferenceTarget::DynamicTable, card.tableId);
}

} // namespace oor::model
