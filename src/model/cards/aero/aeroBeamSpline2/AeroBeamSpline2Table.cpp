#include "oor/model/cards/aero/aeroBeamSpline2/AeroBeamSpline2Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int UNUSED_FIELD = 11;
constexpr int LAST_FIELD = 12;

} // namespace

std::optional<AeroBeamSpline2Card> AeroBeamSpline2Format::read(FieldReader& reader)
{
    AeroBeamSpline2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.requiredInteger(2, "aeroPanelId");
    card.firstBox = reader.requiredInteger(3, "firstBox");
    card.lastBox = reader.requiredInteger(4, "lastBox");
    card.nodeSetId = reader.requiredInteger(5, "nodeSetId");
    card.linearFlexibility = reader.real(6, "linearFlexibility", 0.0);
    card.bendingTorsionRatio = reader.real(7, "bendingTorsionRatio", 1.0);
    card.frameId = reader.integer(8, "frameId", 0);
    card.rotationFlexibilityX = reader.real(9, "rotationFlexibilityX", 0.0);
    card.rotationFlexibilityY = reader.real(10, "rotationFlexibilityY", 0.0);
    card.usage = readKeyword<SplineUsage>(reader, 12, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    requireBlankFields(reader, {UNUSED_FIELD}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroBeamSpline2Format::check(const AeroBeamSpline2Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.aeroPanelId, "aeroPanelId"))
        return problem;
    if (const auto problem = checkRequiredId(card.firstBox, "firstBox"))
        return problem;
    if (const auto problem = checkRequiredId(card.lastBox, "lastBox"))
        return problem;
    if (card.lastBox < card.firstBox)
        return CardProblem{"card.bad-value", "{field} cannot be below {field:firstBox}", "lastBox"};
    if (const auto problem = checkRequiredId(card.nodeSetId, "nodeSetId"))
        return problem;
    if (const auto problem = checkReal(card.linearFlexibility, "linearFlexibility", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.bendingTorsionRatio, "bendingTorsionRatio", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    // A negative DTHX or DTHY is a meaningful value (no attachment of that rotation), so only finiteness is checked.
    if (const auto problem = checkReal(card.rotationFlexibilityX, "rotationFlexibilityX", RealRule::Finite))
        return problem;
    return checkReal(card.rotationFlexibilityY, "rotationFlexibilityY", RealRule::Finite);
}

void AeroBeamSpline2Format::write(const AeroBeamSpline2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.aeroPanelId);
    writer.integer(3, card.firstBox);
    writer.integer(4, card.lastBox);
    writer.integer(5, card.nodeSetId);
    writer.realUnlessDefault(6, card.linearFlexibility, 0.0);
    writer.realUnlessDefault(7, card.bendingTorsionRatio, 1.0);
    writer.integerUnlessDefault(8, card.frameId, 0);
    // The continuation line is required: the two flexibilities are written even when they are zero.
    writer.real(9, card.rotationFlexibilityX);
    writer.real(10, card.rotationFlexibilityY);
    writeKeywordUnlessDefault(writer, 12, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
}

std::uint32_t AeroBeamSpline2Table::append(const AeroBeamSpline2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_aeroPanelId.push(card.aeroPanelId);
    m_firstBox.push(card.firstBox);
    m_lastBox.push(card.lastBox);
    m_nodeSetId.push(card.nodeSetId);
    m_linearFlexibility.push(card.linearFlexibility);
    m_bendingTorsionRatio.push(card.bendingTorsionRatio);
    m_frameId.push(card.frameId);
    m_rotationFlexibilityX.push(card.rotationFlexibilityX);
    m_rotationFlexibilityY.push(card.rotationFlexibilityY);
    m_usage.push(static_cast<std::uint8_t>(card.usage));
    return row;
}

AeroBeamSpline2Card AeroBeamSpline2Table::card(std::uint32_t row) const
{
    AeroBeamSpline2Card card;
    card.id = m_id.get(row);
    card.aeroPanelId = m_aeroPanelId.get(row);
    card.firstBox = m_firstBox.get(row);
    card.lastBox = m_lastBox.get(row);
    card.nodeSetId = m_nodeSetId.get(row);
    card.linearFlexibility = m_linearFlexibility.get(row);
    card.bendingTorsionRatio = m_bendingTorsionRatio.get(row);
    card.frameId = m_frameId.get(row);
    card.rotationFlexibilityX = m_rotationFlexibilityX.get(row);
    card.rotationFlexibilityY = m_rotationFlexibilityY.get(row);
    card.usage = static_cast<SplineUsage>(m_usage.get(row));
    return card;
}

void AeroBeamSpline2Table::store(std::uint32_t row, const AeroBeamSpline2Card& card)
{
    m_id.set(row, card.id);
    m_aeroPanelId.set(row, card.aeroPanelId);
    m_firstBox.set(row, card.firstBox);
    m_lastBox.set(row, card.lastBox);
    m_nodeSetId.set(row, card.nodeSetId);
    m_linearFlexibility.set(row, card.linearFlexibility);
    m_bendingTorsionRatio.set(row, card.bendingTorsionRatio);
    m_frameId.set(row, card.frameId);
    m_rotationFlexibilityX.set(row, card.rotationFlexibilityX);
    m_rotationFlexibilityY.set(row, card.rotationFlexibilityY);
    m_usage.set(row, static_cast<std::uint8_t>(card.usage));
}

void AeroBeamSpline2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_aeroPanelId.keepRows(rows);
    m_firstBox.keepRows(rows);
    m_lastBox.keepRows(rows);
    m_nodeSetId.keepRows(rows);
    m_linearFlexibility.keepRows(rows);
    m_bendingTorsionRatio.keepRows(rows);
    m_frameId.keepRows(rows);
    m_rotationFlexibilityX.keepRows(rows);
    m_rotationFlexibilityY.keepRows(rows);
    m_usage.keepRows(rows);
}

void AeroBeamSpline2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId aeroPanelId = m_aeroPanelId.get(row); aeroPanelId > 0)
        m_aeroPanelId.set(row, visitor.visit("aeroPanelId", ReferenceTarget::AeroElement, aeroPanelId));
    if (const base::EntityId firstBox = m_firstBox.get(row); firstBox > 0)
        m_firstBox.set(row, visitor.visit("firstBox", ReferenceTarget::AeroBox, firstBox));
    if (const base::EntityId lastBox = m_lastBox.get(row); lastBox > 0)
        m_lastBox.set(row, visitor.visit("lastBox", ReferenceTarget::AeroBox, lastBox));
    if (const base::EntityId nodeSetId = m_nodeSetId.get(row); nodeSetId > 0)
        m_nodeSetId.set(row, visitor.visit("nodeSetId", ReferenceTarget::Set, nodeSetId));
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
