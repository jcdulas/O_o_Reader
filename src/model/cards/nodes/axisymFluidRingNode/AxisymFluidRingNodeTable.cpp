#include "oor/model/cards/nodes/axisymFluidRingNode/AxisymFluidRingNodeTable.hpp"

#include "oor/base/text/DigitSets.hpp"

namespace oor::model
{

namespace
{

// PS is a list of the digits 1 to 6 with no repeat ("123456", "3", "246").
bool isValidPermanentConstraint(std::int32_t digits)
{
    if (digits == 0)
        return true;
    bool seen[7] = {};
    for (std::int32_t rest = digits; rest > 0; rest /= 10)
    {
        const std::int32_t digit = rest % 10;
        if (digit < 1 || digit > 6 || seen[digit])
            return false;
        seen[digit] = true;
    }
    return digits > 0;
}

} // namespace

std::optional<AxisymFluidRingNodeCard> AxisymFluidRingNodeFormat::read(FieldReader& reader)
{
    AxisymFluidRingNodeCard card;
    card.id = reader.requiredInteger(1, "id");
    // Fields 2, 3 and 5 are ignored.
    card.phiAngle = reader.requiredReal(4, "phiAngle");
    card.displacementFrameId = reader.integer(6, "displacementFrameId", 0);
    card.permanentConstraints = base::sortedDigits(reader.integer(7, "permanentConstraints", 0));
    card.fluidPointId = reader.requiredInteger(8, "fluidPointId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymFluidRingNodeFormat::check(const AxisymFluidRingNodeCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the ID must be a positive integer", "id"};
    if (card.displacementFrameId < 0)
        return CardProblem{"card.bad-value", "{field} must be 0 or a coordinate system ID", "displacementFrameId"};
    if (!isValidPermanentConstraint(card.permanentConstraints))
        return CardProblem{"card.bad-value", "{field} must be digits 1 to 6 with no repeat", "permanentConstraints"};
    if (card.fluidPointId <= 0)
        return CardProblem{"card.bad-id",
                           "{field} must be the ID of a " + CardProblem::cardNamed("AxisymFluidRing") + " entry",
                           "fluidPointId"};
    return std::nullopt;
}

void AxisymFluidRingNodeFormat::write(const AxisymFluidRingNodeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(4, card.phiAngle);
    writer.integerUnlessDefault(6, card.displacementFrameId, 0);
    if (card.permanentConstraints != 0)
        writer.integer(7, card.permanentConstraints);
    writer.integer(8, card.fluidPointId);
}

std::uint32_t AxisymFluidRingNodeTable::append(const AxisymFluidRingNodeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_phiAngle.push(card.phiAngle);
    m_displacementFrameId.push(card.displacementFrameId);
    m_permanentConstraints.push(card.permanentConstraints);
    m_fluidPointId.push(card.fluidPointId);
    return row;
}

AxisymFluidRingNodeCard AxisymFluidRingNodeTable::card(std::uint32_t row) const
{
    return AxisymFluidRingNodeCard{m_id.get(row), m_phiAngle.get(row), m_displacementFrameId.get(row),
                                   m_permanentConstraints.get(row), m_fluidPointId.get(row)};
}

void AxisymFluidRingNodeTable::store(std::uint32_t row, const AxisymFluidRingNodeCard& card)
{
    m_id.set(row, card.id);
    m_phiAngle.set(row, card.phiAngle);
    m_displacementFrameId.set(row, card.displacementFrameId);
    m_permanentConstraints.set(row, card.permanentConstraints);
    m_fluidPointId.set(row, card.fluidPointId);
}

void AxisymFluidRingNodeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_phiAngle.keepRows(rows);
    m_displacementFrameId.keepRows(rows);
    m_permanentConstraints.keepRows(rows);
    m_fluidPointId.keepRows(rows);
}

void AxisymFluidRingNodeTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId displacementFrame = m_displacementFrameId.get(row); displacementFrame > 0)
        m_displacementFrameId.set(row,
                                  visitor.visit("displacementFrameId", ReferenceTarget::Coordinate, displacementFrame));
    // IDF names a RINGFL entry, a point of the node ID space.
    if (const base::EntityId fluidPoint = m_fluidPointId.get(row); fluidPoint > 0)
        m_fluidPointId.set(row, visitor.visit("fluidPointId", ReferenceTarget::Node, fluidPoint));
}

} // namespace oor::model
