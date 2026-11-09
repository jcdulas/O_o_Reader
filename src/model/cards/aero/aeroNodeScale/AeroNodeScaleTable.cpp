#include "oor/model/cards/aero/aeroNodeScale/AeroNodeScaleTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 4;

} // namespace

std::optional<AeroNodeScaleCard> AeroNodeScaleFormat::read(FieldReader& reader)
{
    AeroNodeScaleCard card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceLength1 = reader.real(2, "referenceLength1", AeroNodeScaleCard::DEFAULT_REFERENCE_LENGTH);
    card.referenceLength2 = reader.real(3, "referenceLength2", AeroNodeScaleCard::DEFAULT_REFERENCE_LENGTH);
    card.referenceLength3 = reader.real(4, "referenceLength3", AeroNodeScaleCard::DEFAULT_REFERENCE_LENGTH);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.report(CardProblem{"card.bad-value",
                                  "an " + std::string(reader.cardName()) +
                                      " has four fields: {field:id}, {field:referenceLength1}, "
                                      "{field:referenceLength2} and {field:referenceLength3}",
                                  ""});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroNodeScaleFormat::check(const AeroNodeScaleCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.referenceLength1, "referenceLength1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.referenceLength2, "referenceLength2", RealRule::Finite))
        return problem;
    return checkReal(card.referenceLength3, "referenceLength3", RealRule::Finite);
}

void AeroNodeScaleFormat::write(const AeroNodeScaleCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.referenceLength1, AeroNodeScaleCard::DEFAULT_REFERENCE_LENGTH);
    writer.realUnlessDefault(3, card.referenceLength2, AeroNodeScaleCard::DEFAULT_REFERENCE_LENGTH);
    writer.realUnlessDefault(4, card.referenceLength3, AeroNodeScaleCard::DEFAULT_REFERENCE_LENGTH);
}

std::uint32_t AeroNodeScaleTable::append(const AeroNodeScaleCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroNodeScaleTable::store(std::uint32_t row, const AeroNodeScaleCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroNodeScaleTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroNodeScaleTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
