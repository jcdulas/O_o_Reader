#include "oor/model/cards/elements/springScalarInline/SpringScalarInlineTable.hpp"

namespace oor::model
{

namespace
{

// The element whose four fields start at `first` (1 for the first, 5 for the second).
SpringScalarInlineCard readElement(FieldReader& reader, int first, const char* const (&names)[4])
{
    SpringScalarInlineCard card;
    card.id = reader.requiredInteger(first, names[0]);
    card.stiffness = reader.requiredReal(first + 1, names[1]);
    card.scalarPoint1 = reader.integer(first + 2, names[2], 0); // blank: grounded
    card.scalarPoint2 = reader.integer(first + 3, names[3], 0);
    if (!reader.failed())
        reader.report(SpringScalarInlineFormat::check(card));
    return card;
}

} // namespace

std::vector<SpringScalarInlineCard> SpringScalarInlineFormat::readAll(FieldReader& reader)
{
    static constexpr const char* NAMES_FIRST[4] = {"id", "stiffness", "scalarPoint1", "scalarPoint2"};
    static constexpr const char* NAMES_SECOND[4] = {"id2", "stiffness2", "scalarPoint3", "scalarPoint4"};
    std::vector<SpringScalarInlineCard> elements;
    elements.push_back(readElement(reader, 1, NAMES_FIRST));
    // The format reference puts one element on a card; decks may put a second one in fields 5 to 8. It is there as soon
    // as one of its fields is, and its EID and K are then required.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7) || !reader.isBlank(8))
        elements.push_back(readElement(reader, 5, NAMES_SECOND));
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " defines at most two elements",
                     "scalarPoint4");
    return elements;
}

std::optional<CardProblem> SpringScalarInlineFormat::check(const SpringScalarInlineCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.scalarPoint1 < 0 || card.scalarPoint2 < 0)
        return CardProblem{"card.bad-id", "the scalar point IDs must not be negative", "scalarPoint1"};
    if (card.scalarPoint1 == 0 && card.scalarPoint2 == 0)
        return CardProblem{"card.missing-field", "{field} and {field:scalarPoint2} cannot both be blank or zero",
                           "scalarPoint1"};
    if (card.scalarPoint1 == card.scalarPoint2)
        return CardProblem{"card.duplicate-node", "the two scalar points must be different", "scalarPoint2"};
    return std::nullopt;
}

void SpringScalarInlineFormat::write(const SpringScalarInlineCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.stiffness);
    writer.integerUnlessDefault(3, card.scalarPoint1, 0);
    writer.integerUnlessDefault(4, card.scalarPoint2, 0);
}

std::uint32_t SpringScalarInlineTable::append(const SpringScalarInlineCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffness.push(card.stiffness);
    m_scalarPoint1.push(card.scalarPoint1);
    m_scalarPoint2.push(card.scalarPoint2);
    return row;
}

SpringScalarInlineCard SpringScalarInlineTable::card(std::uint32_t row) const
{
    return SpringScalarInlineCard{m_id.get(row), m_stiffness.get(row), m_scalarPoint1.get(row), m_scalarPoint2.get(row)};
}

void SpringScalarInlineTable::store(std::uint32_t row, const SpringScalarInlineCard& card)
{
    m_id.set(row, card.id);
    m_stiffness.set(row, card.stiffness);
    m_scalarPoint1.set(row, card.scalarPoint1);
    m_scalarPoint2.set(row, card.scalarPoint2);
}

void SpringScalarInlineTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_scalarPoint1.keepRows(rows);
    m_scalarPoint2.keepRows(rows);
}

void SpringScalarInlineTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // S1 and S2 are scalar points (SPOINT cards, or implicit: the format reference does not require an SPOINT entry for
    // them).
    if (const base::EntityId scalarPoint1 = m_scalarPoint1.get(row); scalarPoint1 > 0)
        m_scalarPoint1.set(row, visitor.visit("scalarPoint1", ReferenceTarget::ScalarOrGridPoint, scalarPoint1));
    if (const base::EntityId scalarPoint2 = m_scalarPoint2.get(row); scalarPoint2 > 0)
        m_scalarPoint2.set(row, visitor.visit("scalarPoint2", ReferenceTarget::ScalarOrGridPoint, scalarPoint2));
}

} // namespace oor::model
