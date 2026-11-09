#include "oor/model/cards/properties/springProp/SpringPropTable.hpp"

namespace oor::model
{

namespace
{

// The property whose four fields start at `first` (1 for the first, 5 for the second).
SpringPropCard readProperty(FieldReader& reader, int first, const char* const (&names)[4])
{
    SpringPropCard card;
    card.id = reader.requiredInteger(first, names[0]);
    card.stiffness = reader.requiredReal(first + 1, names[1]);
    card.structuralDamping = reader.real(first + 2, names[2], 0.0);
    card.stressCoefficient = reader.real(first + 3, names[3], 0.0);
    if (!reader.failed())
        reader.report(SpringPropFormat::check(card));
    return card;
}

} // namespace

std::vector<SpringPropCard> SpringPropFormat::readAll(FieldReader& reader)
{
    static constexpr const char* NAMES_FIRST[4] = {"id1", "stiffness1", "structuralDamping1", "stressCoefficient1"};
    static constexpr const char* NAMES_SECOND[4] = {"id2", "stiffness2", "structuralDamping2", "stressCoefficient2"};
    std::vector<SpringPropCard> properties;
    properties.push_back(readProperty(reader, 1, NAMES_FIRST));
    // The second property is there as soon as one of its fields is; its PID2 and K2 are then required.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7) || !reader.isBlank(8))
        properties.push_back(readProperty(reader, 5, NAMES_SECOND));
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " defines at most two properties",
                     "stressCoefficient2");
    return properties;
}

std::optional<CardProblem> SpringPropFormat::check(const SpringPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    return std::nullopt;
}

void SpringPropFormat::write(const SpringPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.stiffness);
    writer.realUnlessDefault(3, card.structuralDamping, 0.0);
    writer.realUnlessDefault(4, card.stressCoefficient, 0.0);
}

std::uint32_t SpringPropTable::append(const SpringPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffness.push(card.stiffness);
    m_structuralDamping.push(card.structuralDamping);
    m_stressCoefficient.push(card.stressCoefficient);
    return row;
}

SpringPropCard SpringPropTable::card(std::uint32_t row) const
{
    return SpringPropCard{m_id.get(row), m_stiffness.get(row), m_structuralDamping.get(row),
                          m_stressCoefficient.get(row)};
}

void SpringPropTable::store(std::uint32_t row, const SpringPropCard& card)
{
    m_id.set(row, card.id);
    m_stiffness.set(row, card.stiffness);
    m_structuralDamping.set(row, card.structuralDamping);
    m_stressCoefficient.set(row, card.stressCoefficient);
}

void SpringPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_stressCoefficient.keepRows(rows);
}

void SpringPropTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // A PELAS refers to no other card.
}

} // namespace oor::model
