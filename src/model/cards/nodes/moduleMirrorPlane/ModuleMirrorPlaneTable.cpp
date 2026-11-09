#include "oor/model/cards/nodes/moduleMirrorPlane/ModuleMirrorPlaneTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleMoveSupport.hpp"

namespace oor::model
{

std::optional<ModuleMirrorPlaneCard> ModuleMirrorPlaneFormat::read(FieldReader& reader)
{
    ModuleMirrorPlaneCard card;
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

std::optional<CardProblem> ModuleMirrorPlaneFormat::check(const ModuleMirrorPlaneCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    // Three non-collinear points: at least three different points.
    return checkDistinctModulePoints<3>({card.point1, card.point2, card.point3}, {"point1", "point2", "point3"});
}

void ModuleMirrorPlaneFormat::write(const ModuleMirrorPlaneCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.point1);
    writer.integer(3, card.point2);
    writer.integer(4, card.point3);
}

std::uint32_t ModuleMirrorPlaneTable::append(const ModuleMirrorPlaneCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleMirrorPlaneTable::store(std::uint32_t row, const ModuleMirrorPlaneCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleMirrorPlaneTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleMirrorPlaneTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleMirrorPlaneTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleMirrorPlaneCard& card = m_cards.edit(row);
    visitId(visitor, "point1", ReferenceTarget::Node, card.point1);
    visitId(visitor, "point2", ReferenceTarget::Node, card.point2);
    visitId(visitor, "point3", ReferenceTarget::Node, card.point3);
}

} // namespace oor::model
