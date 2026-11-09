#include "oor/model/cards/aero/aeroBodyProp2/AeroBodyProp2Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 12;

} // namespace

std::optional<AeroBodyProp2Card> AeroBodyProp2Format::read(FieldReader& reader)
{
    AeroBodyProp2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.orientation =
        readRequiredKeyword<BodyOrientation>(reader, 2, "orientation", AERO_BODY_PROP2_ORIENTATION_WORDS);
    card.width = reader.requiredReal(3, "width");
    card.aspectRatio = reader.requiredReal(4, "aspectRatio");
    card.slenderHalfWidthListId = reader.integer(5, "slenderHalfWidthListId", 0);
    card.interferenceHalfWidthListId = reader.integer(6, "interferenceHalfWidthListId", 0);
    card.angleListId1 = reader.integer(7, "angleListId1", 0);
    card.angleListId2 = reader.integer(8, "angleListId2", 0);
    card.angleList1FirstElem = reader.integer(9, "angleList1FirstElem", 0);
    card.angleList1LastElem = reader.integer(10, "angleList1LastElem", 0);
    card.angleList2FirstElem = reader.integer(11, "angleList2FirstElem", 0);
    card.angleList2LastElem = reader.integer(12, "angleList2LastElem", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 12 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroBodyProp2Format::check(const AeroBodyProp2Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.width, "width", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.aspectRatio, "aspectRatio", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalId(card.slenderHalfWidthListId, "slenderHalfWidthListId"))
        return problem;
    if (const auto problem = checkOptionalId(card.interferenceHalfWidthListId, "interferenceHalfWidthListId"))
        return problem;
    if (const auto problem = checkOptionalId(card.angleListId1, "angleListId1"))
        return problem;
    if (const auto problem = checkOptionalId(card.angleListId2, "angleListId2"))
        return problem;
    if (const auto problem = checkOptionalId(card.angleList1FirstElem, "angleList1FirstElem"))
        return problem;
    if (const auto problem = checkOptionalId(card.angleList1LastElem, "angleList1LastElem"))
        return problem;
    if (const auto problem = checkOptionalId(card.angleList2FirstElem, "angleList2FirstElem"))
        return problem;
    if (const auto problem = checkOptionalId(card.angleList2LastElem, "angleList2LastElem"))
        return problem;
    // A range lists the first and the last element: it does not go backwards.
    if (card.angleList1FirstElem > 0 && card.angleList1LastElem > 0 &&
        card.angleList1LastElem < card.angleList1FirstElem)
        return CardProblem{"card.bad-value", "{field} cannot be smaller than {field:angleList1FirstElem}",
                           "angleList1LastElem"};
    if (card.angleList2FirstElem > 0 && card.angleList2LastElem > 0 &&
        card.angleList2LastElem < card.angleList2FirstElem)
        return CardProblem{"card.bad-value", "{field} cannot be smaller than {field:angleList2FirstElem}",
                           "angleList2LastElem"};
    return std::nullopt;
}

void AeroBodyProp2Format::write(const AeroBodyProp2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, AERO_BODY_PROP2_ORIENTATION_WORDS, card.orientation);
    writer.real(3, card.width);
    writer.real(4, card.aspectRatio);
    writer.integerUnlessDefault(5, card.slenderHalfWidthListId, 0);
    writer.integerUnlessDefault(6, card.interferenceHalfWidthListId, 0);
    writer.integerUnlessDefault(7, card.angleListId1, 0);
    writer.integerUnlessDefault(8, card.angleListId2, 0);
    writer.integerUnlessDefault(9, card.angleList1FirstElem, 0);
    writer.integerUnlessDefault(10, card.angleList1LastElem, 0);
    writer.integerUnlessDefault(11, card.angleList2FirstElem, 0);
    writer.integerUnlessDefault(12, card.angleList2LastElem, 0);
}

std::uint32_t AeroBodyProp2Table::append(const AeroBodyProp2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_orientation.push(static_cast<std::uint8_t>(card.orientation));
    m_width.push(card.width);
    m_aspectRatio.push(card.aspectRatio);
    m_slenderHalfWidthListId.push(card.slenderHalfWidthListId);
    m_interferenceHalfWidthListId.push(card.interferenceHalfWidthListId);
    m_angleListId1.push(card.angleListId1);
    m_angleListId2.push(card.angleListId2);
    m_angleList1FirstElem.push(card.angleList1FirstElem);
    m_angleList1LastElem.push(card.angleList1LastElem);
    m_angleList2FirstElem.push(card.angleList2FirstElem);
    m_angleList2LastElem.push(card.angleList2LastElem);
    return row;
}

AeroBodyProp2Card AeroBodyProp2Table::card(std::uint32_t row) const
{
    AeroBodyProp2Card card;
    card.id = m_id.get(row);
    card.orientation = static_cast<BodyOrientation>(m_orientation.get(row));
    card.width = m_width.get(row);
    card.aspectRatio = m_aspectRatio.get(row);
    card.slenderHalfWidthListId = m_slenderHalfWidthListId.get(row);
    card.interferenceHalfWidthListId = m_interferenceHalfWidthListId.get(row);
    card.angleListId1 = m_angleListId1.get(row);
    card.angleListId2 = m_angleListId2.get(row);
    card.angleList1FirstElem = m_angleList1FirstElem.get(row);
    card.angleList1LastElem = m_angleList1LastElem.get(row);
    card.angleList2FirstElem = m_angleList2FirstElem.get(row);
    card.angleList2LastElem = m_angleList2LastElem.get(row);
    return card;
}

void AeroBodyProp2Table::store(std::uint32_t row, const AeroBodyProp2Card& card)
{
    m_id.set(row, card.id);
    m_orientation.set(row, static_cast<std::uint8_t>(card.orientation));
    m_width.set(row, card.width);
    m_aspectRatio.set(row, card.aspectRatio);
    m_slenderHalfWidthListId.set(row, card.slenderHalfWidthListId);
    m_interferenceHalfWidthListId.set(row, card.interferenceHalfWidthListId);
    m_angleListId1.set(row, card.angleListId1);
    m_angleListId2.set(row, card.angleListId2);
    m_angleList1FirstElem.set(row, card.angleList1FirstElem);
    m_angleList1LastElem.set(row, card.angleList1LastElem);
    m_angleList2FirstElem.set(row, card.angleList2FirstElem);
    m_angleList2LastElem.set(row, card.angleList2LastElem);
}

void AeroBodyProp2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_orientation.keepRows(rows);
    m_width.keepRows(rows);
    m_aspectRatio.keepRows(rows);
    m_slenderHalfWidthListId.keepRows(rows);
    m_interferenceHalfWidthListId.keepRows(rows);
    m_angleListId1.keepRows(rows);
    m_angleListId2.keepRows(rows);
    m_angleList1FirstElem.keepRows(rows);
    m_angleList1LastElem.keepRows(rows);
    m_angleList2FirstElem.keepRows(rows);
    m_angleList2LastElem.keepRows(rows);
}

void AeroBodyProp2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The four lists of numbers are AEFACT cards; the ranges THIi and THNi are element numbers of a body, not IDs.
    if (const base::EntityId slenderHalfWidthListId = m_slenderHalfWidthListId.get(row); slenderHalfWidthListId > 0)
        m_slenderHalfWidthListId.set(
            row, visitor.visit("slenderHalfWidthListId", ReferenceTarget::AeroFactor, slenderHalfWidthListId));
    if (const base::EntityId interferenceHalfWidthListId = m_interferenceHalfWidthListId.get(row);
        interferenceHalfWidthListId > 0)
        m_interferenceHalfWidthListId.set(row, visitor.visit("interferenceHalfWidthListId", ReferenceTarget::AeroFactor,
                                                             interferenceHalfWidthListId));
    if (const base::EntityId angleListId1 = m_angleListId1.get(row); angleListId1 > 0)
        m_angleListId1.set(row, visitor.visit("angleListId1", ReferenceTarget::AeroFactor, angleListId1));
    if (const base::EntityId angleListId2 = m_angleListId2.get(row); angleListId2 > 0)
        m_angleListId2.set(row, visitor.visit("angleListId2", ReferenceTarget::AeroFactor, angleListId2));
}

} // namespace oor::model
