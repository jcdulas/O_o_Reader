#include "oor/model/cards/properties/massProp/MassPropTable.hpp"

namespace oor::model
{

namespace
{

constexpr int MAX_PROPERTIES = 4;

} // namespace

std::vector<MassPropCard> MassPropFormat::readAll(FieldReader& reader)
{
    static constexpr const char* ID_NAMES[MAX_PROPERTIES] = {"id1", "id2", "id3", "id4"};
    static constexpr const char* M_NAMES[MAX_PROPERTIES] = {"mass1", "mass2", "mass3", "mass4"};
    std::vector<MassPropCard> properties;
    for (int index = 0; index < MAX_PROPERTIES; ++index)
    {
        const int field = 1 + 2 * index;
        // The first property is always read; a later one is there as soon as one of its two fields is.
        if (index > 0 && reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        MassPropCard card;
        card.id = reader.requiredInteger(field, ID_NAMES[index]);
        card.mass = reader.real(field + 1, M_NAMES[index], 0.0);
        if (!reader.failed())
            reader.report(check(card));
        properties.push_back(card);
    }
    if (reader.lastUsedField() > 2 * MAX_PROPERTIES)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " defines at most four properties",
                     "mass4");
    return properties;
}

std::optional<CardProblem> MassPropFormat::check(const MassPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    return std::nullopt;
}

void MassPropFormat::write(const MassPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.mass);
}

std::uint32_t MassPropTable::append(const MassPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_mass.push(card.mass);
    return row;
}

MassPropCard MassPropTable::card(std::uint32_t row) const
{
    return MassPropCard{m_id.get(row), m_mass.get(row)};
}

void MassPropTable::store(std::uint32_t row, const MassPropCard& card)
{
    m_id.set(row, card.id);
    m_mass.set(row, card.mass);
}

void MassPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_mass.keepRows(rows);
}

void MassPropTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // A PMASS refers to no other card.
}

} // namespace oor::model
