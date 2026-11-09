#include "oor/model/cards/nodes/moduleRotation2/ModuleRotation2Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<ModuleRotation2Card> ModuleRotation2Format::read(FieldReader& reader)
{
    ModuleRotation2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.frameId = reader.integer(2, "frameId", 0);
    card.referenceNode = reader.integer(3, "referenceNode", 0);
    card.axis = readRequiredKeyword<ModuleRotation2Axis>(reader, 4, "axis", MODULE_ROTATION2_AXIS_WORDS);
    card.magnitude = reader.requiredReal(5, "magnitude");
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleRotation2Format::check(const ModuleRotation2Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    if (const auto problem = checkOptionalId(card.referenceNode, "referenceNode"))
        return problem;
    if (const auto problem = checkReal(card.magnitude, "magnitude", RealRule::Finite))
        return problem;
    if (card.magnitude == 0.0)
        return CardProblem{"card.bad-value", "{field} must not be zero", "magnitude"};
    return std::nullopt;
}

void ModuleRotation2Format::write(const ModuleRotation2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.frameId, 0);
    writer.integerUnlessDefault(3, card.referenceNode, 0);
    writeKeyword(writer, 4, MODULE_ROTATION2_AXIS_WORDS, card.axis);
    writer.real(5, card.magnitude);
}

std::uint32_t ModuleRotation2Table::append(const ModuleRotation2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleRotation2Table::store(std::uint32_t row, const ModuleRotation2Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleRotation2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleRotation2Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleRotation2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleRotation2Card& card = m_cards.edit(row);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
    visitId(visitor, "referenceNode", ReferenceTarget::Node, card.referenceNode);
}

} // namespace oor::model
