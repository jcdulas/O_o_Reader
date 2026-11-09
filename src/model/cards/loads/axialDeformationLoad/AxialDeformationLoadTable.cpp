#include "oor/model/cards/loads/axialDeformationLoad/AxialDeformationLoadTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_DEFORMATION_FIELD = 2;

} // namespace

std::optional<AxialDeformationLoadCard> AxialDeformationLoadFormat::read(FieldReader& reader)
{
    AxialDeformationLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    // The pairs (EIDi, Di) follow each other from field 2. A pair with both fields blank is padding.
    const int last = reader.lastUsedField();
    for (int field = FIRST_DEFORMATION_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        ElementDeformation deformation;
        deformation.element = reader.requiredInteger(field, "elemId");
        deformation.deformation = reader.requiredReal(field + 1, "deformation");
        card.deformations.push_back(deformation);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxialDeformationLoadFormat::check(const AxialDeformationLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the deformation set ID must be a positive integer", "id"};
    if (card.deformations.empty())
        return CardProblem{"card.missing-field", "the card needs at least one element", "elemId1"};
    // one to three deformations on a card.
    if (card.deformations.size() > AxialDeformationLoadCard::MAX_DEFORMATIONS)
        return CardProblem{"card.bad-value", "the card defines at most three deformations", "elemId3"};
    for (const ElementDeformation& deformation : card.deformations)
    {
        if (deformation.element <= 0)
            return CardProblem{"card.bad-id", "an element ID must be a positive integer", "elemId1"};
    }
    return std::nullopt;
}

void AxialDeformationLoadFormat::write(const AxialDeformationLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_DEFORMATION_FIELD;
    for (const ElementDeformation& deformation : card.deformations)
    {
        writer.integer(field, deformation.element);
        writer.real(field + 1, deformation.deformation);
        field += 2;
    }
}

std::uint32_t AxialDeformationLoadTable::append(const AxialDeformationLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> elements;
    std::vector<double> deformations;
    for (const ElementDeformation& deformation : card.deformations)
    {
        elements.push_back(deformation.element);
        deformations.push_back(deformation.deformation);
    }
    m_element.push(elements);
    m_deformation.push(deformations);
    return row;
}

AxialDeformationLoadCard AxialDeformationLoadTable::card(std::uint32_t row) const
{
    AxialDeformationLoadCard card;
    card.id = m_id.get(row);
    const auto elements = m_element.get(row);
    const auto deformations = m_deformation.get(row);
    for (std::size_t index = 0; index < elements.size(); ++index)
        card.deformations.push_back(ElementDeformation{elements[index], deformations[index]});
    return card;
}

void AxialDeformationLoadTable::store(std::uint32_t row, const AxialDeformationLoadCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> elements;
    std::vector<double> deformations;
    for (const ElementDeformation& deformation : card.deformations)
    {
        elements.push_back(deformation.element);
        deformations.push_back(deformation.deformation);
    }
    m_element.set(row, elements);
    m_deformation.set(row, deformations);
}

void AxialDeformationLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_element.keepRows(rows);
    m_deformation.keepRows(rows);
}

void AxialDeformationLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::vector<base::EntityId> elements(m_element.get(row).begin(), m_element.get(row).end());
    bool changed = false;
    for (base::EntityId& element : elements)
    {
        const base::EntityId updated = visitor.visit("elemId", ReferenceTarget::Element, element);
        changed = changed || updated != element;
        element = updated;
    }
    if (changed)
        m_element.set(row, elements);
}

} // namespace oor::model
