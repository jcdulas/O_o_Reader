#include "oor/model/cards/aero/aeroBeamSpline5/AeroBeamSpline5Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int UNUSED_FIELD = 4;
constexpr int LAST_FIELD = 16;

double blankIfNone(const std::optional<double>& value)
{
    return value ? *value : AeroBeamSpline5Table::BLANK;
}

std::optional<double> noneIfBlank(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

} // namespace

std::optional<AeroBeamSpline5Card> AeroBeamSpline5Format::read(FieldReader& reader)
{
    AeroBeamSpline5Card card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.requiredInteger(2, "aeroPanelId");
    card.aeroBoxListId = reader.requiredInteger(3, "aeroBoxListId");
    card.nodeSetId = reader.requiredInteger(5, "nodeSetId");
    card.linearFlexibility = reader.real(6, "linearFlexibility", 0.0);
    card.bendingTorsionRatio = reader.real(7, "bendingTorsionRatio", 1.0);
    card.frameId = reader.integer(8, "frameId", 0);
    card.rotationFlexibilityX = reader.real(9, "rotationFlexibilityX", 0.0);
    card.rotationFlexibilityY = reader.real(10, "rotationFlexibilityY", 0.0);
    card.usage = readKeyword<SplineUsage>(reader, 12, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    card.method = readKeyword<AeroBeamSpline5Method>(reader, 13, "method", AERO_BEAM_SPLINE5_METHOD_WORDS,
                                                     AeroBeamSpline5Method::Beam);
    card.functionKind = readKeyword<SplineRadialFunction>(reader, 15, "functionKind", SPLINE_RADIAL_FUNCTION_WORDS,
                                                          SplineRadialFunction::Wendland2);
    card.coreRadius = reader.optionalReal(16, "coreRadius");
    // Fields 11 and 14 are not read at all (see the card).
    requireBlankFields(reader, {UNUSED_FIELD}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroBeamSpline5Format::check(const AeroBeamSpline5Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.aeroPanelId, "aeroPanelId"))
        return problem;
    if (const auto problem = checkRequiredId(card.aeroBoxListId, "aeroBoxListId"))
        return problem;
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
    if (const auto problem = checkReal(card.rotationFlexibilityY, "rotationFlexibilityY", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.coreRadius, "coreRadius", RealRule::Positive))
        return problem;
    // The radius of support has no default: a radial interpolation spline cannot do without it.
    if (card.method == AeroBeamSpline5Method::RadialInterpolation && !card.coreRadius)
        return CardProblem{"card.missing-field", "{field} is required when {field:method} is RIS", "coreRadius"};
    return std::nullopt;
}

void AeroBeamSpline5Format::write(const AeroBeamSpline5Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.aeroPanelId);
    writer.integer(3, card.aeroBoxListId);
    writer.integer(5, card.nodeSetId);
    writer.realUnlessDefault(6, card.linearFlexibility, 0.0);
    writer.realUnlessDefault(7, card.bendingTorsionRatio, 1.0);
    writer.integerUnlessDefault(8, card.frameId, 0);
    // The continuation line is required: the two flexibilities are written even when they are zero.
    writer.real(9, card.rotationFlexibilityX);
    writer.real(10, card.rotationFlexibilityY);
    writeKeywordUnlessDefault(writer, 12, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
    writeKeywordUnlessDefault(writer, 13, AERO_BEAM_SPLINE5_METHOD_WORDS, card.method, AeroBeamSpline5Method::Beam);
    writeKeywordUnlessDefault(writer, 15, SPLINE_RADIAL_FUNCTION_WORDS, card.functionKind,
                              SplineRadialFunction::Wendland2);
    if (card.coreRadius)
        writer.real(16, *card.coreRadius);
}

std::uint32_t AeroBeamSpline5Table::append(const AeroBeamSpline5Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_aeroPanelId.push(card.aeroPanelId);
    m_aeroBoxListId.push(card.aeroBoxListId);
    m_nodeSetId.push(card.nodeSetId);
    m_linearFlexibility.push(card.linearFlexibility);
    m_bendingTorsionRatio.push(card.bendingTorsionRatio);
    m_frameId.push(card.frameId);
    m_rotationFlexibilityX.push(card.rotationFlexibilityX);
    m_rotationFlexibilityY.push(card.rotationFlexibilityY);
    m_usage.push(static_cast<std::uint8_t>(card.usage));
    m_method.push(static_cast<std::uint8_t>(card.method));
    m_functionKind.push(static_cast<std::uint8_t>(card.functionKind));
    m_coreRadius.push(blankIfNone(card.coreRadius));
    return row;
}

AeroBeamSpline5Card AeroBeamSpline5Table::card(std::uint32_t row) const
{
    AeroBeamSpline5Card card;
    card.id = m_id.get(row);
    card.aeroPanelId = m_aeroPanelId.get(row);
    card.aeroBoxListId = m_aeroBoxListId.get(row);
    card.nodeSetId = m_nodeSetId.get(row);
    card.linearFlexibility = m_linearFlexibility.get(row);
    card.bendingTorsionRatio = m_bendingTorsionRatio.get(row);
    card.frameId = m_frameId.get(row);
    card.rotationFlexibilityX = m_rotationFlexibilityX.get(row);
    card.rotationFlexibilityY = m_rotationFlexibilityY.get(row);
    card.usage = static_cast<SplineUsage>(m_usage.get(row));
    card.method = static_cast<AeroBeamSpline5Method>(m_method.get(row));
    card.functionKind = static_cast<SplineRadialFunction>(m_functionKind.get(row));
    card.coreRadius = noneIfBlank(m_coreRadius.get(row));
    return card;
}

void AeroBeamSpline5Table::store(std::uint32_t row, const AeroBeamSpline5Card& card)
{
    m_id.set(row, card.id);
    m_aeroPanelId.set(row, card.aeroPanelId);
    m_aeroBoxListId.set(row, card.aeroBoxListId);
    m_nodeSetId.set(row, card.nodeSetId);
    m_linearFlexibility.set(row, card.linearFlexibility);
    m_bendingTorsionRatio.set(row, card.bendingTorsionRatio);
    m_frameId.set(row, card.frameId);
    m_rotationFlexibilityX.set(row, card.rotationFlexibilityX);
    m_rotationFlexibilityY.set(row, card.rotationFlexibilityY);
    m_usage.set(row, static_cast<std::uint8_t>(card.usage));
    m_method.set(row, static_cast<std::uint8_t>(card.method));
    m_functionKind.set(row, static_cast<std::uint8_t>(card.functionKind));
    m_coreRadius.set(row, blankIfNone(card.coreRadius));
}

void AeroBeamSpline5Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_aeroPanelId.keepRows(rows);
    m_aeroBoxListId.keepRows(rows);
    m_nodeSetId.keepRows(rows);
    m_linearFlexibility.keepRows(rows);
    m_bendingTorsionRatio.keepRows(rows);
    m_frameId.keepRows(rows);
    m_rotationFlexibilityX.keepRows(rows);
    m_rotationFlexibilityY.keepRows(rows);
    m_usage.keepRows(rows);
    m_method.keepRows(rows);
    m_functionKind.keepRows(rows);
    m_coreRadius.keepRows(rows);
}

void AeroBeamSpline5Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId aeroPanelId = m_aeroPanelId.get(row); aeroPanelId > 0)
        m_aeroPanelId.set(row, visitor.visit("aeroPanelId", ReferenceTarget::AeroElement, aeroPanelId));
    if (const base::EntityId aeroBoxListId = m_aeroBoxListId.get(row); aeroBoxListId > 0)
        m_aeroBoxListId.set(row, visitor.visit("aeroBoxListId", ReferenceTarget::AeroList, aeroBoxListId));
    if (const base::EntityId nodeSetId = m_nodeSetId.get(row); nodeSetId > 0)
        m_nodeSetId.set(row, visitor.visit("nodeSetId", ReferenceTarget::Set, nodeSetId));
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
