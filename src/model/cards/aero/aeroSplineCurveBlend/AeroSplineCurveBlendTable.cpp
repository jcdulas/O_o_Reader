#include "oor/model/cards/aero/aeroSplineCurveBlend/AeroSplineCurveBlendTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 7;

} // namespace

std::optional<AeroSplineCurveBlendCard> AeroSplineCurveBlendFormat::read(FieldReader& reader)
{
    AeroSplineCurveBlendCard card;
    card.id = reader.requiredInteger(1, "id");
    card.splineId1 = reader.requiredInteger(2, "splineId1");
    card.splineId2 = reader.requiredInteger(3, "splineId2");
    card.option = readKeyword<AeroSplineCurveBlendOption>(reader, 4, "option", AERO_SPLINE_CURVE_BLEND_OPTION_WORDS,
                                                          AeroSplineCurveBlendOption::Linear);
    card.aeroBoxListId = reader.requiredInteger(5, "aeroBoxListId");
    card.depth1 = reader.requiredReal(6, "depth1");
    card.depth2 = reader.requiredReal(7, "depth2");
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroSplineCurveBlendFormat::check(const AeroSplineCurveBlendCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.splineId1, "splineId1"))
        return problem;
    if (const auto problem = checkRequiredId(card.splineId2, "splineId2"))
        return problem;
    if (const auto problem = checkRequiredId(card.aeroBoxListId, "aeroBoxListId"))
        return problem;
    if (const auto problem = checkReal(card.depth1, "depth1", RealRule::Positive))
        return problem;
    return checkReal(card.depth2, "depth2", RealRule::Positive);
}

void AeroSplineCurveBlendFormat::write(const AeroSplineCurveBlendCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.splineId1);
    writer.integer(3, card.splineId2);
    writeKeywordUnlessDefault(writer, 4, AERO_SPLINE_CURVE_BLEND_OPTION_WORDS, card.option,
                              AeroSplineCurveBlendOption::Linear);
    writer.integer(5, card.aeroBoxListId);
    writer.real(6, card.depth1);
    writer.real(7, card.depth2);
}

std::uint32_t AeroSplineCurveBlendTable::append(const AeroSplineCurveBlendCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroSplineCurveBlendTable::store(std::uint32_t row, const AeroSplineCurveBlendCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroSplineCurveBlendTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroSplineCurveBlendTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroSplineCurveBlendTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroSplineCurveBlendCard& card = m_cards.edit(row);
    visitId(visitor, "splineId1", ReferenceTarget::AeroSpline, card.splineId1);
    visitId(visitor, "splineId2", ReferenceTarget::AeroSpline, card.splineId2);
    visitId(visitor, "aeroBoxListId", ReferenceTarget::AeroList, card.aeroBoxListId);
}

} // namespace oor::model
