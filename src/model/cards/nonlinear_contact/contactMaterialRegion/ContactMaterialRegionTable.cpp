#include "oor/model/cards/nonlinear_contact/contactMaterialRegion/ContactMaterialRegionTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

std::optional<ContactMaterialRegionCard> ContactMaterialRegionFormat::read(FieldReader& reader)
{
    ContactMaterialRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.materials = readContactIds(reader, 2, reader.lastUsedField(), "matIds");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactMaterialRegionFormat::check(const ContactMaterialRegionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.materials.empty())
        return CardProblem{"card.missing-field", "the region needs at least one material", "matIds"};
    for (const base::EntityId material : card.materials)
    {
        if (material <= 0)
            return CardProblem{"card.bad-id", "a material ID must be a positive integer", "matIds"};
    }
    return std::nullopt;
}

void ContactMaterialRegionFormat::write(const ContactMaterialRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.materials.size(); ++index)
        writer.integer(2 + static_cast<int>(index), card.materials[index]);
}

std::uint32_t ContactMaterialRegionTable::append(const ContactMaterialRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_materials.push(card.materials);
    return row;
}

ContactMaterialRegionCard ContactMaterialRegionTable::card(std::uint32_t row) const
{
    ContactMaterialRegionCard card;
    card.id = m_id.get(row);
    const auto materials = m_materials.get(row);
    card.materials.assign(materials.begin(), materials.end());
    return card;
}

void ContactMaterialRegionTable::store(std::uint32_t row, const ContactMaterialRegionCard& card)
{
    m_id.set(row, card.id);
    m_materials.set(row, card.materials);
}

void ContactMaterialRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_materials.keepRows(rows);
}

void ContactMaterialRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdColumn(m_materials, row, visitor, "matIds", ReferenceTarget::Material);
}

} // namespace oor::model
