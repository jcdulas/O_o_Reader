#include "oor/model/cards/nodes/acousticSlotSurfacePoint/AcousticSlotSurfacePointTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5; // IDF

} // namespace

std::optional<AcousticSlotSurfacePointCard> AcousticSlotSurfacePointFormat::read(FieldReader& reader)
{
    AcousticSlotSurfacePointCard card;
    card.id = reader.requiredInteger(1, "id");
    card.radius = reader.requiredReal(2, "radius");
    card.zCoord = reader.requiredReal(3, "zCoord");
    card.slotWidth = reader.optionalReal(4, "slotWidth");
    card.fluidPointId = reader.integer(5, "fluidPointId", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticSlotSurfacePointFormat::check(const AcousticSlotSurfacePointCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.radius, "radius", RealRule::Finite))
        return problem;
    if (card.radius == 0.0)
        return CardProblem{"card.bad-value", "{field} must not be zero", "radius"};
    if (const auto problem = checkReal(card.zCoord, "zCoord", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.slotWidth, "slotWidth", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalId(card.fluidPointId, "fluidPointId"))
        return problem;
    // A point on an interface needs a radius above zero.
    if (card.fluidPointId > 0 && card.radius <= 0.0)
        return CardProblem{"card.bad-value",
                           "{field} must be above zero when " + CardProblem::fieldNamed("fluidPointId") + " is given",
                           "radius"};
    return std::nullopt;
}

void AcousticSlotSurfacePointFormat::write(const AcousticSlotSurfacePointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.radius);
    writer.real(3, card.zCoord);
    if (card.slotWidth)
        writer.real(4, *card.slotWidth);
    if (card.fluidPointId != 0)
        writer.integer(5, card.fluidPointId);
}

std::uint32_t AcousticSlotSurfacePointTable::append(const AcousticSlotSurfacePointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_radius.push(card.radius);
    m_zCoord.push(card.zCoord);
    m_slotWidth.push(realToColumn(card.slotWidth));
    m_fluidPointId.push(card.fluidPointId);
    return row;
}

AcousticSlotSurfacePointCard AcousticSlotSurfacePointTable::card(std::uint32_t row) const
{
    AcousticSlotSurfacePointCard card;
    card.id = m_id.get(row);
    card.radius = m_radius.get(row);
    card.zCoord = m_zCoord.get(row);
    card.slotWidth = realFromColumn(m_slotWidth.get(row));
    card.fluidPointId = m_fluidPointId.get(row);
    return card;
}

void AcousticSlotSurfacePointTable::store(std::uint32_t row, const AcousticSlotSurfacePointCard& card)
{
    m_id.set(row, card.id);
    m_radius.set(row, card.radius);
    m_zCoord.set(row, card.zCoord);
    m_slotWidth.set(row, realToColumn(card.slotWidth));
    m_fluidPointId.set(row, card.fluidPointId);
}

void AcousticSlotSurfacePointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_radius.keepRows(rows);
    m_zCoord.keepRows(rows);
    m_slotWidth.keepRows(rows);
    m_fluidPointId.keepRows(rows);
}

void AcousticSlotSurfacePointTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The IDF names a GRIDF fluid point, which is a point of the node ID space.
    if (const base::EntityId fluidPoint = m_fluidPointId.get(row); fluidPoint > 0)
        m_fluidPointId.set(row, visitor.visit("fluidPointId", ReferenceTarget::Node, fluidPoint));
}

} // namespace oor::model
