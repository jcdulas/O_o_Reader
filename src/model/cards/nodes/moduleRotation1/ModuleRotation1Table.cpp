#include "oor/model/cards/nodes/moduleRotation1/ModuleRotation1Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleMoveSupport.hpp"

namespace oor::model
{

std::optional<ModuleRotation1Card> ModuleRotation1Format::read(FieldReader& reader)
{
    ModuleRotation1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceNode = reader.integer(2, "referenceNode", 0);
    card.startNode = reader.requiredInteger(3, "startNode");
    card.endNode = reader.requiredInteger(4, "endNode");
    card.magnitude = reader.requiredReal(5, "magnitude");
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleRotation1Format::check(const ModuleRotation1Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.referenceNode, "referenceNode"))
        return problem;
    // The axis is the vector from IDS to IDE: two different points.
    if (const auto problem = checkDistinctModulePoints<2>({card.startNode, card.endNode}, {"startNode", "endNode"}))
        return problem;
    if (const auto problem = checkReal(card.magnitude, "magnitude", RealRule::Finite))
        return problem;
    if (card.magnitude == 0.0)
        return CardProblem{"card.bad-value", "{field} must not be zero", "magnitude"};
    return std::nullopt;
}

void ModuleRotation1Format::write(const ModuleRotation1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.referenceNode, 0);
    writer.integer(3, card.startNode);
    writer.integer(4, card.endNode);
    writer.real(5, card.magnitude);
}

std::uint32_t ModuleRotation1Table::append(const ModuleRotation1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleRotation1Table::store(std::uint32_t row, const ModuleRotation1Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleRotation1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleRotation1Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleRotation1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleRotation1Card& card = m_cards.edit(row);
    visitId(visitor, "referenceNode", ReferenceTarget::Node, card.referenceNode);
    visitId(visitor, "startNode", ReferenceTarget::Node, card.startNode);
    visitId(visitor, "endNode", ReferenceTarget::Node, card.endNode);
}

} // namespace oor::model
