#include "oor/model/cards/nodes/moduleTranslation/ModuleTranslationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleMoveSupport.hpp"

namespace oor::model
{

std::optional<ModuleTranslationCard> ModuleTranslationFormat::read(FieldReader& reader)
{
    ModuleTranslationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.startNode = reader.requiredInteger(2, "startNode");
    card.endNode = reader.requiredInteger(3, "endNode");
    card.magnitude = reader.real(4, "magnitude", 0.0);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleTranslationFormat::check(const ModuleTranslationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    // A vector has a start and an end: the same point twice has no direction.
    if (const auto problem = checkDistinctModulePoints<2>({card.startNode, card.endNode}, {"startNode", "endNode"}))
        return problem;
    // The format reference states no range for the magnitude: any finite real is kept; zero is the default, the length
    // of the vector.
    return checkReal(card.magnitude, "magnitude", RealRule::Finite);
}

void ModuleTranslationFormat::write(const ModuleTranslationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.startNode);
    writer.integer(3, card.endNode);
    writer.realUnlessDefault(4, card.magnitude, 0.0);
}

std::uint32_t ModuleTranslationTable::append(const ModuleTranslationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleTranslationTable::store(std::uint32_t row, const ModuleTranslationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleTranslationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleTranslationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleTranslationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleTranslationCard& card = m_cards.edit(row);
    visitId(visitor, "startNode", ReferenceTarget::Node, card.startNode);
    visitId(visitor, "endNode", ReferenceTarget::Node, card.endNode);
}

} // namespace oor::model
