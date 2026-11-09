#include "oor/model/cards/properties/damperProp/DamperPropTable.hpp"

namespace oor::model
{

namespace
{

constexpr int MAX_PROPERTIES = 4;

} // namespace

std::vector<DamperPropCard> DamperPropFormat::readAll(FieldReader& reader)
{
    static constexpr const char* ID_NAMES[MAX_PROPERTIES] = {"id1", "id2", "id3", "id4"};
    static constexpr const char* B_NAMES[MAX_PROPERTIES] = {"damping1", "damping2", "damping3", "damping4"};
    std::vector<DamperPropCard> properties;
    for (int index = 0; index < MAX_PROPERTIES; ++index)
    {
        const int field = 1 + 2 * index;
        // The first property is always read; a later one is there as soon as one of its two fields is, and has both.
        if (index > 0 && reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        DamperPropCard card;
        card.id = reader.requiredInteger(field, ID_NAMES[index]);
        card.damping = reader.requiredReal(field + 1, B_NAMES[index]);
        if (!reader.failed())
            reader.report(check(card));
        properties.push_back(card);
    }
    if (reader.lastUsedField() > 2 * MAX_PROPERTIES)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " defines at most four properties",
                     "damping4");
    return properties;
}

std::optional<CardProblem> DamperPropFormat::check(const DamperPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    return std::nullopt;
}

void DamperPropFormat::write(const DamperPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.damping);
}

std::uint32_t DamperPropTable::append(const DamperPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_damping.push(card.damping);
    return row;
}

DamperPropCard DamperPropTable::card(std::uint32_t row) const
{
    return DamperPropCard{m_id.get(row), m_damping.get(row)};
}

void DamperPropTable::store(std::uint32_t row, const DamperPropCard& card)
{
    m_id.set(row, card.id);
    m_damping.set(row, card.damping);
}

void DamperPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_damping.keepRows(rows);
}

void DamperPropTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // A PDAMP refers to no other card.
}

} // namespace oor::model
