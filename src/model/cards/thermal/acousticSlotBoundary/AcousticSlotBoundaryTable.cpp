#include "oor/model/cards/thermal/acousticSlotBoundary/AcousticSlotBoundaryTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<AcousticSlotBoundaryCard> AcousticSlotBoundaryFormat::read(FieldReader& reader)
{
    AcousticSlotBoundaryCard card;
    card.density = reader.optionalReal(1, "density");
    card.slotCount = reader.optionalInteger(2, "slotCount");
    const int last = reader.lastUsedField();
    for (int field = FIRST_POINT_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue; // the padding of a line
        card.points.push_back(reader.requiredInteger(field, "slotPoint"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticSlotBoundaryFormat::check(const AcousticSlotBoundaryCard& card)
{
    if (const auto problem = checkOptionalReal(card.density, "density", RealRule::Positive))
        return problem;
    if (card.slotCount && *card.slotCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "slotCount"};
    if (card.points.size() < AcousticSlotBoundaryCard::MIN_POINTS)
        return CardProblem{"card.missing-field", "the card needs at least two slot points", "slotPoint2"};
    for (const base::EntityId point : card.points)
    {
        if (const auto problem = checkRequiredId(point, "slotPoint"))
            return problem;
    }
    return std::nullopt;
}

void AcousticSlotBoundaryFormat::write(const AcousticSlotBoundaryCard& card, FieldWriter& writer)
{
    if (card.density)
        writer.real(1, *card.density);
    if (card.slotCount)
        writer.integer(2, *card.slotCount);
    int field = FIRST_POINT_FIELD;
    for (const base::EntityId point : card.points)
        writer.integer(field++, point);
}

std::uint32_t AcousticSlotBoundaryTable::append(const AcousticSlotBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_pointCount.push(static_cast<std::int32_t>(card.points.size()));
    m_cards.push(card);
    return row;
}

void AcousticSlotBoundaryTable::store(std::uint32_t row, const AcousticSlotBoundaryCard& card)
{
    m_pointCount.set(row, static_cast<std::int32_t>(card.points.size()));
    m_cards.set(row, card);
}

void AcousticSlotBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_pointCount.keepRows(rows);
    m_cards.keepRows(rows);
}

void AcousticSlotBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (base::EntityId& point : m_cards.edit(row).points)
        visitId(visitor, "slotPoint", ReferenceTarget::Node, point);
}

} // namespace oor::model
