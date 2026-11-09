#include "oor/model/cards/nonlinear_contact/contactSetUnion/ContactSetUnionTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <algorithm>

namespace oor::model
{

std::optional<ContactSetUnionCard> ContactSetUnionFormat::read(FieldReader& reader)
{
    ContactSetUnionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.sets = readContactIds(reader, 2, reader.lastUsedField(), "memberSetId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactSetUnionFormat::check(const ContactSetUnionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.sets.empty())
        return CardProblem{"card.missing-field", "the union needs at least one contact set", "memberSetId"};
    for (const base::EntityId set : card.sets)
    {
        if (set <= 0)
            return CardProblem{"card.bad-id", "a contact set ID must be a positive integer", "memberSetId"};
        // Si is not the ID of this or of any other BCTADD, which the table checks across the
        // cards.
        if (set == card.id)
            return CardProblem{"card.bad-id", "a contact set of the union is not the ID of the union itself",
                               "memberSetId"};
    }
    return std::nullopt;
}

void ContactSetUnionFormat::write(const ContactSetUnionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.sets.size(); ++index)
        writer.integer(2 + static_cast<int>(index), card.sets[index]);
}

std::uint32_t ContactSetUnionTable::append(const ContactSetUnionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_sets.push(card.sets);
    return row;
}

ContactSetUnionCard ContactSetUnionTable::card(std::uint32_t row) const
{
    ContactSetUnionCard card;
    card.id = m_id.get(row);
    const auto sets = m_sets.get(row);
    card.sets.assign(sets.begin(), sets.end());
    return card;
}

void ContactSetUnionTable::store(std::uint32_t row, const ContactSetUnionCard& card)
{
    m_id.set(row, card.id);
    m_sets.set(row, card.sets);
}

void ContactSetUnionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_sets.keepRows(rows);
}

void ContactSetUnionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdColumn(m_sets, row, visitor, "memberSetId", ReferenceTarget::ContactTable);
}

} // namespace oor::model
