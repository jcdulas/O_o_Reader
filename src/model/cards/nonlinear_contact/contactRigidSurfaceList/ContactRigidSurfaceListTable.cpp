#include "oor/model/cards/nonlinear_contact/contactRigidSurfaceList/ContactRigidSurfaceListTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

std::optional<ContactRigidSurfaceListCard> ContactRigidSurfaceListFormat::read(FieldReader& reader)
{
    ContactRigidSurfaceListCard card;
    card.id = reader.requiredInteger(1, "id");
    card.surfaces = readContactIds(reader, 2, reader.lastUsedField(), "rigidSurfaceIds");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactRigidSurfaceListFormat::check(const ContactRigidSurfaceListCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.surfaces.empty())
        return CardProblem{"card.missing-field", "the list needs at least one rigid surface", "rigidSurfaceIds"};
    for (const base::EntityId surface : card.surfaces)
    {
        if (surface <= 0)
            return CardProblem{"card.bad-id", "a rigid surface ID must be a positive integer", "rigidSurfaceIds"};
    }
    return std::nullopt;
}

void ContactRigidSurfaceListFormat::write(const ContactRigidSurfaceListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.surfaces.size(); ++index)
        writer.integer(2 + static_cast<int>(index), card.surfaces[index]);
}

std::uint32_t ContactRigidSurfaceListTable::append(const ContactRigidSurfaceListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_surfaces.push(card.surfaces);
    return row;
}

ContactRigidSurfaceListCard ContactRigidSurfaceListTable::card(std::uint32_t row) const
{
    ContactRigidSurfaceListCard card;
    card.id = m_id.get(row);
    const auto surfaces = m_surfaces.get(row);
    card.surfaces.assign(surfaces.begin(), surfaces.end());
    return card;
}

void ContactRigidSurfaceListTable::store(std::uint32_t row, const ContactRigidSurfaceListCard& card)
{
    m_id.set(row, card.id);
    m_surfaces.set(row, card.surfaces);
}

void ContactRigidSurfaceListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_surfaces.keepRows(rows);
}

void ContactRigidSurfaceListTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdColumn(m_surfaces, row, visitor, "rigidSurfaceIds", ReferenceTarget::ContactSurface);
}

} // namespace oor::model
