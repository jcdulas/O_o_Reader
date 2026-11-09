#include "oor/model/cards/nonlinear_contact/contactEllipsoidList/ContactEllipsoidListTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

std::optional<ContactEllipsoidListCard> ContactEllipsoidListFormat::read(FieldReader& reader)
{
    ContactEllipsoidListCard card;
    card.id = reader.requiredInteger(1, "id");
    card.ellipsoidNames = readContactWords(reader, 2, reader.lastUsedField());
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactEllipsoidListFormat::check(const ContactEllipsoidListCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.ellipsoidNames.empty())
        return CardProblem{"card.missing-field", "the list needs at least one ellipsoid name", "ellipsoidNames"};
    for (const std::string& name : card.ellipsoidNames)
    {
        if (name.empty() || name.find_first_of(" ,$\t") != std::string::npos)
            return CardProblem{"card.bad-value", "an ellipsoid name is a word with no blank, comma or $",
                               "ellipsoidNames"};
    }
    return std::nullopt;
}

void ContactEllipsoidListFormat::write(const ContactEllipsoidListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.ellipsoidNames.size(); ++index)
        writer.text(2 + static_cast<int>(index), card.ellipsoidNames[index]);
}

std::uint32_t ContactEllipsoidListTable::append(const ContactEllipsoidListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ellipsoidNames.push(card.ellipsoidNames);
    return row;
}

ContactEllipsoidListCard ContactEllipsoidListTable::card(std::uint32_t row) const
{
    ContactEllipsoidListCard card;
    card.id = m_id.get(row);
    const auto ellipsoidNames = m_ellipsoidNames.get(row);
    card.ellipsoidNames.assign(ellipsoidNames.begin(), ellipsoidNames.end());
    return card;
}

void ContactEllipsoidListTable::store(std::uint32_t row, const ContactEllipsoidListCard& card)
{
    m_id.set(row, card.id);
    m_ellipsoidNames.set(row, card.ellipsoidNames);
}

void ContactEllipsoidListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ellipsoidNames.keepRows(rows);
}

} // namespace oor::model
