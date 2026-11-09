#include "oor/model/cards/nodes/acousticSlotFluidPoint/AcousticSlotFluidPointTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr base::EntityId FLUID_POINT_LIMIT = 1000000; // an ID is below it (format reference: 0 < Integer < 1000000)
constexpr int LAST_FIELD = 3;

} // namespace

std::optional<AcousticSlotFluidPointCard> AcousticSlotFluidPointFormat::read(FieldReader& reader)
{
    AcousticSlotFluidPointCard card;
    card.id = reader.requiredInteger(1, "id");
    card.radius = reader.requiredReal(2, "radius");
    card.zCoord = reader.requiredReal(3, "zCoord");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticSlotFluidPointFormat::check(const AcousticSlotFluidPointCard& card)
{
    if (card.id <= 0 || card.id >= FLUID_POINT_LIMIT)
        return CardProblem{"card.bad-id", "the ID must be an integer from 1 to 999999", "id"};
    if (const auto problem = checkReal(card.radius, "radius", RealRule::Positive))
        return problem;
    return checkReal(card.zCoord, "zCoord", RealRule::Finite);
}

void AcousticSlotFluidPointFormat::write(const AcousticSlotFluidPointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.radius);
    writer.real(3, card.zCoord);
}

std::uint32_t AcousticSlotFluidPointTable::append(const AcousticSlotFluidPointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_radius.push(card.radius);
    m_zCoord.push(card.zCoord);
    return row;
}

AcousticSlotFluidPointCard AcousticSlotFluidPointTable::card(std::uint32_t row) const
{
    return AcousticSlotFluidPointCard{m_id.get(row), m_radius.get(row), m_zCoord.get(row)};
}

void AcousticSlotFluidPointTable::store(std::uint32_t row, const AcousticSlotFluidPointCard& card)
{
    m_id.set(row, card.id);
    m_radius.set(row, card.radius);
    m_zCoord.set(row, card.zCoord);
}

void AcousticSlotFluidPointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_radius.keepRows(rows);
    m_zCoord.keepRows(rows);
}

} // namespace oor::model
