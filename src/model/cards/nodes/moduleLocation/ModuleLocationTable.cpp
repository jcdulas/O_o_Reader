#include "oor/model/cards/nodes/moduleLocation/ModuleLocationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleMoveSupport.hpp"

namespace oor::model
{

std::optional<ModuleLocationCard> ModuleLocationFormat::read(FieldReader& reader)
{
    ModuleLocationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pointA1 = reader.requiredInteger(2, "pointA1");
    card.pointA2 = reader.requiredInteger(3, "pointA2");
    card.pointA3 = reader.requiredInteger(4, "pointA3");
    card.pointB1 = reader.requiredInteger(5, "pointB1");
    card.pointB2 = reader.requiredInteger(6, "pointB2");
    card.pointB3 = reader.requiredInteger(7, "pointB3");
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleLocationFormat::check(const ModuleLocationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    // Each triple is three non-collinear points, so three different points; the two triples are in different Bulk Data
    // sections and may use the same numbers.
    if (const auto problem =
            checkDistinctModulePoints<3>({card.pointA1, card.pointA2, card.pointA3}, {"pointA1", "pointA2", "pointA3"}))
        return problem;
    return checkDistinctModulePoints<3>({card.pointB1, card.pointB2, card.pointB3}, {"pointB1", "pointB2", "pointB3"});
}

void ModuleLocationFormat::write(const ModuleLocationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.pointA1);
    writer.integer(3, card.pointA2);
    writer.integer(4, card.pointA3);
    writer.integer(5, card.pointB1);
    writer.integer(6, card.pointB2);
    writer.integer(7, card.pointB3);
}

std::uint32_t ModuleLocationTable::append(const ModuleLocationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleLocationTable::store(std::uint32_t row, const ModuleLocationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleLocationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleLocationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleLocationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // Only the points of module 0 are references: the PAi are points of the module's own Bulk Data section.
    ModuleLocationCard& card = m_cards.edit(row);
    visitId(visitor, "pointB1", ReferenceTarget::Node, card.pointB1);
    visitId(visitor, "pointB2", ReferenceTarget::Node, card.pointB2);
    visitId(visitor, "pointB3", ReferenceTarget::Node, card.pointB3);
}

} // namespace oor::model
