#include "oor/model/cards/nodes/moduleMirror1/ModuleMirror1Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleMoveSupport.hpp"

namespace oor::model
{

std::optional<ModuleMirror1Card> ModuleMirror1Format::read(FieldReader& reader)
{
    ModuleMirror1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.point1 = reader.requiredInteger(2, "point1");
    card.point2 = reader.requiredInteger(3, "point2");
    card.point3 = reader.requiredInteger(4, "point3");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleMirror1Format::check(const ModuleMirror1Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    // Three non-collinear points: at least three different points.
    return checkDistinctModulePoints<3>({card.point1, card.point2, card.point3}, {"point1", "point2", "point3"});
}

void ModuleMirror1Format::write(const ModuleMirror1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.point1);
    writer.integer(3, card.point2);
    writer.integer(4, card.point3);
}

std::uint32_t ModuleMirror1Table::append(const ModuleMirror1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleMirror1Table::store(std::uint32_t row, const ModuleMirror1Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleMirror1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleMirror1Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleMirror1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleMirror1Card& card = m_cards.edit(row);
    visitId(visitor, "point1", ReferenceTarget::Node, card.point1);
    visitId(visitor, "point2", ReferenceTarget::Node, card.point2);
    visitId(visitor, "point3", ReferenceTarget::Node, card.point3);
}

} // namespace oor::model
