#include "oor/model/cards/aero/aeroSurfaceSpline4/AeroSurfaceSpline4Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int UNUSED_FIELD = 4;
constexpr int LAST_FIELD = 12;

double blankIfNone(const std::optional<double>& value)
{
    return value ? *value : AeroSurfaceSpline4Table::BLANK;
}

std::optional<double> noneIfBlank(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

} // namespace

std::optional<AeroSurfaceSpline4Card> AeroSurfaceSpline4Format::read(FieldReader& reader)
{
    AeroSurfaceSpline4Card card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.requiredInteger(2, "aeroPanelId");
    card.aeroBoxListId = reader.requiredInteger(3, "aeroBoxListId");
    card.nodeSetId = reader.requiredInteger(5, "nodeSetId");
    card.linearFlexibility = reader.real(6, "linearFlexibility", 0.0);
    card.method = readKeyword<AeroSurfaceSpline4Method>(reader, 7, "method", AERO_SURFACE_SPLINE4_METHOD_WORDS,
                                                        AeroSurfaceSpline4Method::InfinitePlate);
    card.usage = readKeyword<SplineUsage>(reader, 8, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    card.elemCountX = reader.integer(9, "elemCountX", AeroSurfaceSpline4Card::DEFAULT_ELEMENTS);
    card.elemCountY = reader.integer(10, "elemCountY", AeroSurfaceSpline4Card::DEFAULT_ELEMENTS);
    card.functionKind = readKeyword<SplineRadialFunction>(reader, 11, "functionKind", SPLINE_RADIAL_FUNCTION_WORDS,
                                                          SplineRadialFunction::Wendland2);
    card.coreRadius = reader.optionalReal(12, "coreRadius");
    requireBlankFields(reader, {UNUSED_FIELD}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroSurfaceSpline4Format::check(const AeroSurfaceSpline4Card& card)
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
    if (card.elemCountX <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "elemCountX"};
    if (card.elemCountY <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "elemCountY"};
    if (const auto problem = checkOptionalReal(card.coreRadius, "coreRadius", RealRule::Positive))
        return problem;
    // The radius of support has no default: a radial interpolation spline cannot do without it.
    if (card.method == AeroSurfaceSpline4Method::RadialInterpolation && !card.coreRadius)
        return CardProblem{"card.missing-field", "{field} is required when {field:method} is RIS", "coreRadius"};
    return std::nullopt;
}

void AeroSurfaceSpline4Format::write(const AeroSurfaceSpline4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.aeroPanelId);
    writer.integer(3, card.aeroBoxListId);
    writer.integer(5, card.nodeSetId);
    writer.realUnlessDefault(6, card.linearFlexibility, 0.0);
    writeKeywordUnlessDefault(writer, 7, AERO_SURFACE_SPLINE4_METHOD_WORDS, card.method,
                              AeroSurfaceSpline4Method::InfinitePlate);
    writeKeywordUnlessDefault(writer, 8, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
    // The continuation line is optional: it appears only when one of its fields is not the
    // default.
    writer.integerUnlessDefault(9, card.elemCountX, AeroSurfaceSpline4Card::DEFAULT_ELEMENTS);
    writer.integerUnlessDefault(10, card.elemCountY, AeroSurfaceSpline4Card::DEFAULT_ELEMENTS);
    writeKeywordUnlessDefault(writer, 11, SPLINE_RADIAL_FUNCTION_WORDS, card.functionKind,
                              SplineRadialFunction::Wendland2);
    if (card.coreRadius)
        writer.real(12, *card.coreRadius);
}

std::uint32_t AeroSurfaceSpline4Table::append(const AeroSurfaceSpline4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_aeroPanelId.push(card.aeroPanelId);
    m_aeroBoxListId.push(card.aeroBoxListId);
    m_nodeSetId.push(card.nodeSetId);
    m_linearFlexibility.push(card.linearFlexibility);
    m_method.push(static_cast<std::uint8_t>(card.method));
    m_usage.push(static_cast<std::uint8_t>(card.usage));
    m_elemCountX.push(card.elemCountX);
    m_elemCountY.push(card.elemCountY);
    m_functionKind.push(static_cast<std::uint8_t>(card.functionKind));
    m_coreRadius.push(blankIfNone(card.coreRadius));
    return row;
}

AeroSurfaceSpline4Card AeroSurfaceSpline4Table::card(std::uint32_t row) const
{
    AeroSurfaceSpline4Card card;
    card.id = m_id.get(row);
    card.aeroPanelId = m_aeroPanelId.get(row);
    card.aeroBoxListId = m_aeroBoxListId.get(row);
    card.nodeSetId = m_nodeSetId.get(row);
    card.linearFlexibility = m_linearFlexibility.get(row);
    card.method = static_cast<AeroSurfaceSpline4Method>(m_method.get(row));
    card.usage = static_cast<SplineUsage>(m_usage.get(row));
    card.elemCountX = m_elemCountX.get(row);
    card.elemCountY = m_elemCountY.get(row);
    card.functionKind = static_cast<SplineRadialFunction>(m_functionKind.get(row));
    card.coreRadius = noneIfBlank(m_coreRadius.get(row));
    return card;
}

void AeroSurfaceSpline4Table::store(std::uint32_t row, const AeroSurfaceSpline4Card& card)
{
    m_id.set(row, card.id);
    m_aeroPanelId.set(row, card.aeroPanelId);
    m_aeroBoxListId.set(row, card.aeroBoxListId);
    m_nodeSetId.set(row, card.nodeSetId);
    m_linearFlexibility.set(row, card.linearFlexibility);
    m_method.set(row, static_cast<std::uint8_t>(card.method));
    m_usage.set(row, static_cast<std::uint8_t>(card.usage));
    m_elemCountX.set(row, card.elemCountX);
    m_elemCountY.set(row, card.elemCountY);
    m_functionKind.set(row, static_cast<std::uint8_t>(card.functionKind));
    m_coreRadius.set(row, blankIfNone(card.coreRadius));
}

void AeroSurfaceSpline4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_aeroPanelId.keepRows(rows);
    m_aeroBoxListId.keepRows(rows);
    m_nodeSetId.keepRows(rows);
    m_linearFlexibility.keepRows(rows);
    m_method.keepRows(rows);
    m_usage.keepRows(rows);
    m_elemCountX.keepRows(rows);
    m_elemCountY.keepRows(rows);
    m_functionKind.keepRows(rows);
    m_coreRadius.keepRows(rows);
}

void AeroSurfaceSpline4Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId aeroPanelId = m_aeroPanelId.get(row); aeroPanelId > 0)
        m_aeroPanelId.set(row, visitor.visit("aeroPanelId", ReferenceTarget::AeroPanelOrComponent, aeroPanelId));
    if (const base::EntityId aeroBoxListId = m_aeroBoxListId.get(row); aeroBoxListId > 0)
        m_aeroBoxListId.set(row, visitor.visit("aeroBoxListId", ReferenceTarget::AeroList, aeroBoxListId));
    if (const base::EntityId nodeSetId = m_nodeSetId.get(row); nodeSetId > 0)
        m_nodeSetId.set(row, visitor.visit("nodeSetId", ReferenceTarget::Set, nodeSetId));
}

} // namespace oor::model
