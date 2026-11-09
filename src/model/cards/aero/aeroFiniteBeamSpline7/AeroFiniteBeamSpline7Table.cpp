#include "oor/model/cards/aero/aeroFiniteBeamSpline7/AeroFiniteBeamSpline7Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 16;

} // namespace

std::optional<AeroFiniteBeamSpline7Card> AeroFiniteBeamSpline7Format::read(FieldReader& reader)
{
    AeroFiniteBeamSpline7Card card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.integer(2, "aeroPanelId", 0);
    card.aeroBoxListId = reader.requiredInteger(3, "aeroBoxListId");
    card.nodeSetId = reader.requiredInteger(5, "nodeSetId");
    card.linearFlexibility = reader.real(6, "linearFlexibility", 0.0);
    card.bendingTorsionRatio = reader.real(7, "bendingTorsionRatio", 1.0);
    card.frameId = reader.integer(8, "frameId", 0);
    card.usage = readKeyword<SplineUsage>(reader, 12, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    card.method = readKeyword<AeroFiniteBeamSpline7Method>(reader, 13, "method", AERO_FINITE_BEAM_SPLINE7_METHOD_WORDS,
                                                           AeroFiniteBeamSpline7Method::TranslationsAndRotations);
    card.rotationFlexibility = reader.real(14, "rotationFlexibility", 0.0);
    card.bendingExtensionRatio = reader.real(15, "bendingExtensionRatio", 1.0);
    card.minBeamLengthRatio = reader.real(16, "minBeamLengthRatio", 0.01);
    requireBlankFields(reader, {4, 9, 10, 11}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroFiniteBeamSpline7Format::check(const AeroFiniteBeamSpline7Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.aeroPanelId, "aeroPanelId"))
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
    if (const auto problem = checkReal(card.rotationFlexibility, "rotationFlexibility", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.bendingExtensionRatio, "bendingExtensionRatio", RealRule::Positive))
        return problem;
    return checkReal(card.minBeamLengthRatio, "minBeamLengthRatio", RealRule::Positive);
}

void AeroFiniteBeamSpline7Format::write(const AeroFiniteBeamSpline7Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.aeroPanelId, 0);
    writer.integer(3, card.aeroBoxListId);
    writer.integer(5, card.nodeSetId);
    writer.realUnlessDefault(6, card.linearFlexibility, 0.0);
    writer.realUnlessDefault(7, card.bendingTorsionRatio, 1.0);
    writer.integerUnlessDefault(8, card.frameId, 0);
    writeKeywordUnlessDefault(writer, 12, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
    writeKeywordUnlessDefault(writer, 13, AERO_FINITE_BEAM_SPLINE7_METHOD_WORDS, card.method,
                              AeroFiniteBeamSpline7Method::TranslationsAndRotations);
    writer.realUnlessDefault(14, card.rotationFlexibility, 0.0);
    writer.realUnlessDefault(15, card.bendingExtensionRatio, 1.0);
    writer.realUnlessDefault(16, card.minBeamLengthRatio, 0.01);
}

std::uint32_t AeroFiniteBeamSpline7Table::append(const AeroFiniteBeamSpline7Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroFiniteBeamSpline7Table::store(std::uint32_t row, const AeroFiniteBeamSpline7Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroFiniteBeamSpline7Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroFiniteBeamSpline7Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroFiniteBeamSpline7Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroFiniteBeamSpline7Card& card = m_cards.edit(row);
    visitId(visitor, "aeroPanelId", ReferenceTarget::AeroElement, card.aeroPanelId);
    visitId(visitor, "aeroBoxListId", ReferenceTarget::AeroList, card.aeroBoxListId);
    visitId(visitor, "nodeSetId", ReferenceTarget::Set, card.nodeSetId);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
}

} // namespace oor::model
