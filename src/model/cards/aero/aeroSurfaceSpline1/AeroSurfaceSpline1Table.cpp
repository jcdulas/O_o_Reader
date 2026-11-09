#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 10;

} // namespace

std::optional<AeroSurfaceSpline1Card> AeroSurfaceSpline1Format::read(FieldReader& reader)
{
    AeroSurfaceSpline1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.requiredInteger(2, "aeroPanelId");
    card.firstBox = reader.requiredInteger(3, "firstBox");
    card.lastBox = reader.requiredInteger(4, "lastBox");
    card.nodeSetId = reader.requiredInteger(5, "nodeSetId");
    card.linearFlexibility = reader.real(6, "linearFlexibility", 0.0);
    card.method = readKeyword<AeroSurfaceSpline1Method>(reader, 7, "method", AERO_SURFACE_SPLINE1_METHOD_WORDS,
                                                        AeroSurfaceSpline1Method::InfinitePlate);
    card.usage = readKeyword<SplineUsage>(reader, 8, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    card.elemCountX = reader.integer(9, "elemCountX", AeroSurfaceSpline1Card::DEFAULT_ELEMENTS);
    card.elemCountY = reader.integer(10, "elemCountY", AeroSurfaceSpline1Card::DEFAULT_ELEMENTS);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 10 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroSurfaceSpline1Format::check(const AeroSurfaceSpline1Card& card)
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
    if (card.elemCountX <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "elemCountX"};
    if (card.elemCountY <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "elemCountY"};
    return std::nullopt;
}

void AeroSurfaceSpline1Format::write(const AeroSurfaceSpline1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.aeroPanelId);
    writer.integer(3, card.firstBox);
    writer.integer(4, card.lastBox);
    writer.integer(5, card.nodeSetId);
    writer.realUnlessDefault(6, card.linearFlexibility, 0.0);
    writeKeywordUnlessDefault(writer, 7, AERO_SURFACE_SPLINE1_METHOD_WORDS, card.method,
                              AeroSurfaceSpline1Method::InfinitePlate);
    writeKeywordUnlessDefault(writer, 8, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
    // The continuation line is optional: it appears only when a count is not the default of 10.
    writer.integerUnlessDefault(9, card.elemCountX, AeroSurfaceSpline1Card::DEFAULT_ELEMENTS);
    writer.integerUnlessDefault(10, card.elemCountY, AeroSurfaceSpline1Card::DEFAULT_ELEMENTS);
}

std::uint32_t AeroSurfaceSpline1Table::append(const AeroSurfaceSpline1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_aeroPanelId.push(card.aeroPanelId);
    m_firstBox.push(card.firstBox);
    m_lastBox.push(card.lastBox);
    m_nodeSetId.push(card.nodeSetId);
    m_linearFlexibility.push(card.linearFlexibility);
    m_method.push(static_cast<std::uint8_t>(card.method));
    m_usage.push(static_cast<std::uint8_t>(card.usage));
    m_elemCountX.push(card.elemCountX);
    m_elemCountY.push(card.elemCountY);
    return row;
}

AeroSurfaceSpline1Card AeroSurfaceSpline1Table::card(std::uint32_t row) const
{
    AeroSurfaceSpline1Card card;
    card.id = m_id.get(row);
    card.aeroPanelId = m_aeroPanelId.get(row);
    card.firstBox = m_firstBox.get(row);
    card.lastBox = m_lastBox.get(row);
    card.nodeSetId = m_nodeSetId.get(row);
    card.linearFlexibility = m_linearFlexibility.get(row);
    card.method = static_cast<AeroSurfaceSpline1Method>(m_method.get(row));
    card.usage = static_cast<SplineUsage>(m_usage.get(row));
    card.elemCountX = m_elemCountX.get(row);
    card.elemCountY = m_elemCountY.get(row);
    return card;
}

void AeroSurfaceSpline1Table::store(std::uint32_t row, const AeroSurfaceSpline1Card& card)
{
    m_id.set(row, card.id);
    m_aeroPanelId.set(row, card.aeroPanelId);
    m_firstBox.set(row, card.firstBox);
    m_lastBox.set(row, card.lastBox);
    m_nodeSetId.set(row, card.nodeSetId);
    m_linearFlexibility.set(row, card.linearFlexibility);
    m_method.set(row, static_cast<std::uint8_t>(card.method));
    m_usage.set(row, static_cast<std::uint8_t>(card.usage));
    m_elemCountX.set(row, card.elemCountX);
    m_elemCountY.set(row, card.elemCountY);
}

void AeroSurfaceSpline1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_aeroPanelId.keepRows(rows);
    m_firstBox.keepRows(rows);
    m_lastBox.keepRows(rows);
    m_nodeSetId.keepRows(rows);
    m_linearFlexibility.keepRows(rows);
    m_method.keepRows(rows);
    m_usage.keepRows(rows);
    m_elemCountX.keepRows(rows);
    m_elemCountY.keepRows(rows);
}

void AeroSurfaceSpline1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId aeroPanelId = m_aeroPanelId.get(row); aeroPanelId > 0)
        m_aeroPanelId.set(row, visitor.visit("aeroPanelId", ReferenceTarget::AeroElement, aeroPanelId));
    if (const base::EntityId firstBox = m_firstBox.get(row); firstBox > 0)
        m_firstBox.set(row, visitor.visit("firstBox", ReferenceTarget::AeroBox, firstBox));
    if (const base::EntityId lastBox = m_lastBox.get(row); lastBox > 0)
        m_lastBox.set(row, visitor.visit("lastBox", ReferenceTarget::AeroBox, lastBox));
    if (const base::EntityId nodeSetId = m_nodeSetId.get(row); nodeSetId > 0)
        m_nodeSetId.set(row, visitor.visit("nodeSetId", ReferenceTarget::Set, nodeSetId));
}

} // namespace oor::model
