#include "oor/model/cards/properties/viscousDamperProp/ViscousDamperPropTable.hpp"

namespace oor::model
{

namespace
{

// The property whose three fields start at `first` (1 for the first, 5 for the second).
ViscousDamperPropCard readProperty(FieldReader& reader, int first, const char* const (&names)[3])
{
    ViscousDamperPropCard card;
    card.id = reader.requiredInteger(first, names[0]);
    card.extensionalDamping = reader.requiredReal(first + 1, names[1]);
    card.rotationalDamping = reader.real(first + 2, names[2], 0.0);
    if (!reader.failed())
        reader.report(ViscousDamperPropFormat::check(card));
    return card;
}

} // namespace

std::vector<ViscousDamperPropCard> ViscousDamperPropFormat::readAll(FieldReader& reader)
{
    static constexpr const char* NAMES_FIRST[3] = {"id1", "extensionalDamping1", "rotationalDamping1"};
    static constexpr const char* NAMES_SECOND[3] = {"id2", "extensionalDamping2", "rotationalDamping2"};
    std::vector<ViscousDamperPropCard> properties;
    properties.push_back(readProperty(reader, 1, NAMES_FIRST));
    // The second property starts in field 5 (field 4 is blank, format reference); it is there as soon as one of its
    // fields is, and its PID2 and CE2 are then required.
    if (!reader.isBlank(4))
        reader.error(
            "card.bad-value",
            "field 4 of a " + std::string(reader.cardName()) + " is blank: the second property starts in field 5",
            "id2");
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7))
        properties.push_back(readProperty(reader, 5, NAMES_SECOND));
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " defines at most two properties",
                     "rotationalDamping2");
    return properties;
}

std::optional<CardProblem> ViscousDamperPropFormat::check(const ViscousDamperPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    return std::nullopt;
}

void ViscousDamperPropFormat::write(const ViscousDamperPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.extensionalDamping);
    writer.realUnlessDefault(3, card.rotationalDamping, 0.0);
}

std::uint32_t ViscousDamperPropTable::append(const ViscousDamperPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_extensionalDamping.push(card.extensionalDamping);
    m_rotationalDamping.push(card.rotationalDamping);
    return row;
}

ViscousDamperPropCard ViscousDamperPropTable::card(std::uint32_t row) const
{
    return ViscousDamperPropCard{m_id.get(row), m_extensionalDamping.get(row), m_rotationalDamping.get(row)};
}

void ViscousDamperPropTable::store(std::uint32_t row, const ViscousDamperPropCard& card)
{
    m_id.set(row, card.id);
    m_extensionalDamping.set(row, card.extensionalDamping);
    m_rotationalDamping.set(row, card.rotationalDamping);
}

void ViscousDamperPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_extensionalDamping.keepRows(rows);
    m_rotationalDamping.keepRows(rows);
}

void ViscousDamperPropTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // A PVISC refers to no other card.
}

} // namespace oor::model
